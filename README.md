# k-bus_reader
Small skelton program for Arduino Nano Every to read BMW K-BUS data.

## Required items
No K-Bus specific IC needed. Just Arduino Nano Every board and some common electronic components are necessary.
- Arduino Nano Every
- USB cable to connect PC to the Arduino board
- Resistance and diode for converting voltage level from 12V to 5V
- Miscellaneous like lead wire and tools for wiring.

## Setup
Below is just an example to get K-Bus signal input to Arduino Rx pin while dropping voltage to 5V.
![Wiring](https://github.com/hismat/k-bus_reader/blob/main/Wiring.png?raw=true)
  
## About code
k-bus_reader is running a small statemachine with three state; unknown, idle and occupied which represent the bus state.
The program starts with unknown state as it cannot tell whether the program started in the middle of K-Bus message transmission or not.
If it was confirmed that there has been no signal was exchanged over the bus for pre-defined period of time (50 ms), k-bus_reader takes it as the bus is currently in idle state.
Then, the state transits to occupied state when new byte available in the serial buffer. The new byte is taken as the source address, the first byte of K-Bus message.
After all bytes of the K-Bus meesage were read, k-bus_reader checks the checksum calculation result and the state goes back to idle.

## K-Bus message
First byte: Source address
Second byte: Message length (length is counted from the third byte to the last byte)
Third byte: Destination address
Fourth byte to second last byte: Payload data
Last Byte: Check sum
