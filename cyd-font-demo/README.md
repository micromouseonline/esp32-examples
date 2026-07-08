# cyd-font-demo

Boots straight into a font browser -- no menu, no other modes. PREV/NEXT
touch buttons cycle through every embedded font (plus the DSEG7 7-segment
font) so you can compare candidates before picking one for use elsewhere.

Extracted from `cerberus-gate-controller`'s Font Demo supervisor entry.
`font-demo.h` is unchanged from that project; everything else here is a
minimal standalone harness around it (no supervisor state machine, no
input-event queue, no NeoKey/GPIO producers -- just direct touch polling).

## Targets

CYD-family boards only (all four have touch; M5 Core does not, so it isn't
a target here):

| PlatformIO env                              | Board                             |
|----------------------------------------------|------------------------------------|
| cyd-font-demo-s3-cyd-touch-freenove          | Freenove FNK0104B ESP32-S3 CYD     |
| cyd-font-demo-cyd2usb-diymalls-ili9341       | CYD2USB (DIYMalls, ILI9341 panel)  |
| cyd-font-demo-cyd2usb-diymalls-st7789        | CYD2USB (DIYMalls, ST7789 panel)   |
| cyd-font-demo-jc2432w328c                    | JC2432W328C                        |

## Build

```
pio run -e cyd-font-demo-s3-cyd-touch-freenove
pio run -e cyd-font-demo-s3-cyd-touch-freenove -t upload
```

See the [workspace build guide](../BUILDING.md) for details on targeting
different boards.
