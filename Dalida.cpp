/*
 * Copyright (C) 2020 Paul Barrett <pbarrett(at)bitsystems(dot)com(dot)au>
 *
 * This code is derived from https://github.com/cosino/dali2560.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "Dali.h"
#include <avr/eeprom.h>

void Dali_rx(Dali * d, uint8_t * data, uint8_t len);
void serialDali_rx(uint8_t errn, uint8_t * data, uint8_t n);
uint8_t exeCmd(char *msg);
uint8_t devCmd(char *msg);
uint8_t grpCmd(char *msg);
uint8_t busCmd(char *msg);
uint8_t rmpCmd(char *msg);
uint8_t cfgCmd(char *msg);
uint8_t tstCmd(char *msg);
uint8_t action(char *msg);
void storeSlaves(uint8_t * slaves);
void retrieveSlaves(Dali * dali, uint8_t * slaves);
int checkSlave(Dali * dali, uint8_t dev);

Dali *Master[2];
uint8_t bytes_rx;
char msg[10];
uint8_t rx_buf[100];	/* First byte used for length */
uint8_t rx_code;
enum rx_codes {STRING_CODING, BIN_CODING};

void serialDali(void) {
	uint8_t errn;

	if (Serial.available()) {
		msg[bytes_rx] = (char)Serial.read();
		if (msg[bytes_rx] == '\n') {
			if (msg[bytes_rx - 1] == '\r') { /* Adjustment protocol */
				msg[bytes_rx - 1] = '\n';
			}
			bytes_rx = 0;
			errn = exeCmd(msg);
			if (errn == 0) {
				serialDali_rx(0, rx_buf + 1, rx_buf[0]);
			}
			else if (errn != 0xFF) {
				/* 
				 * Is not an error
				 * Remap finished do not send anything
				 */
				serialDali_rx(errn, NULL, 0);
			}
		} else if (bytes_rx == 9) {
			bytes_rx = 0;
		} else {
			bytes_rx++;
		}
	}
}

void serialDali_rx(uint8_t errn, uint8_t * data, uint8_t n) {
	uint8_t buf[100];
	uint8_t n_char = 0;

	if (errn == 0) {
		buf[0] = 'O';
		if (rx_code == STRING_CODING) {
			for (int a = 0; a < n; a++) {
				n_char +=
				    sprintf((char *)buf + 1 + n_char, "%d",
					    (char)(data[a]));
			}
		} else if (rx_code == BIN_CODING) {
			for (int a = 0; a < n; a++) {
				buf[1 + a] = data[a];
			}
			n_char = n;
		}
		buf[1 + n_char] = '\r';
		buf[2 + n_char] = '\n';
		Serial.write(buf, n_char + 3);

	} else {
		switch (errn) {
		case 0x01:
			Serial.println(F("E01 - Invalid Command"));
			break;
		case 0x02:
			Serial.println(F("E02 - Command Not Implemented"));
			break;
		case 0x03:
			Serial.println(F("E03 - DALI BUS Busy"));
			break;
		case 0x20:
			Serial.println(F("E20 - Invalid BUS ID"));
			break;
		case 0x30:
			Serial.println(F("E30 - Invalid DEV Address"));
			break;
		case 0x90:
			Serial.println(F("E90 - Timeout"));
			break;
		case 0x99:
			Serial.println(F(" "));
			break;
		}
	}
}

void Dali_rx(Dali * d, uint8_t * data, uint8_t len) {
	serialDali_rx(0, data, 1);
}

uint8_t exeCmd(char *msg) {
	switch (msg[0]) {
	case 'd':
		return devCmd(msg);
		break;
	case 'g':
		return grpCmd(msg);
		break;
	case 'b':
		return busCmd(msg);
		break;
	case 'r':
		return rmpCmd(msg);
		break;
	case 'c':
		return cfgCmd(msg);
		break;
	case 't':
		return tstCmd(msg);
		break;
	case '?':
		Serial.println(F("========================== HELP =========================="));
		Serial.println(F("    d:Device | g:Group | b:Bus | r:Remap | c:Configure"));
		Serial.println(F("=========================================================="));
		return 0x99;
		break;
	}
	return 0x01;	/* Syntax error */
}

uint8_t devCmd(char *msg) {
	uint8_t bus, dev, arc;
	char str[] = "00";
	char str2[] = "000";
	uint8_t cfg_buf[5];

	if ((Master[0]->dali_status & 0x01) == 1) {
		return 0x03;
	}

	str[0] = msg[2];
	str[1] = msg[3];
	dev = (uint8_t) strtol(str, NULL, 10);

	str2[0] = msg[4];
	str2[1] = msg[5];
	str2[2] = msg[6];
	arc = (uint8_t) strtol(str2, NULL, 10);

	switch (msg[1]) {
	case '1':
		Master[0]->sendCommand(5, SINGLE, dev);	/* ON */
		rx_buf[0] = 0;
		return 0x00;

	case '0':
		Master[0]->sendCommand(0, SINGLE, dev);	/* OFF */
		rx_buf[0] = 0;
		return 0x00;

	case 'a':
		Master[0]->sendDirect(arc, SINGLE, dev);	/* Arc level */
		rx_buf[0] = 0;
		return 0x00;

	case 'i':

		// Return information for all active slaves
		if (msg[2] == 'a') {

			for(int i = 0;i < 64; i++) {

				int8_t data = 0;
				int dev = i;

				Master[0]->sendCommand(153, SINGLE, i);	/* Query Device Type */
				data = *(Master[0]->getReply());
				
				if (data > 0) {
					Master[0]->sendCommand(162, SINGLE, dev);	/* Query MIN LEVEL */
					cfg_buf[0] = *(Master[0]->getReply());
					Master[0]->sendCommand(161, SINGLE, dev);	/* Query MAX LEVEL */
					cfg_buf[1] = *(Master[0]->getReply());
					Master[0]->sendCommand(164, SINGLE, dev);	/* Query SYSFAIL LEVEL */
					cfg_buf[2] = *(Master[0]->getReply());
					Master[0]->sendCommand(163, SINGLE, dev);	/* Query POWERON LEVEL */
					cfg_buf[3] = *(Master[0]->getReply());
					Master[0]->sendCommand(165, SINGLE, dev);	/* Query SYSFAIL LEVEL */
					cfg_buf[4] = *(Master[0]->getReply());


					Serial.print(dev);  
					Serial.print(" ");Serial.print(cfg_buf[0], DEC);
					Serial.print(" ");Serial.print(cfg_buf[1], DEC);
					Serial.print(" ");Serial.print(cfg_buf[2], DEC);
					Serial.print(" ");Serial.print(cfg_buf[3], DEC);
					Serial.print(" ");Serial.print(cfg_buf[4], HEX);     
					Serial.println();

					delay(50);
				}

			}
			rx_buf[0] = 0;
			return 0x00;
		}

		// Return individual slave information

		Master[0]->sendCommand(162, SINGLE, dev);	/* Query MIN LEVEL */
		cfg_buf[0] = *(Master[0]->getReply());
		Master[0]->sendCommand(161, SINGLE, dev);	/* Query MAX LEVEL */
		cfg_buf[1] = *(Master[0]->getReply());
		Master[0]->sendCommand(164, SINGLE, dev);	/* Query SYSFAIL LEVEL */
		cfg_buf[2] = *(Master[0]->getReply());
		Master[0]->sendCommand(163, SINGLE, dev);	/* Query POWERON LEVEL */
		cfg_buf[3] = *(Master[0]->getReply());
		Master[0]->sendCommand(165, SINGLE, dev);	/* Query SYSFAIL LEVEL */
		cfg_buf[4] = *(Master[0]->getReply());


		Serial.print(dev);  
		Serial.print(" ");Serial.print(cfg_buf[0], DEC);
		Serial.print(" ");Serial.print(cfg_buf[1], DEC);
		Serial.print(" ");Serial.print(cfg_buf[2], DEC);
		Serial.print(" ");Serial.print(cfg_buf[3], DEC);
		Serial.print(" ");Serial.print(cfg_buf[4], HEX);     
		Serial.println();

		rx_buf[0] = 0;	
		return 0x00;

	case '?':
		Serial.println(F("================================= DEVICE HELP ================================"));
		Serial.println(F("1XX:ON | 0XX:OFF | aYYY:ARC | iXX:Info (Specific Slave) | ia:Info (All Slaves)"));
		Serial.println(F("               XX = Slave ID 0-63 | YYY = ARC Brightness 0-254                "));                
		Serial.println(F("=============================================================================="));
		return 0x99;
		break;
	}

	return 0x01;
}

uint8_t grpCmd(char *msg) {
	uint8_t grp, arc;
	char str[] = "00";
	char str2[] = "000";
	
	if ((Master[0]->dali_status & 0x01) == 1)
		return 0x03;

	str[0] = msg[2];
	str[1] = msg[3];
	grp = (uint8_t) strtol(str, NULL, 10);

	str2[0] = msg[4];
	str2[1] = msg[5];
	str2[2] = msg[6];
	arc = (uint8_t) strtol(str2, NULL, 10);

	switch (msg[1]) {
	case '1':
		Master[0]->sendDirect(5, GROUP, grp);	/* ON */
		rx_buf[0] = 0;
		return 0x00;

	case '0':
		Master[0]->sendDirect(0, GROUP, grp);	/* OFF */
		rx_buf[0] = 0;
		return 0x00;

	case 'a':
		//arc = (uint8_t) strtol(msg + 4, NULL, 16);
		Master[0]->sendDirect(arc, GROUP, grp);	/* Arc level */
		rx_buf[0] = 0;
		return 0x00;

	case '?':
		Serial.println(F("================================= GROUP HELP ================================="));
		Serial.println(F("                          1XX:ON | 0XX:OFF | aYYY:ARC                         "));
		Serial.println(F("               XX = Slave ID 0-63 | YYY = ARC Brightness 0-254                "));                
		Serial.println(F("=============================================================================="));
		return 0x99;
		break;
	}

	return 0x01;
}

uint8_t busCmd(char *msg) {
	uint8_t grp, arc;
	char str[] = "00";
	int i, j;
	uint8_t _slaves[8];

	str[0] = msg[2];
	str[1] = msg[3];
	str[2] = msg[4];
	arc = (uint8_t) strtol(str, NULL, 10);

	if ((Master[0]->dali_status & 0x01) == 1) {
		return 0x03;
	}
	switch (msg[1]) {
	case '1':
		Master[0]->sendCommand(5, BROADCAST, grp);	/* ON */
		rx_buf[0] = 0;
		return 0x00;

	case '0':
		Master[0]->sendCommand(0, BROADCAST, grp);	/* OFF */
		rx_buf[0] = 0;
		return 0x00;

	case 'a':
		//arc = (uint8_t) strtol(msg + 3, NULL, 10);
		Master[0]->sendDirect(arc, BROADCAST, grp);	/* Arc level */
		rx_buf[0] = 0;
		return 0x00;

	case 'l':

		retrieveSlaves(Master[0], _slaves);
		rx_buf[0] = 64;
		for (i = 0; i < 8; i++)
			for (j = 0; j < 8; j++)
				rx_buf[1 + i * 8 + j] =
				    ((_slaves[i] & (1 << j)) ? 1 : 0);
		rx_code = STRING_CODING;
		return 0x00;

	case 's':
		Master[0]->list_dev();	/* Scan devices */

		retrieveSlaves(Master[0], _slaves);
		rx_buf[0] = 64;
		for (i = 0; i < 8; i++)
			for (j = 0; j < 8; j++)
				rx_buf[1 + i * 8 + j] =
				    ((_slaves[i] & (1 << j)) ? 1 : 0);
		rx_code = STRING_CODING;
		return 0x00;

	case '?':
		Serial.println(F("================================== BUS HELP =================================="));
		Serial.println(F("         1XX:ON | 0XX:OFF | aYYY:ARC | l:List Slaves | s:Scan Slaves          "));
		Serial.println(F("               XX = Slave ID 0-63 | YYY = ARC Brightness 0-254                "));                
		Serial.println(F("=============================================================================="));
		return 0x99;
		break;
	}

	return 0x01;
}

uint8_t rmpCmd(char *msg) {
	uint8_t dev;
	uint8_t newdev;
	char str[] = "00";
	char str2[] = "00";

	// Device Address
	str[0] = msg[2];
	str[1] = msg[3];
	//dev = (uint8_t) strtol(str, NULL, 16);
	dev = (uint8_t) strtol(str, NULL, 10);

	// New Device Address
	str2[0] = msg[4];
	str2[1] = msg[5];
	//newdev = (uint8_t) strtol(str2, NULL, 16);
	newdev = (uint8_t) strtol(str2, NULL, 10);

	// Catch invalid initial device short address
	if (dev < 0 || dev > 63 || newdev < 0 || newdev > 63) {
		return 0x30;
	}

	switch (msg[1]) {
	case 'a':	/* Remap All */
		if ((Master[0]->dali_status & 0x01) == 1) {
			return 0x03;
		}
		serialDali_rx(0, NULL, 0);
		dev_found = 0;
		if (Master[0] != NULL) {
			Master[0]->remap(ALL);
		}
		return 0xFF;	/* Remap finished. Do not send anything. */

	case 'u':	/* Remap all unknown devices */
		if ((Master[0]->dali_status & 0x01) == 1) {
			return 0x03;
		}
		serialDali_rx(0, NULL, 0);
		dev_found = 0;
		if (Master[0] != NULL) {
			Master[0]->remap(MISS_SHORT);
		}
		return 0xFF;	/* Remap finished. Do not send anything. */

	case 'p':	/* Check Remap status */
		if ((Master[0]->dali_status & 0x01) == 1) {
			rx_buf[1] = (dev_found * 100) / 64;
		} else {
			rx_buf[1] = 100;
		}
		rx_buf[0] = 1;
		rx_code = STRING_CODING;
		return 0x00;

	case 's':	/* Static Remap with specific starting short address Rs{XX} */
		if ((Master[0]->dali_status & 0x01) == 1) {
			return 0x03;
		}
		serialDali_rx(0, NULL, 0);
		dev_found = 0;
		if (Master[0] != NULL) {
			Master[0]->remapStatic(dev,ALL);
		}
		return 0xFF;	/* Remap finished. Do not send anything. */

	case 'm':	/* Remap a short address to a new one */
		if ((Master[0]->dali_status & 0x01) == 1) {
			return 0x03;
		}
		serialDali_rx(0, NULL, 0);
		if (Master[0] != NULL) {
			Master[0]->remapMove(dev,newdev,ALL);
		}
		return 0xFF;	/* Remap finished. Do not send anything. */

	case 'A':	/* Remap Abort */
		if (Master[0] != NULL) {
			Master[0]->abort_remap();
		}
		rx_buf[0] = 0;
		return 0x00;

	case '?':
		Serial.println(F("=============================================== REMAP HELP ================================================"));
		Serial.println(F(" a:Remap All | u:Remap Unknown | p:Remap Progress | sXX:Remap All From | mYYZZ:Scan Slaves | A:Abort Remap "));
		Serial.println(F("                    XX = Start Slave ID 0-63 | YY = Current Slave ID | ZZ = New Slave ID                   "));                
		Serial.println(F("==========================================================================================================="));
		return 0x99;
		break;
	}

	return 0x01;
}

uint8_t cfgCmd(char *msg) {
	uint8_t dev, data;
	char str[] = "00";
	char str2[] = "000";

	// Check BUS State
	if ((Master[0]->dali_status & 0x01) == 1) {
		return 0x03;
	}

	// Extract device ID
	str[0] = msg[2];
	str[1] = msg[3];
	dev = (uint8_t) strtol(str, NULL, 10);

	// Extract data
	str2[0] = msg[4];
	str2[1] = msg[5];
	str2[2] = msg[6];
	data = (uint8_t) strtol(str2, NULL, 10);

	switch (msg[1]) {
	case 'm': 
		
		// Set data in DTR on bus
		Master[0]->sendExtCommand(257, data);

		// Store DTR as MIN LEVEL
		Master[0]->sendCommand(43, SINGLE, dev);
 		
		rx_buf[0] = 0;
		return 0x00;
	
	case 'x': 
		
		// Set data in DTR on bus
		Master[0]->sendExtCommand(257, data);

		// Store DTR as MAX LEVEL
		Master[0]->sendCommand(42, SINGLE, dev);

		rx_buf[0] = 0;
		return 0x00;

	case 'f': 
		
		// Set data in DTR on bus
		Master[0]->sendExtCommand(257, data);

		// Store DTR as SYS FAIL LEVEL
		Master[0]->sendCommand(44, SINGLE, dev);

		rx_buf[0] = 0;
		return 0x00;

	case 'p': 
		
		// Set data in DTR on bus
		Master[0]->sendExtCommand(257, data);

		// Store DTR as PWR ON LEVEL
		Master[0]->sendCommand(45, SINGLE, dev);

		rx_buf[0] = 0;
		return 0x00;

	case 't': 
		
		// Set data in DTR on bus
		Master[0]->sendExtCommand(257, data);

		// Store DTR as FADE TIME
		Master[0]->sendCommand(46, SINGLE, dev);

		rx_buf[0] = 0;
		return 0x00;

	case 'r': 
		
		// Set data in DTR on bus
		Master[0]->sendExtCommand(257, data);

		// Store DTR as FADE RATE
		Master[0]->sendCommand(47, SINGLE, dev);

		rx_buf[0] = 0;
		return 0x00;

	case 'z': 
		
		// Set data in DTR on bus
		Master[0]->sendExtCommand(257, data);

		// Reset driver
		Master[0]->sendCommand(32, SINGLE, dev);

		rx_buf[0] = 0;
		return 0x00;

	case '?':
		Serial.println(F("====================================================== CONFIG HELP ======================================================="));
		Serial.println(F(" mXXYYY:MIN Level | xXXYYY:MAX Level | fXXYYY:SYSFAIL Level | pXXYYY:PWRFAIL Level | tXXYYY:Fade Time | rXXYYY:Fade Rate "));
		Serial.println(F("                                                   zXX:Reset Driver                                                      "));     
		Serial.println(F("                                          XX = Start Slave ID 0-63 | YYY = Value                                         "));                
		Serial.println(F("========================================================================================================================="));
		return 0x99;
		break;
	}

	 // E01 - Invalid Command
	return 0x01;
}

uint8_t tstCmd(char *msg) {
	uint8_t dev, arc;
	char str[] = "00";


	// Check BUS State
	if ((Master[0]->dali_status & 0x01) == 1) {
		return 0x03;
	}

	str[0] = msg[2];
	str[1] = msg[3];
	//dev = (uint8_t) strtol(str, NULL, 16);
	dev = (uint8_t) strtol(str, NULL, 10);

	switch (msg[1]) {
	case '1':
		// E02 - Command Not Implemented
		return 0x02;
	}
	 // E01 - Invalid Command
	return 0x01;
}

void storeSlaves(uint8_t * slaves) {
	int base_addr = 0x0000;

	for (int i = 0; i < 8; i++) {
		eeprom_write_byte((unsigned char *)(base_addr + i), slaves[i]);
	}
}

void retrieveSlaves(Dali * dali, uint8_t * slaves) {
	int base_addr = 0x0000;

	for (int i = 0; i < 8; i++) {
		slaves[i] = eeprom_read_byte((unsigned char *)(base_addr + i));
		dali->slaves[i] = slaves[i];
	}
}

int checkSlave(Dali * dali, uint8_t dev) {
	uint8_t i, j, slaves[8];

	i = dev / 8;
	j = dev % 8;
	retrieveSlaves(dali, slaves);

	if ((slaves[i] & (1 << j)) == 1) {
		return 0;
	} else {
		return -1;
	}
}
