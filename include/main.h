#ifndef MAIN_
#define MAIN_

#pragma once

#include "si5351.h"

extern int FT_8_counter;
extern int ft8_marker;
extern int WF_counter;
extern int xmit_flag;
extern int DSP_Flag;
extern char Station_Call[11];
extern char Station_Locator[7];
extern char Short_Station_Locator[5];
extern int decode_flag;
extern uint16_t cursor_line;
extern int Tune_On;
extern int master_decoded;
extern uint16_t cursor_freq;
extern int ft8_flag;
extern int ft8_xmit_counter;
extern int slot_state;
extern int target_slot;
extern bool free_text;
extern bool tx_pressed;
extern bool clr_pressed;
extern int log_display_flag;
extern uint32_t start_time, ft8_time;
extern void writeCodecRegister(uint16_t reg, uint16_t val);
extern	void m5_si5351_write_bulk(uint8_t addr, uint8_t start_reg, const uint8_t *data, size_t len);
extern	void m5_si5351_write(uint8_t addr, uint8_t reg, uint8_t data);
extern uint8_t m5_si5351_read(uint8_t addr, uint8_t reg);

void tx_display_update(void);


#endif /* MAIN_*/