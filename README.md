# D-Star DV TNC

> [!WARNING]
> The current code is a quick and dirty PoC. Please check later if you're looking for something stable.

That project aims to build a KISS software TNC to bridge AX.25 packet applications though D-Star DV slow and fast data.
On the great scheme of things, the idea is to communicate with a [digital Service Gateway](https://github.com/dscp46/dsgw/).

## Radio support
What we need:
  * [ ] On launch, collect existing settings and save them [P1]
  * [ ] On termination, restore prior settings [P1]
  * [ ] Buffer management, proper channel flow control [P1]
  * [ ] Auto-detect radio type, then set our own settings auto-magically. [P2]
  * [ ] DV fast data auto upgrade [P3]
  * [ ] Auto QSY to the appropriate channel [P4]
  * [ ] CI-V commands bridging through SetHardware commands [P5]
  * [ ] AGWPE compatibility [P6]

### Icom Radios
On the Icom side, I've noted 3 possible interface models to access the data channel:
  * Hybrid data / CI-V port (ID-51s work like this), discrimination between the two modes is done through a different serial line speed.
  * CI-V Data Transceive commands (through the `22 00` commands). Some additional work needed to understand how back pressure is applicable.  
  * CI-V plus dedicated communication channel (IC-705, IC-7100).

### Kenwood
I've only dug in the TH-D74A/E documentation, Kenwood proposes DV Fast data communication, further tests needed.
We could reuse the dedicated communication channel model.

For now, I need to check how the usb port exposes the interface (one or two virtual serial ports?).

## Data transmission format
The data transmission format is defined in [the Specification](doc/Specification.md) attached to this repository.
