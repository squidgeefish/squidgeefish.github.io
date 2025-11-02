---
layout: default
permalink: /iv-3-flex-sao
---

# ИB-3 Flex SAO

It is not a Nixie!

What if you bought a pile of cheap nine-segment VFDs from Ukraine on eBay and a pile of cheap (used...) high-voltage driver chips from Aliexpress? Clearly the only reasonable application is to make a batch order of flex PCBs with an integrated stiffener from JLCPCB.

We have a 20V boost converter and a CH32V003 microcontroller on board. The switch on the top of the SAO allows you to shut off both the filament drive and the 20V boost converter if you want to save power (current draw is around 150mA) or reduce your RF emission profile.

## Source Files

Hardware source files and less-janky software directory tree will be uploaded soon™ (currently still in the midst of conference madness).

https://github.com/squidgeefish/NineSegSAO/tree/main

## Usage Caveats

The pins on the SAO header are only soldered to the flex PCB on one side. If you push too hard on the header pins, you can rip them off of the flex PCB.

If you are trying to stick individual DuPont-style jumpers on the pins, I recommend unfolding the tube from the housing and holding your thumb directly over the pins on the flex PCB side so that they cannot go anywhere.

If you are putting this SAO (or any SAO!) on the Supercon 9 badge, make very sure that you get the box header aligned to the cutout in the front panel. If your SAO does not seat fully, it may fall off...

The front panel on the badge this year has 0-tolerance cutout for the SAO box header, and the hand-assembly of the badges appears not to have incorporated an alignment jig. 

It may also be worth hot-gluing the SAO connector shroud to the SAO itself since some of them slide more freely than others, causing premature stress to the flex PCB pads.

## GPIO Pins

Just like the CH32V003 SAO at Supercon 8, the SWIO programming pin is broken out to pin 5 (`GPIO1`) of the SAO header (and so this SAO can be reprogrammed using the Hackaday Europe firmware for the Supercon 8 badge).

Pin 6 (`GPIO2`) is not connected.

## I2C Registers

The I2C functionality uses the `i2c_slave.h` header from the fantastic `ch32fun` repo. It, uh, also uses the default 0x09 I2C address (0x12 with R/W bit) because I forgot to change it.

(The I2C functionality ran into some deeply undefined behavior in the compiler at 2:30AM the day before I was handing these out, and so it is less fleshed-out than ideal.)

Register values do not persist across power cycles of the SAO.

### Operating Mode

| Register | Default Value | Usage |
|----------|---------------|-------|
| 0/0x0    | 0/1  | Operating mode |

There are two animations (courtesy of cnlohr) that the badge alternates between every 10 seconds or so. 

| Value | Mode |
|-------|------|
| 0     | Segment-fading "shader" |
| 1     | Count-up mode |
| 2     | Fixed output of number in register 1 |

Unfortunately, I forgot to add a mode that is dedicated to cycling between modes 0 and 1, so the SAO will undo any setting to this register after ten seconds on average...

Mode 2 exists as a way to use this SAO as a numerical display; just remember that you'll need to re-send this every few seconds to keep the SAO from auto-cycling back to mode 0 or 1.

### Mode-2 Register Value

| Register | Default Value | Usage |
|----------|---------------|-------|
| 1/0x1 | 7 | Value to display in Mode 2 |
