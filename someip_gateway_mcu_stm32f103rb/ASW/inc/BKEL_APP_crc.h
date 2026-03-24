/*
 * BKEL_APP_crc.h
 *
 *  Created on: Jan 4, 2026
 *      Author: sjkang
 */

#ifndef ASW_INC_BKEL_APP_CRC_H_
#define ASW_INC_BKEL_APP_CRC_H_

#define CRC8_POLY  0x07u

static inline uint8_t calc_crc8(const uint8_t *data, size_t len) {
	uint8_t crc = 0x00;

	    for (size_t i = 0; i < len; i++)
	    {
	        crc ^= data[i];

	        for (uint8_t bit = 0; bit < 8; bit++)
	        {
	            if (crc & 0x80)
	                crc = (crc << 1) ^ CRC8_POLY;
	            else
	                crc <<= 1;
	        }
	    }
	    return crc;
}


#endif /* ASW_INC_BKEL_APP_CRC_H_ */
