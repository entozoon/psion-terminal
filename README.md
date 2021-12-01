# Terminal Bitch Monitor

LilyGo T5 4.7" e-ink terminal display reading, say the output of a Pi, via serial pins.

Borrowing massively from [this library](https://github.com/vroland/epdiy), this is a standalone device but only really exists to serve a [larger project](https://github.com/entozoon/terminal-bitch).

## Hangups

- Ideally wanting to use the intrepid [vroland](https://github.com/vroland)'s terminal software but there's an issue with this [example and platformio](https://github.com/vroland/epdiy/issues/12#issuecomment-983142030)

## Principles

- Rotation is possible, so orientating upside down in the build is fine
- Serial read is possible, and output from a Pi untested but surely fine...
- Low power enough to run from same 5V as Pi _but_ ideally reading battery life, so you know.. don't do that

## To Do

- Abstract this as its own device, to be included properly in the larger terminal-bitch project.

## Notes:

- The [LilyGo library](https://github.com/Xinyuan-LilyGO/LilyGo-EPD47) just ripped off [this one](https://github.com/vroland/epdiy), so I'll use that instead (it supports rotation..)
- There's a lilygo terminal example there in fact, and battery level!
