# wireless-low-power-sensor-networks

This is the design and implementation for wireless low power sensor networks funded by DOE ARPA-E at the University of Utah.

This repo contains the implementation for a wireless protocol that is used for an ARPA-E wireless low-power sensor network project.

To see a more detailed, technical description of this protocol, go to: https://docs.google.com/document/d/12ByDM7kRo1fibv0Xf9CkuNfE5_rwe9qosZBLpxuLz6I/edit?usp=sharing

<br/>

## Installation Details
The Teensy requires a modified version of the RadioHead library to work correctly with the LoRa 1276/rfm95w module.
See: https://forum.pjrc.com/threads/41878-Probable-race-condition-in-Radiohead-library

The version of the RadioHead library used is included with the TeensyDuino install. The modification done is described in 
https://forum.pjrc.com/threads/41878-Probable-race-condition-in-Radiohead-library/page3 (top link attachement). 

### Installing The Modified Lbrary:
The library `radiohead_modified.zip` should be unzipped and the `RadioHead` folder should be placed here: `C:\Program Files (x86)\Arduino\hardware\teensy\avr\libraries\`.

In addition, make sure there is no RadioHead library install in `Documents\Arduino\libraries` as this will override any library in the Arduino folder.

Pinouts:\
Lora	Teensy\
CS	10\
RST	4\
EN 5\
MOSI	11\
MISO	12\
SCK	13\
G0	3\
