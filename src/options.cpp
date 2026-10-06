
#include <Arduino.h>
#include <M5Unified.h>
#include <EEPROM.h>
#include <SPI.h>
#include "si5351.h"
#include "display.h"
#include "options.h"
#include "button.h"
#include "stdio.h"

#define sentinel 1948 // 1037, 1945, 1066,
#define EEPROM_SIZE 64

void init_eeprom(void) {
    EEPROM.begin(EEPROM_SIZE);
    
}

struct OptionStruct
{
  const char *Name;
  const int16_t Initial;
  const int16_t Minimum;
  const int16_t Maximum;
  const int16_t ChangeUnits;
  int16_t CurrentValue;
};

// Order must match OptionNumber in options.h
OptionStruct s_optionsData[] = {
    {
        /*Name*/ "  Band_Index ", // opt0
        /*Init*/ _20M,
        /*Min */ 0,
        /*Max */ 6,
        /*Rate*/ 1,
        /*Data*/ 0,
    },

    {
        /*Name*/ "  Map_Index ", // opt0
        /*Init*/ 0,
        /*Min */ 0,
        /*Max */ 2,
        /*Rate*/ 1,
        /*Data*/ 0,
    }};

// Work with option data
int16_t Options_GetValue(int optionIdx)
{
  return s_optionsData[optionIdx].CurrentValue;
}

void Options_SetValue(int optionIdx, int16_t newValue)
{
  s_optionsData[optionIdx].CurrentValue = newValue;
}

static void Options_ResetToDefaults(void)
{
  for (int i = 0; i < NUM_OPTIONS; i++)
  {
    Options_SetValue(i, s_optionsData[i].Initial);
  }
}



// Initialization
void Options_Initialize(void)
{

  if (EEPROM.readShort(10) == sentinel)
  {
    s_optionsData[0].CurrentValue = EEPROM.readShort(20);
    s_optionsData[1].CurrentValue = EEPROM.readShort(30);

    
  }
  else
  {
    EEPROM.writeShort(10, sentinel);
    Options_ResetToDefaults();
    EEPROM.writeShort(20, s_optionsData[0].Initial);
    EEPROM.writeShort(30, s_optionsData[1].Initial);

  }

  BandIndex = Options_GetValue(0);

  start_freq = sBand_Data[BandIndex].Frequency;
  show_wide(320, 120, start_freq);
  Map_Index = Options_GetValue(1);
}

void Options_StoreValue(int optionIdx)
{
  int16_t option_value = Options_GetValue(optionIdx);

  switch (optionIdx)
  {
  case 0:
    EEPROM.writeShort(20, option_value);
    break;
  case 1:
    EEPROM.writeShort(30, option_value);
    break;
  }
}
