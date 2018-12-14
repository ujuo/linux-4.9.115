/*
 * ASoC simple sound card support
 *
 * Copyright (C) 2012 Renesas Solutions Corp.
 * Kuninori Morimoto <kuninori.morimoto.gx@renesas.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/clk.h>
#include <linux/device.h>
#include <linux/gpio.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/platform_device.h>
#include <linux/string.h>
#include <sound/jack.h>
#include <sound/simple_card.h>
#include <sound/soc-dai.h>
#include <sound/soc.h>
#include <sound/pcm_params.h>

struct simple_card_data {
	struct snd_soc_card snd_card;
	struct simple_dai_props {
		struct asoc_simple_dai cpu_dai;
		struct asoc_simple_dai codec_dai;
		unsigned int mclk_fs;
	} *dai_props;
	unsigned int mclk_fs;
	int gpio_hp_det;
	int gpio_hp_det_invert;
	int gpio_mic_det;
	int gpio_mic_det_invert;
	struct snd_soc_dai_link dai_link[];	/* dynamically allocated */
};

#define simple_priv_to_dev(priv) ((priv)->snd_card.dev)
#define simple_priv_to_link(priv, i) ((priv)->snd_card.dai_link + i)
#define simple_priv_to_props(priv, i) ((priv)->dai_props + i)

static int nx_simple_card_startup(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct simple_card_data *priv =	snd_soc_card_get_drvdata(rtd->card);
	struct simple_dai_props *dai_props =
		&priv->dai_props[rtd - rtd->card->rtd];
	int ret;

	ret = clk_prepare_enable(dai_props->cpu_dai.clk);
	if (ret)
		return ret;

	ret = clk_prepare_enable(dai_props->codec_dai.clk);
	if (ret)
		clk_disable_unprepare(dai_props->cpu_dai.clk);

	return ret;
}

static void nx_simple_card_shutdown(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct simple_card_data *priv =	snd_soc_card_get_drvdata(rtd->card);
	struct simple_dai_props *dai_props =
		&priv->dai_props[rtd - rtd->card->rtd];

	clk_disable_unprepare(dai_props->cpu_dai.clk);

	clk_disable_unprepare(dai_props->codec_dai.clk);
}

static int nx_simple_card_hw_params(struct snd_pcm_substream *substream,
				      struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	struct simple_card_data *priv = snd_soc_card_get_drvdata(rtd->card);
	struct simple_dai_props *dai_props =
		&priv->dai_props[rtd - rtd->card->rtd];
	unsigned int mclk, mclk_fs = 0;
	int ret = 0;

	if (priv->mclk_fs)
		mclk_fs = priv->mclk_fs;
	else if (dai_props->mclk_fs)
		mclk_fs = dai_props->mclk_fs;

	if (mclk_fs) {
		mclk = params_rate(params) * mclk_fs;
		ret = snd_soc_dai_set_sysclk(codec_dai, 0, mclk,
					     SND_SOC_CLOCK_IN);
		if (ret && ret != -ENOTSUPP)
			goto err;

		ret = snd_soc_dai_set_sysclk(cpu_dai, 0, mclk,
					     SND_SOC_CLOCK_OUT);
		if (ret && ret != -ENOTSUPP)
			goto err;
	} else {
		mclk_fs = (params_format(params) == SNDRV_PCM_FORMAT_S24_LE) ?
			384 : 256;
		mclk = params_rate(params) * mclk_fs;
		ret = snd_soc_dai_set_sysclk(codec_dai, 0, mclk,
					     SND_SOC_CLOCK_IN);
		if (ret && ret != -ENOTSUPP)
			goto err;

		ret = snd_soc_dai_set_sysclk(cpu_dai, 0, mclk,
					     SND_SOC_CLOCK_OUT);
		if (ret && ret != -ENOTSUPP)
			goto err;
	}

	return 0;
err:
	return ret;
}

static struct snd_soc_ops nx_simple_card_ops = {
	.startup = nx_simple_card_startup,
	.shutdown = nx_simple_card_shutdown,
	.hw_params = nx_simple_card_hw_params,
};

static struct snd_soc_jack nx_simple_card_hp_jack;
static struct snd_soc_jack_pin nx_simple_card_hp_jack_pins[] = {
	{
		.pin = "Headphones",
		.mask = SND_JACK_HEADPHONE,
	},
};
static struct snd_soc_jack_gpio nx_simple_card_hp_jack_gpio = {
	.name = "Headphone detection",
	.report = SND_JACK_HEADPHONE,
	.debounce_time = 150,
};

static struct snd_soc_jack nx_simple_card_mic_jack;
static struct snd_soc_jack_pin nx_simple_card_mic_jack_pins[] = {
	{
		.pin = "Mic Jack",
		.mask = SND_JACK_MICROPHONE,
	},
};
static struct snd_soc_jack_gpio nx_simple_card_mic_jack_gpio = {
	.name = "Mic detection",
	.report = SND_JACK_MICROPHONE,
	.debounce_time = 150,
};

static int __nx_simple_card_dai_init(struct snd_soc_dai *dai,
				       struct asoc_simple_dai *set)
{
	int ret;

	if (set->sysclk) {
		ret = snd_soc_dai_set_sysclk(dai, 0, set->sysclk, 0);
		if (ret && ret != -ENOTSUPP) {
			dev_err(dai->dev, "simple-card: set_sysclk error\n");
			goto err;
		}
	}

	if (set->slots) {
		ret = snd_soc_dai_set_tdm_slot(dai,
					       set->tx_slot_mask,
					       set->rx_slot_mask,
						set->slots,
						set->slot_width);
		if (ret && ret != -ENOTSUPP) {
			dev_err(dai->dev, "simple-card: set_tdm_slot error\n");
			goto err;
		}
	}

	ret = 0;

err:
	return ret;
}

static int nx_simple_card_dai_init(struct snd_soc_pcm_runtime *rtd)
{
	struct simple_card_data *priv =	snd_soc_card_get_drvdata(rtd->card);
	struct snd_soc_dai *codec = rtd->codec_dai;
	struct snd_soc_dai *cpu = rtd->cpu_dai;
	struct simple_dai_props *dai_props;
	int num, ret;
	unsigned int mclk_fs = 0;

	num = rtd - rtd->card->rtd;
	dai_props = &priv->dai_props[num];

	if (priv->mclk_fs)
		mclk_fs = priv->mclk_fs;
	else if (dai_props->mclk_fs)
		mclk_fs = dai_props->mclk_fs;

	ret = __nx_simple_card_dai_init(codec, &dai_props->codec_dai);
	if (ret < 0)
		return ret;

	if (mclk_fs) {
		cpu->driver->playback.formats =
			(mclk_fs == 256) ? SNDRV_PCM_FMTBIT_S16_LE :
			SNDRV_PCM_FMTBIT_S24_LE;
		cpu->driver->capture.formats =
			(mclk_fs == 256) ? SNDRV_PCM_FMTBIT_S16_LE :
			SNDRV_PCM_FMTBIT_S24_LE;
	}

	ret = __nx_simple_card_dai_init(cpu, &dai_props->cpu_dai);
	if (ret < 0)
		return ret;

	if (gpio_is_valid(priv->gpio_hp_det)) {
		snd_soc_card_jack_new(rtd->card, "Headphones",
				      SND_JACK_HEADPHONE,
				      &nx_simple_card_hp_jack,
				      nx_simple_card_hp_jack_pins,
				      ARRAY_SIZE(nx_simple_card_hp_jack_pins));

		nx_simple_card_hp_jack_gpio.gpio = priv->gpio_hp_det;
		nx_simple_card_hp_jack_gpio.invert = priv->gpio_hp_det_invert;
		snd_soc_jack_add_gpios(&nx_simple_card_hp_jack, 1,
				       &nx_simple_card_hp_jack_gpio);
	}

	if (gpio_is_valid(priv->gpio_mic_det)) {
		snd_soc_card_jack_new(rtd->card, "Mic Jack",
				      SND_JACK_MICROPHONE,
				      &nx_simple_card_mic_jack,
				      nx_simple_card_mic_jack_pins,
				      ARRAY_SIZE(nx_simple_card_mic_jack_pins));
		nx_simple_card_mic_jack_gpio.gpio = priv->gpio_mic_det;
		nx_simple_card_mic_jack_gpio.invert = priv->gpio_mic_det_invert;
		snd_soc_jack_add_gpios(&nx_simple_card_mic_jack, 1,
				       &nx_simple_card_mic_jack_gpio);
	}
	return 0;
}

static int
nx_simple_card_sub_parse_of(struct device_node *np,
			      struct asoc_simple_dai *dai,
			      struct device_node **p_node,
			      const char **name,
			      int *args_count)
{
	struct of_phandle_args args;
	struct clk *clk;
	u32 val;
	int ret;

	/*
	 * Get node via "sound-dai = <&phandle port>"
	 * it will be used as xxx_of_node on soc_bind_dai_link()
	 */
	ret = of_parse_phandle_with_args(np, "sound-dai",
					 "#sound-dai-cells", 0, &args);
	if (ret)
		return ret;

	*p_node = args.np;

	if (args_count)
		*args_count = args.args_count;

	/* Get dai->name */
	ret = snd_soc_of_get_dai_name(np, name);
	if (ret < 0)
		return ret;

	/* Parse TDM slot */
	ret = snd_soc_of_parse_tdm_slot(np, &dai->tx_slot_mask,
					&dai->rx_slot_mask,
					&dai->slots, &dai->slot_width);
	if (ret)
		return ret;

	/*
	 * Parse dai->sysclk come from "clocks = <&xxx>"
	 * (if system has common clock)
	 *  or "system-clock-frequency = <xxx>"
	 *  or device's module clock.
	 */
	if (of_property_read_bool(np, "clocks")) {
		clk = of_clk_get(np, 0);
		if (IS_ERR(clk)) {
			ret = PTR_ERR(clk);
			return ret;
		}

		dai->sysclk = clk_get_rate(clk);
		dai->clk = clk;
	} else if (!of_property_read_u32(np, "system-clock-frequency", &val)) {
		dai->sysclk = val;
	} else {
		clk = of_clk_get(args.np, 0);
		if (!IS_ERR(clk))
			dai->sysclk = clk_get_rate(clk);
	}

	return 0;
}

static int nx_simple_card_parse_daifmt(struct device_node *node,
					 struct simple_card_data *priv,
					 struct device_node *codec,
					 char *prefix, int idx)
{
	struct snd_soc_dai_link *dai_link = simple_priv_to_link(priv, idx);
	struct device *dev = simple_priv_to_dev(priv);
	struct device_node *bitclkmaster = NULL;
	struct device_node *framemaster = NULL;
	unsigned int daifmt;

	daifmt = snd_soc_of_parse_daifmt(node, prefix,
					 &bitclkmaster, &framemaster);
	daifmt &= ~SND_SOC_DAIFMT_MASTER_MASK;

	if (strlen(prefix) && !bitclkmaster && !framemaster) {
		/*
		 * No dai-link level and master setting was not found from
		 * sound node level, revert back to legacy DT parsing and
		 * take the settings from codec node.
		 */
		dev_dbg(dev, "Revert to legacy daifmt parsing\n");

		daifmt = snd_soc_of_parse_daifmt(codec, NULL, NULL, NULL) |
			(daifmt & ~SND_SOC_DAIFMT_CLOCK_MASK);
	} else {
		if (codec == bitclkmaster)
			daifmt |= (codec == framemaster) ?
				SND_SOC_DAIFMT_CBM_CFM : SND_SOC_DAIFMT_CBM_CFS;
		else
			daifmt |= (codec == framemaster) ?
				SND_SOC_DAIFMT_CBS_CFM : SND_SOC_DAIFMT_CBS_CFS;
	}

	dai_link->dai_fmt = daifmt;

	of_node_put(bitclkmaster);
	of_node_put(framemaster);

	return 0;
}

static int nx_simple_card_dai_link_of(struct device_node *node,
					struct simple_card_data *priv,
					int idx,
					bool is_top_level_node)
{
	struct device *dev = simple_priv_to_dev(priv);
	struct snd_soc_dai_link *dai_link = simple_priv_to_link(priv, idx);
	struct simple_dai_props *dai_props = simple_priv_to_props(priv, idx);
	struct device_node *cpu = NULL;
	struct device_node *plat = NULL;
	struct device_node *codec = NULL;
	char *name;
	char prop[128];
	char *prefix = "";
	int ret, cpu_args;
	u32 val;

	/* For single DAI link & old style of DT node */
	if (is_top_level_node)
		prefix = "simple-audio-card,";

	snprintf(prop, sizeof(prop), "%scpu", prefix);
	cpu = of_get_child_by_name(node, prop);

	snprintf(prop, sizeof(prop), "%splat", prefix);
	plat = of_get_child_by_name(node, prop);

	snprintf(prop, sizeof(prop), "%scodec", prefix);
	codec = of_get_child_by_name(node, prop);

	if (!cpu || !codec) {
		ret = -EINVAL;
		dev_err(dev, "%s: Can't find %s DT node\n", __func__, prop);
		goto dai_link_of_err;
	}

	ret = nx_simple_card_parse_daifmt(node, priv,
					    codec, prefix, idx);
	if (ret < 0)
		goto dai_link_of_err;

	if (!of_property_read_u32(node, "mclk-fs", &val))
		dai_props->mclk_fs = val;

	ret = nx_simple_card_sub_parse_of(cpu, &dai_props->cpu_dai,
					    &dai_link->cpu_of_node,
					    &dai_link->cpu_dai_name,
					    &cpu_args);
	if (ret < 0)
		goto dai_link_of_err;

	ret = nx_simple_card_sub_parse_of(codec, &dai_props->codec_dai,
					    &dai_link->codec_of_node,
					    &dai_link->codec_dai_name, NULL);
	if (ret < 0)
		goto dai_link_of_err;

	if (!dai_link->cpu_dai_name || !dai_link->codec_dai_name) {
		ret = -EINVAL;
		goto dai_link_of_err;
	}

	if (plat) {
		struct of_phandle_args args;

		ret = of_parse_phandle_with_args(plat, "sound-dai",
						 "#sound-dai-cells", 0, &args);
		dai_link->platform_of_node = args.np;
	} else {
		/* Assumes platform == cpu */
		dai_link->platform_of_node = dai_link->cpu_of_node;
	}

	/* DAI link name is created from CPU/CODEC dai name */
	name = devm_kzalloc(dev,
			    strlen(dai_link->cpu_dai_name)   +
			    strlen(dai_link->codec_dai_name) + 2,
			    GFP_KERNEL);
	if (!name) {
		ret = -ENOMEM;
		goto dai_link_of_err;
	}

	sprintf(name, "%s-%s", dai_link->cpu_dai_name,
				dai_link->codec_dai_name);
	dai_link->name = dai_link->stream_name = name;
	dai_link->ops = &nx_simple_card_ops;
	dai_link->init = nx_simple_card_dai_init;

	dev_dbg(dev, "\tname : %s\n", dai_link->stream_name);
	dev_dbg(dev, "\tformat : %04x\n", dai_link->dai_fmt);
	dev_dbg(dev, "\tcpu : %s / %d\n",
		dai_link->cpu_dai_name,
		dai_props->cpu_dai.sysclk);
	dev_dbg(dev, "\tcodec : %s / %d\n",
		dai_link->codec_dai_name,
		dai_props->codec_dai.sysclk);

	/*
	 * In soc_bind_dai_link() will check cpu name after
	 * of_node matching if dai_link has cpu_dai_name.
	 * but, it will never match if name was created by
	 * fmt_single_name() remove cpu_dai_name if cpu_args
	 * was 0. See:
	 *	fmt_single_name()
	 *	fmt_multiple_name()
	 */
	if (!cpu_args)
		dai_link->cpu_dai_name = NULL;

dai_link_of_err:
	of_node_put(cpu);
	of_node_put(codec);

	return ret;
}

static int nx_simple_card_parse_of(struct device_node *node,
				     struct simple_card_data *priv)
{
	struct device *dev = simple_priv_to_dev(priv);
	enum of_gpio_flags flags;
	u32 val;
	int ret;

	if (!node)
		return -EINVAL;

	/* Parse the card name from DT */
	snd_soc_of_parse_card_name(&priv->snd_card, "simple-audio-card,name");

	/* The off-codec widgets */
	if (of_property_read_bool(node, "simple-audio-card,widgets")) {
		ret = snd_soc_of_parse_audio_simple_widgets(&priv->snd_card,
					"simple-audio-card,widgets");
		if (ret)
			return ret;
	}

	/* DAPM routes */
	if (of_property_read_bool(node, "simple-audio-card,routing")) {
		ret = snd_soc_of_parse_audio_routing(&priv->snd_card,
					"simple-audio-card,routing");
		if (ret)
			return ret;
	}

	/* Factor to mclk, used in hw_params() */
	ret = of_property_read_u32(node, "simple-audio-card,mclk-fs", &val);
	if (ret == 0)
		priv->mclk_fs = val;

	dev_dbg(dev, "New simple-card: %s\n", priv->snd_card.name ?
		priv->snd_card.name : "");

	/* Single/Muti DAI link(s) & New style of DT node */
	if (of_get_child_by_name(node, "simple-audio-card,dai-link")) {
		struct device_node *np = NULL;
		int i = 0;

		for_each_child_of_node(node, np) {
			dev_dbg(dev, "\tlink %d:\n", i);
			ret = nx_simple_card_dai_link_of(np, priv,
							   i, false);
			if (ret < 0) {
				of_node_put(np);
				return ret;
			}
			i++;
		}
	} else {
		/* For single DAI link & old style of DT node */
		ret = nx_simple_card_dai_link_of(node, priv, 0, true);
		if (ret < 0)
			return ret;
	}

	priv->gpio_hp_det = of_get_named_gpio_flags(node,
				"simple-audio-card,hp-det-gpio", 0, &flags);
	priv->gpio_hp_det_invert = !!(flags & OF_GPIO_ACTIVE_LOW);
	if (priv->gpio_hp_det == -EPROBE_DEFER)
		return -EPROBE_DEFER;

	priv->gpio_mic_det = of_get_named_gpio_flags(node,
				"simple-audio-card,mic-det-gpio", 0, &flags);
	priv->gpio_mic_det_invert = !!(flags & OF_GPIO_ACTIVE_LOW);
	if (priv->gpio_mic_det == -EPROBE_DEFER)
		return -EPROBE_DEFER;

	if (!priv->snd_card.name)
		priv->snd_card.name = priv->snd_card.dai_link->name;

	return 0;
}

/* Decrease the reference count of the device nodes */
static int nx_simple_card_unref(struct snd_soc_card *card)
{
	struct snd_soc_dai_link *dai_link;
	int num_links;

	for (num_links = 0, dai_link = card->dai_link;
	     num_links < card->num_links;
	     num_links++, dai_link++) {
		of_node_put(dai_link->cpu_of_node);
		of_node_put(dai_link->codec_of_node);
	}
	return 0;
}

static int nx_simple_card_probe(struct platform_device *pdev)
{
	struct simple_card_data *priv;
	struct snd_soc_dai_link *dai_link;
	struct device_node *np = pdev->dev.of_node;
	struct device *dev = &pdev->dev;
	int num_links, ret;

	/* Get the number of DAI links */
	if (np && of_get_child_by_name(np, "simple-audio-card,dai-link"))
		num_links = of_get_child_count(np);
	else
		num_links = 1;

	/* Allocate the private data and the DAI link array */
	priv = devm_kzalloc(dev,
			sizeof(*priv) + sizeof(*dai_link) * num_links,
			GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	/* Init snd_soc_card */
	priv->snd_card.owner = THIS_MODULE;
	priv->snd_card.dev = dev;
	dai_link = priv->dai_link;
	priv->snd_card.dai_link = dai_link;
	priv->snd_card.num_links = num_links;

	priv->gpio_hp_det = -ENOENT;
	priv->gpio_mic_det = -ENOENT;

	/* Get room for the other properties */
	priv->dai_props = devm_kzalloc(dev,
			sizeof(*priv->dai_props) * num_links,
			GFP_KERNEL);
	if (!priv->dai_props)
		return -ENOMEM;

	if (np && of_device_is_available(np)) {

		ret = nx_simple_card_parse_of(np, priv);
		if (ret < 0) {
			if (ret != -EPROBE_DEFER)
				dev_err(dev, "parse error %d\n", ret);
			goto err;
		}

	} else {
		struct asoc_simple_card_info *cinfo;

		cinfo = dev->platform_data;
		if (!cinfo) {
			dev_err(dev, "no info for nx-simple-card\n");
			return -EINVAL;
		}

		if (!cinfo->name ||
		    !cinfo->codec_dai.name ||
		    !cinfo->codec ||
		    !cinfo->platform ||
		    !cinfo->cpu_dai.name) {
			dev_err(dev, "insufficient nx_simple_card_info settings\n");
			return -EINVAL;
		}

		priv->snd_card.name	=
			(cinfo->card) ? cinfo->card : cinfo->name;
		dai_link->name		= cinfo->name;
		dai_link->stream_name	= cinfo->name;
		dai_link->platform_name	= cinfo->platform;
		dai_link->codec_name	= cinfo->codec;
		dai_link->cpu_dai_name	= cinfo->cpu_dai.name;
		dai_link->codec_dai_name = cinfo->codec_dai.name;
		dai_link->dai_fmt	= cinfo->daifmt;
		dai_link->init		= nx_simple_card_dai_init;
		memcpy(&priv->dai_props->cpu_dai, &cinfo->cpu_dai,
					sizeof(priv->dai_props->cpu_dai));
		memcpy(&priv->dai_props->codec_dai, &cinfo->codec_dai,
					sizeof(priv->dai_props->codec_dai));

	}

	snd_soc_card_set_drvdata(&priv->snd_card, priv);

	ret = devm_snd_soc_register_card(&pdev->dev, &priv->snd_card);
	if (ret >= 0)
		return ret;

err:
	nx_simple_card_unref(&priv->snd_card);
	return ret;
}

static int nx_simple_card_remove(struct platform_device *pdev)
{
	struct snd_soc_card *card = platform_get_drvdata(pdev);
	struct simple_card_data *priv = snd_soc_card_get_drvdata(card);

	if (gpio_is_valid(priv->gpio_hp_det))
		snd_soc_jack_free_gpios(&nx_simple_card_hp_jack, 1,
					&nx_simple_card_hp_jack_gpio);
	if (gpio_is_valid(priv->gpio_mic_det))
		snd_soc_jack_free_gpios(&nx_simple_card_mic_jack, 1,
					&nx_simple_card_mic_jack_gpio);

	return nx_simple_card_unref(card);
}

static const struct of_device_id nx_simple_of_match[] = {
	{ .compatible = "nexell,simple-audio-card", },
	{},
};
MODULE_DEVICE_TABLE(of, nx_simple_of_match);

static struct platform_driver nx_simple_card = {
	.driver = {
		.name = "nx-simple-card",
		.pm = &snd_soc_pm_ops,
		.of_match_table = nx_simple_of_match,
	},
	.probe = nx_simple_card_probe,
	.remove = nx_simple_card_remove,
};

module_platform_driver(nx_simple_card);

MODULE_ALIAS("platform:nx-simple-card");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("ASoC Simple Sound Card");
MODULE_AUTHOR("Kuninori Morimoto <kuninori.morimoto.gx@renesas.com>");
