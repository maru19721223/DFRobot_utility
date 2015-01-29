/**************************************************************************/
/*! 
  @file     DF_GPS.cpp
  @author   lisper (leyapin@gmail.com, lisper.li@dfrobot.com)
  @license  LGPLv3 (see license.txt) 

  parse gps gpgga protocol

  Copyright (C) DFRobot - www.dfrobot.com
 */
/**************************************************************************/

#include <Arduino.h>
#include <DF_GPS.h>

#define _debug
//
uint8_t decToInt2 (char *the_buf) {
	uint8_t value = 0;
	value += (the_buf[0]-'0') *10;
	value += (the_buf[1]-'0');
	return value;
}

//
uint8_t hexToInt2 (char *the_buf) {
	uint8_t value = 0; 
	if (the_buf[0] >= '0' && the_buf[0] <= '9') {
		value += (the_buf[0] - '0') * 16;
	} else {
		value += (the_buf[0] - 'A' + 10) * 16;
	}

	if (the_buf[1] >= '0' && the_buf[1] <= '9') {
		value += (the_buf[1] - '0');
	} else {
		value += (the_buf[1] - 'A' + 10);
	}
	return value; 
}

DFGPS::DFGPS (Stream &theSerial) {
	_mySerial = &theSerial;
}

// check sum using xor
uint8_t DFGPS::gps_checksum (char *array) {
	uint8_t sum = array[1];
	for (uint8_t i=2; array[i] != '*'; i++) {
		sum ^= array[i];
	}
	return sum;
}

//get gga checksum
uint8_t DFGPS::gps_read_checksum (char **the_str) {
	char *temp = the_str[14];
	if (temp[0] != '*') {
		printf ("error no *");
		return 0;
	}
	uint8_t sum = hexToInt2 (temp+1);
	return sum;
}


//
uint8_t split_by_char (char *the_src, char the_char, char **the_des, uint8_t the_siz) {
	uint8_t src_len = strlen (the_src);
	uint8_t di=0;
	the_des[di++] = the_src;
	for (uint8_t si=0; si<src_len && di < the_siz; si++) {
		if (the_src[si] == the_char) {
			the_des[di++] = the_src+si+1;
			the_src[si] = '\0';
		}
	}
	return di;
}

//
uint8_t split_by_comma (char *the_src, char **the_des, uint8_t the_siz) {
	return split_by_char (the_src, ',', the_des, the_siz);
}

/*
   void DFGPS::gps_print_debug (char **the_strp, uint8_t the_leng) {
   for (uint8_t i=0; i<the_leng; i++) {
   printf ("%d:%d = %s\n", i, strlen (the_strp[i]), the_strp[i]);
   }
   printf ("----------------------\n");
   }
 */


uint8_t delete_crlf (char *the_buf) {
	uint8_t leng = strlen (the_buf);
	for (uint8_t i=0; i<leng-1; i++) {
		if (the_buf[i] == '\r' && the_buf[i+1] == '\n') {
			the_buf[i] = '\0';
			return 1;
		}
	}
	return 0;
}

void DFGPS::gps_print_gpgga (gpgga_s *my_gpgga) {
	printf ("now time =	%d:%d:%d\n", my_gpgga->utc.hour, my_gpgga->utc.minute, my_gpgga->utc.second);
	printf ("%c : %f\n", my_gpgga->ns, my_gpgga->longitude);
	printf ("%c : %f\n", my_gpgga->ew, my_gpgga->satellites);
	printf ("fix =	%d\nnum = %d\n", my_gpgga->fix, my_gpgga->num);
	printf ("hdop =	%f\n", my_gpgga->hdop);
	printf ("altitude =	%f %c\n", my_gpgga->altitude, my_gpgga->a_units);
	printf ("level =	%f %c\n", my_gpgga->level, my_gpgga->l_units);
}

//
void DFGPS::gpgga (gpgga_s *gpgga_data) {
	//split_by_comma (gps_buffer, gpsp, sizeof (gpsp)/4);	//!!! bug

	///////////////////////////////////
	gpgga_data->utc.hour = decToInt2 (gpsp[1]) + 8;
	gpgga_data->utc.minute = decToInt2 (gpsp[1]+2);
	gpgga_data->utc.second = decToInt2 (gpsp[1]+4);
	///////////////////////////////////
	gpgga_data->longitude = atof (gpsp[2]);
	gpgga_data->ns = gpsp[3][0];
	gpgga_data->satellites = atof (gpsp[4]);
	gpgga_data->ew = gpsp[5][0];
	///////////////////////////////////
	gpgga_data->fix = atoi (gpsp[6]);
	gpgga_data->num = atoi (gpsp[7]);
	gpgga_data->hdop = atof (gpsp[8]);
	gpgga_data->altitude = atof (gpsp[9]);
	gpgga_data->a_units = gpsp[10][0];
	gpgga_data->level = atof (gpsp[11]);
	gpgga_data->l_units = gpsp[12][0];
	free (gps_buffer);
	//gps_print_gpgga (gpgga_data);
}

int DFGPS::parse () {
	delete_crlf (gps_buffer);
	uint8_t sum = gps_checksum (gps_buffer);
	wordNum = split_by_comma (gps_buffer, gpsp, sizeof (gpsp)/sizeof (char*));
	uint8_t check_result = gps_read_checksum (gpsp);
	if (check_result != sum) {
		return 0;
	} else
		return 1;
}

void DFGPS::printGPGGA () {
	for (int i=0; i<wordNum; i++) {
		Serial.print (i);
		Serial.print (": ");
		Serial.println (gpsp[i]);
	}
}

int DFGPS::read () {
	if (_mySerial->available ()) {
		if (_mySerial->read () == '$') {
			gps_buffer[0] = '$';
			while (_mySerial->available () == 0);
			_mySerial->readBytes (gps_buffer+1, 5);
			if (strncmp (gps_buffer+1, "GPGGA", 5) == 0) {
				while (_mySerial->available () == 0);
				_mySerial->readBytesUntil ('$', gps_buffer+6, 100-6);
#ifdef _debug
				Serial.println (gps_buffer);
#endif
				////////////////////////////////////
				parse ();
				return 1;
			}
		}
	}
	return 0;
}

char DFGPS::fixc () {
	return gpsp[6][0];
}

uint8_t DFGPS::fix () {
	if (gpsp[6][0])
		return gpsp[6][0]-'0';
	else 
		return 0;
}

uint8_t DFGPS::getHour () {
	if (gpsp[1][0])
		return decToInt2 (gpsp[1]) + 8;
	else 
		return 0;
}
char *DFGPS::getTime () {
	return gpsp[1];
}

uint8_t DFGPS::getMinute () {
	if (gpsp[1][0])
		return decToInt2 (gpsp[1]+2);
	else 
		return 0;
}


uint8_t DFGPS::getSecond () {
	if (gpsp[1][0])
		return decToInt2 (gpsp[1]+4);
	else 
		return 0;
}


double DFGPS::getLongitude () {
	return atof (gpsp[2]);
}
char *DFGPS::getLongitudeStr () {
	return gpsp[2];
}

char DFGPS::getNS () {
	return gpsp[3][0];
}

char DFGPS::getEW () {
	return gpsp[5][0];
}

char *DFGPS::getSatellitesStr () {
	return gpsp[4];
}
double DFGPS::getSatellites () {
	return atof (gpsp[4]);
}

uint8_t DFGPS::getNum () {
	return atoi (gpsp[7]);
}
char *DFGPS::getNumStr () {
	return gpsp[7];
}

char *DFGPS::getHDOPStr () {
	return gpsp[8];
}
double DFGPS::getHDOP () {
	return atof (gpsp[8]);
}

char *DFGPS::getAltidudeStr () {
	return gpsp[9];
}
double DFGPS::getAltidude () {
	return atof (gpsp[9]);
}

char DFGPS::getAunits () {
	return gpsp[10][0];
}
char *DFGPS::getLevelStr () {
	return gpsp[11];
}

double DFGPS::getLevel () {
	return atof (gpsp[11]);
}

char DFGPS::getLunits () {
	return gpsp[12][0];
}
