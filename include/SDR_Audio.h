#ifndef SDR_Audio_
#define SDR_Audio_
#include "math.h"

extern int DSP_Flag;
extern int attenuation;
void init_codec_driver(void);
void init_i2s_duplex(void);
void process_audio_stream(void);
void lineInLevel(uint8_t left, uint8_t right) ;
void process_xmit_message(void);

#endif /* SDR_Audio_ */