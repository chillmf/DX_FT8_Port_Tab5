/*
 * options.h
 *
 *  Created on: May 25, 2016
 *      Author: user
 */

#ifndef OPTIONS_H_
#define OPTIONS_H_

#include <stdint.h>

enum OptionNumber
{
    OPTION_Band_Index = 0,
    OPTION_Map_Index,
    NUM_OPTIONS
};

// Work with option data
int16_t Options_GetValue(int optionIdx);
void Options_SetValue(int optionIdx, int16_t optioValue);
void Options_Initialize(void);
void Options_StoreValue(int optionIdx);
void init_eeprom(void);

#endif /* OPTIONS_H_ */
