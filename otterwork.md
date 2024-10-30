---
layout: default
permalink: /otterwork/
---

# Otterwork

Ought to work, ought ter work, otter work, otterwork...

Spoiler alert: it didn't work (the first time), so I had to add a dorkopod:

![dorkopod](/otterwork/dorkopod.png)

## GPIO Pins

I have added bodge wires to hook up GPIO2 to pin PD5 (for programming via SWIM) and to attach GPIO1 to PC5 so that you can drive the WS2812s directly in operating mode 6.

## I2C Register Interface

The I2C interface uses I2C address 0x37 (0x6E if you're including the R/W bit).

I2C register values are not persisted across device power-off.

### Operating Modes

| Register | Default Value | Usage |
|----------|---------------|-------|
| 128/0x80 | 5    | Operating mode |

Register 128 (0x80) allows you to step through different animation patterns.

| Value | Mode |
|-------|------|
| 0     | Rotating Pinwheel |
| 1     | Horizontally-scrolling Rainbow |
| 2     | Expanding Bullseye Pattern |
| 3     | Vertically-scrolling Rainbow |
| 4     | `[3R:2G:3B]`-encoded values from I2C framebuffer |
| 5     | Cycle through patterns 0-3 (default) |
| 6     | Set LED driver pin high-Z |

### Framebuffer

| Register | Default Value | Usage      |
|--------------|-----------|------------|
| 0-127 (0x7F) | 5 | Mode 4 Framebuffer |

Registers 0 to 127 (0x7F) inclusive provide a `[3R:2G:3B]`-packed color encoding for each pixel, 0-indexed.

### Brightness

| Register | Default Value | Usage     |
|--------------|-----------|-----------|
| 129 (0x81) | 12 | Brightness Divisor |

Register 129 (0x81) is the value by which the internal 24-bit color channels are divided for brightness. I wouldn't go brighter than 5 for your eyes' sake, but if you have some canned air for cooling or something you could try it...\
If you go dimmer than about 16, you really start seeing the internal subpixels rather than letting the package's built-in diffuser do its work.

## Programming

I have a SOIC-8 STM8S001J3 on here so that we have some basic blinkiness available, but it would have been better to have something with a little more grunt - a FastLED port would be cool, but the only one I found online is like six years old...

Anyway, the build chain and WS2812 assembly driver are shamelessly cribbed from [stecman's WS2812 gist](https://gist.github.com/stecman/eed14f9a01d50b9e376429959f35493a). As the intro there says, it's quick and hacked together, and so I've pruned it down a bit while glomming I2C and some other lighting effects in there. `main.c` has a reasonable number of comments including links to anywhere I used for reference.

### Source Code

This is long enough that my typical "dump it all at the end of the article" approach is going to be hard on your scroll bar, so here are the raw files.

[SConscript](/otterwork/Sconscript)\
[SConstruct](/otterwork/Sconstruct)\
[main.c](/otterwork/main.c)\
[map.h](/otterwork/map.h)
