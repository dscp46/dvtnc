# AX.25 over D-Star
This document describes the transport of AX.25 Frames over the D-Star simple data communication channel.

## Format of AX.25 frames
Prior to transmission, in this document, AX.25 frames *MUST* be considered in their raw form.
In particular, the following steps, specified in [AX.25 specification version 2.2](https://tarpn.net/t/faq/files/AX25.2.2-Sep%2017-1-10Sep17.pdf) *MUST NOT* be applied:
  * Insertion of the AX.25 Frame Check Sequence
  * Bit stuffing
  * Insertion of the flag byte (`0x7E`)

### Frame Check Sequence
To detect in-transit frame corruption, a 32-bit Frame Check Sequence is calculated by both the sender and the receiver of a frame.

The Frame Check Sequence is calculated in accordance with recommandations in the HDLC reference document, ISO 3309.

The Frame Check Sequence is transmitted with respect to the little-endian byte order (that is, in opposition to the network byte order).

### Frame encoding, escaping
Some byte values are prohibited on the D-Star simple data communication channel, and *MUST* be escaped (see [section 6.2](https://www.jarl.com/d-star/STD7_0.pdf) ).

To ensure transmitted frames comply with this requirement, frames are encoded using the following parameters:
  * Start of frame marker: `0xE1`
  * End of frame Marker: `0xE0`
  * Escape character: `0x3D` (ASCII character '=')
  * Forbidden characters: `0x00`, `0x11`, `0x13`, `0x1A`, `0x24`, `0xCB`, `0xFD`, `0xFE`, `0xFF`, on top of aforementioned characters.
  * Escape Offset: `+64`. If the escaped value exceeds 255 (`0xFF`), the carry bit is discarded, such that the result stays a 8-bit value. Reciprocally, when unescaping an escaped value, the borrow bit is discarded, such the result stays a 8-bit value.
  * Maximum unescaped packet size: at least 557 bytes (512 bytes payload, 17 bytes for AX.25 2.2 in Extended Asynchronous Balanced Mode, 28 bytes for an AXSec Authentication Header).

Here is the justification for the various forbidden characters:
  * `0x00`: Mandated by section 6.2 (簡易データ通信).
  * `0x11`: Software flow control Xoff
  * `0x13`: Software flow control Xon
  * `0x24`: Escaped to avoid interaction with the D-PRS logic (ASCII '$').
  * `0xCB`: [D-Star specification](https://www.jarl.com/d-star/STD7_0.pdf) references a "packet loss" padding pattern, which contains the sequence `E7 84 76` in the data part of a 20ms frame (see section 6.6). `0x84` is forbidden to avoid reproducing that pattern (in the second part of a S-Frame). When unscrambled, the banned value becomes `0xCB`. Escaping that single value is sufficient to break the loss pattern.
  * `0xFD` to `0xFF`: CI-V Command words.

## Selection of transmission speed
In a packet session, parties are defined as the Initiator and the Responder.

The Initiator is generally an end user. The Responder can be an end user, or a Service Gateway, behind a repeater. Both the latter functions can be hosted by the same equipment.

When possible, the Initiator *SHOULD* prefer the use of DV Fast Data instead of DV Slow Data for transmission.

When initialized, the Responder *MUST* infer the Initiator will use DV Slow Data. If DV Fast Data is received from the Initiator, the Responder *SHOULD* to DV Fast Data for the rest of the session.

## Test vectors
### Frame 1
Raw AX.25 frame:
`8c 68 90 9e 8c 40 e0 8c 62 b4 b2 82 40 75 10 f0 8e fd 6e 1b`

Raw AX.25 frame with CRC:
`8c 68 90 9e 8c 40 e0 8c 62 b4 b2 82 40 75 10 f0 8e fd 6e 1b bd 85 61 49`

Encoded AX.25 frame:
`e1 8c 68 90 9e 8c 40 3d 20 8c 62 b4 b2 82 40 75 10 f0 8e 3d 3d 6e 1b bd 85 61 49 e0`

### Frame 2
Raw AX.25 frame:
`8c 68 90 9e 8c 40 f8 8c 68 90 9e 8c 40 61 62 f0 5b 52 4d 53 20 45 78 70 72 65 73 73 2d 31 2e 37 2e 32 31 2e 30 2d 42 32 46 48 4d 24 5d 0d 0a`

Raw AX.25 frame with FCS:
`8c 68 90 9e 8c 40 f8 8c 68 90 9e 8c 40 61 62 f0 5b 52 4d 53 20 45 78 70 72 65 73 73 2d 31 2e 37 2e 32 31 2e 30 2d 42 32 46 48 4d 24 5d 0d 0a dc e2 cf 8f`

Encoded AX.25 frame:
`e1 8c 68 90 9e 8c 40 f8 8c 68 90 9e 8c 40 61 62 f0 5b 52 4d 53 20 45 78 70 72 65 73 73 2d 31 2e 37 2e 32 31 2e 30 2d 42 32 46 48 4d 3d 64 5d 0d 0a dc e2 cf 8f e0`
