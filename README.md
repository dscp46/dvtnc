# D-Star DV TNC

> [!WARNING]
> The current code is a quick and dirty PoC. Please check later if you're looking for something stable.

That project aims to build a KISS software TNC to bridge AX.25 packet applications though D-Star DV slow and fast data.
On the great scheme of things, the idea is to communicate with a [digital Service Gateway](https://github.com/dscp46/dsgw/).

## Radio support
What we need:
  * On launch, collect existing settings and save them [P1]
  * On termination, restore prior settings [P1]
  * Auto-detect radio type, then set our own settings auto-magically. [P1]
  * Buffer management, proper channel flow control [P2]
  * Auto QSY to the appropriate channel [P3]
  * DV fast data auto upgrade [P4]
  * CI-V commands bridging through SetHardware commands [P5]
  * AGWPE compatibility [P6]
  * AX.25 Header compression (Similar to RFC2508) [P7]

On the Icom side, I've noted 2 interface models to access the data channel:
  * CI-V Transceive commands (ID-51s work like this)
  * CI-V plus dedicated communication channel.

TODO: dig in Kenwood specs

## Data encapsulation format:
Strip the frame marker character, then Yencode the KISS frames prior to transmitting them as is.

yEncoding parameters:
  * Start Marker: `0xE1`
  * End Marker: `0xE0`
  * Escape character: `=` (`0x3D`)
  * Forbidden characters: `0x00`, `0x11`, `0x13`, `0x1A`, `0x24`, `0x84`, `0xFD`, `0xFE`, `0xFF` (plus aforementioned characters)
  * Offset: 64

`0x84` is forbidden to avoid reproducing the "frame lost" pattern. `0x24` is forbidden to avoid triggering the D-PRS frame interpreter. `0x11` and `0x13` are used by software flow control, and `0xFD` to `0xFF` are used for CI-V commands.

## Control channel
SetHardware commands can be used to control the radio, regardless of the port number.
Port 14 is dedicated for the future inter-TNC communication (ping, dv fast data auto-upgrade).
