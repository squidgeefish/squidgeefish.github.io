---
layout: project
title: A1502 Water Damage
description: Diagnosing and repairing a water-damaged A1502 motherboard
slug: a1502-water-damage
image: /assets/a1502-water-damage/banner.jpg
date: 2022-03-19

---

## Background

I've been using a 13" 2014 MacBook Pro (A1502) for a few years now, ever since I picked it up for a steal on Craigslist. However, with the low price came a few problems: dead pixels, misaligned backlight diffuser sheets, assorted frame damage, and a few missing internal components thanks to the previous owner's choice of an unscrupulous repair shop...

Since I'm rather fond of this laptop, with its USB Type-A ports, scissor-switch keyboard, and removable storage (now upgraded to an NVMe drive), I decided to poke around on eBay to see if I could find a donor machine from which I would be able to harvest the chassis and screen, effectively overhauling the user-facing portions of my laptop. As anybody who has spent any time around me might suspect, this led to a rather obsessive trawl through eBay's "for parts or not working" section that consumed far too much of my free time over the course of a couple weeks.

I wound up winning an auction for a late-2013 13" MacBook Pro (also an A1502, of course) which was indeed in good cosmetic shape (the screen is basically perfect, which is surpring given these laptops' predilection towards delamination of the anti-glare coating). As the seller said, it had a problem with the fans constantly revving up and incredibly slow performance. While I could have just moved my good motherboard over to this chassis and called it a day, that would hardly be any fun, right?

## Troubleshooting

The laptop wouldn't even boot at first - I got the classic `Your computer restarted because of a problem. Press a key or wait a few seconds to continue starting up` black screen of death. Booting it in verbose mode (holding `command-V` on startup), I was able to see that a kernel panic was occuring. However, the kext and stack trace were walking the actual cause of the panic off the top of the screen. After a few rounds of rebooting while taking rapid-fire pictures with my phone, I was able to see the following message:
```
panic(cpu 0 caller 0xfffff801522c5a2): "failed to mount data volume!"@AppleInternal/BuildRoot/Library/Caches/com.apple.xbs/Sources/xnu/xnu-6153.141.1/bsd/kern/bsd_init.c:1137
```
This message actually shows up in Apple's Darwin source code online, which is kinda neat. I didn't dig too far into it, however, since it seemed like the problem was with the macOS install (Apple has split the system and user data volumes since 10.15 Catalina, and the data volume here appeared to be borked). Since there were also signs that the previous user had tried installing OpenCore on here (possibly in preparation for a macOS 12 upgrade?), I just swapped the drive with a known-good 128GB SSD.

This succeeded in booting into macOS, though it did take forever to do so, with the fans ramping up to full wind-tunnel mode. At this point, it seemed pretty clear that something in the thermal system was having issues, since Macs will crank the fans all the way up and downclock the processor to 0.8GHz if they think they have a power or thermal problem. Since the battery and charger were properly recognized, I was pretty sure something was up with the thermals.

From the `powermetrics` command, I was able to determine that, while the CPU die temperature was perfectly happy at around 25 C, I had `cpu thermal level 255` and `cpu asserting prochot` errors despite the SMC asserting `Number of prochots: 0`. The `PROCHOT` signal is a CPU-internal flag that indicates that it's necessary to thermal-throttle, which is what I was seeing despite the fact that the CPU temperature was perfectly fine...

I installed `istats` via Ruby and was able to have it iterate through all the temperature sensors on the motherboard. Comparing it to my known-good motherboard, a few sensors stood out as erroneous:
```
Th1H Fin Stack Proximity:   -128.0°C
Ts0S Memory Bank Proximity: -128.0°C
Ts1S Unknown:               -128.0°C
TA0P Ambient Temperature:   -128.0°C
TC0P CPU 0 Proximity:       -128.0°C
TH0V Unknown:               -127.0°C
TM0P Memory Slot Proximity: -128.0°C
```

It was at this point that I got clever and actually ripped the laptop apart to see what might be lurking inside... As soon as I had the board out of the chassis, a pretty obviously-smoking gun emerged.
![Blue corrosion surrounding an IC on the bottom of the board](/assets/{{page.slug}}/corroded.jpg)
For reference, this is how this chip looks on a (much happier) motherboard from a 13" 2015 MacBook Air (A1369) (so, yes, that blue crud on my motherboard is a certifiably bad thing):
![Uncorroded EMC1704-2](/assets/{{page.slug}}/a1369-untouched.jpg)

This chip is a [Microchip/SMSC EMC1704-2 I2C temperature monitor](https://www.microchip.com/en-us/product/EMC1704-2), which can provide up to four temperature readings over SMBUS. On this Mac's problematic 820-3536 motherboard, this is `U5870`, and it provides the `Th1H`, `TA0P`, `TC0P`, and `TM0P` temperature signals - all of which just so happen to be reading erroneously.

![Apple schematic for U5870](/assets/{{page.slug}}/schematic.jpg)

## Repair
After cleaning the board up (and consulting an 820-3536 boardview), I found that the corrosion had eaten a few parts and their pads. Namely, I was missing `R870` (a 47-Ohm resistor delivering `PP3V3_S0` to this IC), `C5870` (0.1μF decoupling capacitor), and `R5875` (0-Ohm resistor tying `CPUTHMSNS_ADDR_SEL` to ground, choosing the I2C address).
![U5870 after a good cleaning, showing missing pads and components](/assets/{{page.slug}}/cleaned.jpg)
`R5870` and `C5870`, respectively, are the missing pads on the left side, while `R5875` is the vertically-aligned pair of pads on the right.

Since the pads (and their internal vias) were totally corroded away, I wound up dead-bugging replacement 0603s for `R5870` and `C5870` and then running 36AWG wire to `PP3V3_S0` and ground points, as well as soldering onto what was left of a `PP3V3_S0_CPUTHMSNS_R` trace after scraping the soldermask off of it. I also ran a 40AWG wire from `CPUTHMSNS_ADDR_SEL` to ground, replacing `R5875`.
There's also a 36AWG wire running to the high side of `R5871`, a 100K pullup resistor for `CPUTHMSNS_THM_L`, since that connection seemed to have been broken by the corrosion as well.
![36AWG bodge wires rebuilding traces near U5870 on an A1502 motherboard](/assets/{{page.slug}}/rework.jpg)

After verifying that it worked (no more wind tunnel impersonations!), I stabilized the resistor with some superglue and finished the motherboard swap, rejuvenating my 2014 Mac and adding another MacBook to my stack of "repaired but not really resale-worthy" hardware. Unfortunately, I can't upgrade its 4GB of RAM to 16GB; that requires the 820-4924 motherboard with 8 DRAM chips...

Fun fact: Apple Diagnostics (previously Apple Hardware Test) used to tell you what sensors specifically were having trouble, but they've removed that functionality in the newest versions. Running Apple Diagnostics on this machinge (holding `D` on startup) gave me error codes `PFM006` ("There may be an issue with the SMC (System Management Controller)") and `PPN001` ("There may be an issue with the power management system"), which is probably why the eBay seller said that this motherboard "has a faulty System Management Controller."  

So I suppose the moral of the story is to rip it apart before getting too deep into the weeds and making a pile of symlinks to get Ruby to run on the latest version of macOS...
