# Terminal Bitch Monitor

LilyGo T5 4.7" e-ink terminal display reading, say the output of a Pi, via serial pins.

This is a standalone device but only really exists to serve a [larger project](https://github.com/entozoon/terminal-bitch).

## Hangups

- Ideally wanting to use the intrepid [vroland](https://github.com/vroland)'s terminal software but there's an issue with this [example and platformio](https://github.com/vroland/epdiy/issues/12#issuecomment-983142030). I've managed to get it working at a drastically reduced row/column count but that isn't gonna fly.

## Principles

- Rotation is possible, so orientating upside down in the build is fine
- Serial read is possible, and output from a Pi untested but surely fine...
- Low power enough to run from same 5V as Pi _but_ ideally reading battery life, so you know.. don't do that

## To Do

- Abstract this as its own device, to be included properly in the larger terminal-bitch project.

## Type through Pi serial via ssh

```bash
sudo apt-get install screen
sudo screen /dev/ttyAMA0 115200
(which will be blank on computer)
```

## Notes:

- The [LilyGo library](https://github.com/Xinyuan-LilyGO/LilyGo-EPD47) just ripped off [this one](https://github.com/vroland/epdiy), so I'll use that instead (it supports rotation..)
- There's a lilygo terminal example there in fact, and battery level!
