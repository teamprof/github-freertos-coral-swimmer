## Introduction
Swimming can be challenging for people with visual impairments. They may have difficulty orienting themselves in the water and staying in their lane, without veer off course toward the lane line. They may also have difficulty knowing when they have reached the end of the pool.

We propose a Swimmer Assistive System that consists of computer vision equipment and a Bluetooth-enabled bone conduction earphone.

The computer vision equipment includes a Coral Micro Board and an ESP32 Dev Kit: the Coral Micro Board identifies the swimmer’s location, while the ESP32 Dev Kit that broadcasts audible feedback via Bluetooth.

The swimmer wears a bone conduction earphone, that is enabled with Bluetooth, to hear their location that is inferred by the computer vision equipment.

[![License: GPL v3](https://img.shields.io/badge/License-GPL_v3-blue.svg)](https://github.com/teamprof/freertos-coral-swimmer/blob/main/LICENSE)

<a href="https://www.buymeacoffee.com/teamprof" target="_blank"><img src="https://cdn.buymeacoffee.com/buttons/v2/default-yellow.png" alt="Buy Me A Coffee" style="height: 28px !important;width: 108px !important;" ></a>

---
## Swimmer Assistive System
```
                                  +------------+
                                  |  swimming  | (the cap prevent lost of earphone during swimming)
                                  |    cap     |
+-----------+                     +------------+           
| computer  |                     |    bone    |           
|  vision   |     (Bluetooth)     | conduction | 
| equipment |                     |  earphone  |            
+-----------+                     +------------+       

The computer vision equipment is installed somewhere above the swimming pool.
The bone conduction earphone is weared by the swimmer. A swimming cap is recommended to prevent lost of the earphone during swimming.
```

## System image 
## *** System image will be available in E/Nov ***


## Computer vision equipment
```
   +--------+
   | camera |
+--+--------+--+
|              |                          
| coral micro  |         +-------+         +-------+       
|              | - i2c - | level | - i2c - | esp32 |   
|              |  (1V8)  |shifter|  (3V3)  | a2dp  |        
+--------------+         +-------+         +-------+       
```
Firmware for the coral micro is on this github [freertos-coral-swimmer](https://github.com/teamprof/freertos-coral-swimmer)
Firmware for the esp32 is available here [esp32-a2dp-source](https://github.com/teamprof/esp32-a2dp-source)

---
## Hardware
The following components are required for this project:
1. [Coral Micro Board](https://coral.ai/docs/dev-board-micro/get-started/)
2. ESP32 Dev Kit V1 
3. Bluetooth enabled bone conduction earphone (A Bluetooth speaker is used for illustration/demonstration purpose)
4. I2C level shifter module
5. (optional) coral-esp32-adapter
[![coral-esp32-adapter](/doc/image/pcba.jpg)](https://github.com/teamprof/esp32-a2dp-source/blob/main/doc/image/pcba.jpg)

---
## Software 
1. Install [Coral Dev Board Micro](https://coral.ai/docs/dev-board-micro/get-started)
4. Download and extract this freertos-coral-swimmer code from github 
---

## LED
- Status LED: turn on when detected swimming pool
- User LED: turn on when detected swimmer
---

## Hardware connection between Coral Dev Board Micro and ESP32
```
+------------------------+     +---------------------+
| Coral Dev Board Micro  |     |       ESP32         |
+-------+----------------+     +------+--------------+
| GPIO  |   description  |     | GPIO |  description |
+-------+----------------+     +------+--------------+
| kSCL1 |   I2C1 SCL     |     |  22  |   I2C SCL    |
| kSDA1 |   I2C1 SDA     |     |  21  |   I2C SDA    |
+-------+----------------+     +------+--------------+
```


## I2C communication format
```
 command: IsA2dpConnected
                  +----------------+----------------+
                  |     byte 0     |     byte 1     |
                  +----------------+----------------+
 master to slave  |IsA2dpConnected |       0        |
                  +----------------+----------------+
 slave to master  |       0        |     result     |
                  +----------------+----------------+


command: PlaySound
                 +----------------+----------------+----------------+----------------+
                 |     byte 0     |     byte 1     |     byte 2     |     byte 3     |
                 +----------------+----------------+----------------+----------------+
master to slave  |   PlaySound    |     volume     |     sound      |        0       |
                 +----------------+----------------+----------------+----------------+
slave to master  |       0        |        0       |       0        |     result     |
                 +----------------+----------------+----------------+----------------+

```

## Flow of communication between Coral Dev Board Micro and ESP32
```
    coral                                 esp32
      |                                     |
      |   I2cCommand::IsA2dpConnected       |
      | ----------------------------------> |
      |                                     |
      |   I2cResponse::A2dpConnected or     |
      |   I2cResponse::A2dpDisconnected     |
      | <---------------------------------- |
      |                                     |
      |                                     |
      |       loop IsA2dpConnected          |
      |       until A2dpConnected           |
      |                                     |
      |                                     |
      |   I2cCommand::PlaySound             |
      |   param: <volumne> <sound>          |
      | ----------------------------------> |
      |                                     |
      |   I2cResponse::Success or           |
      |   I2cResponse::ErrorInvalidParam or |
      |   I2cResponse::ErrorDisconnected    |
      | <---------------------------------- |

    note:
    1. coral sends I2cCommand::PlaySound command every 0.5s
    2. coral re-start I2cCommand::IsA2dpConnected flow once "I2cResponse::ErrorDisconnected" is received
```

## Download, build and flash
## *** To be available soon ***



## Code explanation

### I2C address
The I2C slave address of ESP32 is defined in the macro "I2C_DEV_ADDR" in the file "./src/app/thread/ThreadReporter.cpp"
Here's an example of setting the I2C slave address to "0x55".
```
#define I2C_DEV_ADDR ((uint8_t)0x55)
```

```
### Please refer to source code for details
## *** source code will be available in E/Nov ***

---

## Demo
## *** Video demo will be available in E/Nov ***


---
### Debug
Enable or disable log be defining/undefining macro on "src/LibLog.h"

Debug is disabled by "#undef DEBUG_LOG_LEVEL"
Enable trace debug by "#define DEBUG_LOG_LEVEL Debug"

Example of LibLog.h
```
// enable debug log by defining the following macro
#define DEBUG_LOG_LEVEL Debug
// disable debug log by comment out the macro DEBUG_LOG_LEVEL 
// #undef DEBUG_LOG_LEVEL
```
---
### Troubleshooting
If you get compilation errors, more often than not, you may need to install a newer version of the coralmicro.

Sometimes, the project will only work if you update the board core to the latest version because I am using newly added functions.

---
### Issues
Submit issues to: [Coral swimmer assistance issues](https://github.com/teamprof/coral-swimmer/issues) 

---
### TO DO
1. Search for bug and improvement.
---

### Contributions and Thanks
Many thanks to the following authors who have developed great software and libraries.
1. [Hackster](https://www.hackster.io/contests/buildtogether)
2. [Coral team](https://coral.ai/about-coral/)

Many thanks for everyone for bug reporting, new feature suggesting, testing and contributing to the development of this project.
---

### Contributing
If you want to contribute to this project:
- Report bugs and errors
- Ask for enhancements
- Create issues and pull requests
- Tell other people about this library
---

### License
- The project is licensed under GNU GENERAL PUBLIC LICENSE Version 3
---

### Copyright
- Copyright 2023 teamprof.net@gmail.com. All rights reserved.



