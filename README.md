Arduino DALI library for ATmega328P devices
===========================================

DALI master library derived from the [Cosino DALI2560  library](https://github.com/cosino/dali2560)

Supported Boards
----------------

Code has been tested on ATmega328P devices
* Arduino Uno
* Arduino Nano
* Arduino Pro Mini

Code is available for ATmega2560 devices but is not maintained

Support for other boards are depent on PCINT BUS configuration in dali.cpp ~ lines 260-350

Usage
-----

The library exposes a basic serial command interface to control the DALI bus.
Originally the library sourced from Cosino allowed for multiple bus's however modifications for this library has only been made to reference BUS 0.

If looking to incorporate this in a Wifi based project then an Arduino Nano could be paired with an ESP32 or ESP8266 and the Nano would act as a co-processor, receiving and transmitting between devices over the hardware serial ports.

Circuitry
---------

The circuit design for the DALI bus interface is derived from the attached schematic. Minor modifications were made based on components available to me.

* U1 & U2 Optocouplers  - TCLT10000 > PC817
* Q2 Transistor - MMBT2222A-TP > BC548
* D5 Bridge Rectifier - BGX 50AE6327 > 2W10

This is not my own design and I cannot recall where this was found. If someone knows where this is from or are the creators I will happily update this to provide credits.

<img src="https://github.com/paultbarrett/bitsDALI/raw/master/schematic.jpg">

Note that this circuit has been in use in my own house for over 1 year without any problems.

A external DALI power supply is used and all drivers are OSRAM OPPTOTRONIC.

Sample Program
--------------
```c
#include <Dali.h>

/*
 * Objects
 */

// Hardware Configuration
#define dali0_rx_pin 2 // UNO and Mini Pro Pin 2 | Arduino Mega Pin A8
#define dali0_tx_pin 4 // Uno and Mini Pro Pin 4 | Arudino Mega 22

int debugMode = 3;

Dali dali0;

void dali0_receiver(Dali *d, uint8_t *data, uint8_t len) {
  Serial.print("RX - ");
  unsigned int word = ((unsigned int)data[0] << 8) + data[1];
  Serial.println(word, BIN);
}

/*
 * Setup
 */
 
void setup()
{
  delay(500);
  Serial.begin(115200);
  
  delay(500);
  Serial.println("DALI testing program - ver. 1.00");
  
  delay(500);
  Serial.println("Init DALI");
  dali0.begin(dali0_tx_pin,dali0_rx_pin);

  dali0.EventHandlerReceivedData = dali0_receiver;
}

/*
 * Loop
 */

void loop()
{
  serialDali();
}
```
Additional Documentation
-------------


[commands.txt](https://github.com/paultbarrett/bitsDALI/blob/master/dali.txt) - The serial commands to interact with the interface

[dali.txt](https://github.com/paultbarrett/bitsDALI/blob/master/dali.txt) - DALI command specs
