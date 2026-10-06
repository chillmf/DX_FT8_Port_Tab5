#ifndef PROCESS_DSP_
#define PROCESS_DSP_

#include "math.h"
#define FFT_SIZE  2048
#define input_gulp_size 1024
#define ft8_buffer_size 348 //arbitrary for 3 kc
#define ft8_min_bin 48


extern int16_t FT8_Data[2048 / 2];
extern uint8_t FFT_Buffer[ft8_buffer_size];
extern int ft8_flag, FT_8_counter, ft8_marker;
#define ft8_msg_samples 91

extern uint8_t export_fft_power[ft8_msg_samples * ft8_buffer_size * 4];

void init_DSP(void);
void process_FT8_FFT(void);

static void compute_radix2_fft(float* real, float* imag, int n);
void extract_power(int offset);

#endif /* PROCESS_DSP */