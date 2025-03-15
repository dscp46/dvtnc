# D-Star TNC

> [!CAUTION]
> For now, this repo is a scratchpad to collect ideas.

That project aims to build a KISS software TNC to interface AX.25 packet applications with D-Star DV slow and fast data.

## Radio support
What we need:
  * On launch, collect existing settings and save them [P1]
  * Auto-detect radio type, then set our own settings auto-magically. [P1]
  * Auto QSY to the appropriate channel [P2]
  * On termination, restore prior settings [P1]
  * DV fast data auto upgrade [P3]
  * CI-V commands bridging through SetHardware commands [P4]

On the Icom side, I've noted 2 interface models to access the data channel:
  * CI-V Transceive commands (ID-51s work like this)
  * CI-V plus dedicated communication channel.

TODO: dig in Kenwood specs

## Data encapsulation format:
Yencode the KISS frames prior to transmitting them as is.

yEncoding parameters:
  * Start Marker: `` TODO: find a statistically unlikely character
  * End Marker: `` TODO: find a statistically unlikely character
  * Escape character: `=` (`0x3D`)
  * Forbidden characters: `0x00`, `0x11`, `0x13`, `0x1A`, `0xFD`, `0xFE`, `0xFF`
  * Offset: 64

## Control channel
SetHardware commands can be used to control the radio, regardless of the port number.
Port 14 is dedicated for the future inter-TNC communication (ping, dv fast data auto-upgrade).
