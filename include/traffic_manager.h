#ifndef TRAFFIC_MANAGER_
#define TRAFFIC_MANAGER_

#pragma once

void set_FT8_Tone(uint8_t ft8_tone);
void setup_to_transmit_on_next_DSP_Flag(void);
void tune_On_sequence(void);
void tune_Off_sequence(void);
void service_QSO_mode(int decoded_signals);
void service_Beacon_mode(int decoded_signals);
void ft8_receive_sequence(void);
void ft8_transmit_sequence(void);
void terminate_QSO(void);
void set_Rcvr_Freq(void);

#endif /* TRAFFIC_MANAGER_ */