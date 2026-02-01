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
  * Forbidden characters: `0x00`, `0x11`, `0x13`, `0x1A`, `0x24`, `0xCB`, `0xFD`, `0xFE`, `0xFF` (plus aforementioned characters)
  * Offset: 64 (0x40)

D-Star references a "frame lost" padding pattern, which contains the sequence `E7 84 76` in the data part of a 20ms frame. `0x84` is forbidden to avoid reproducing that pattern (in the second segment of a DV Slow data stream). That value is given as scrambled. When unscrambled, the banned value is `0xCB`. When operating in Fast Data, it isn't necessary to escape other values, since the mitigation byte, present in the Audio part of a frame, is set to `0x02` to mute the AMBE decoder.

`0x24` is escaped to avoid triggering the D-PRS frame interpreter.

`0x11` and `0x13` are control characters used by the serial line software flow control.

The `0xFD` to `0xFF` range is used by CI-V commands.

The `0x00` was escaped in D-Rats, but the rationale wasn't documented. Also, it isn't referenced as a value of concern in  probably escaped to avoid issues with string

## Control channel
SetHardware commands can be used to control the radio, regardless of the port number.
Port 14 is dedicated for the future inter-TNC communication (ping, dv fast data auto-upgrade).
