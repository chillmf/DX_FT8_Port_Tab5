
#include <SPI.h>
#include "si5351.h"
#include "traffic_manager.h"
#include "display.h"
#include "decode_ft8.h"
#include "gen_ft8.h"
#include "button.h"
#include "main.h"
#include "SDR_Audio.h"

#define FT8_TONE_SPACING 625

static uint64_t F_Long, F_FT8, F_Receive;

extern Si5351 si5351;

static void set_Xmit_Freq(void)
{
  F_Long = (((uint64_t)start_freq * 1000 + (uint64_t)cursor_freq) * 100);
  si5351.set_freq(F_Long, SI5351_CLK0);
}

void tune_On_sequence(void)
{
  set_Xmit_Freq();
  set_Attenuator_Gain(0.05);
  attenuation = 1;
  delay(10);
  transmit_sequence();
  delay(10);
  si5351.output_enable(SI5351_CLK0, 1);
}

void tune_Off_sequence(void)
{
  si5351.output_enable(SI5351_CLK0, 0);
  delay(10);
  receive_sequence();
  delay(10);
  set_Attenuator_Gain(1.0);
  attenuation = 1;
}

void set_FT8_Tone(uint8_t ft8_tone)
{
  F_FT8 = F_Long + (uint64_t)ft8_tone * FT8_TONE_SPACING;
  si5351.set_freq(F_FT8, SI5351_CLK0);
}

void ft8_receive_sequence(void)
{
  si5351.output_enable(SI5351_CLK0, 0);
  delay(10);
  receive_sequence();
  delay(10);
  attenuation = 1;
}

void ft8_transmit_sequence(void)
{
  set_Xmit_Freq();
   si5351.set_freq(F_Long, SI5351_CLK0);
  attenuation = 1;
  delay(10);
  transmit_sequence();
  delay(10);
  si5351.output_enable(SI5351_CLK0, 1);
}

void setup_to_transmit_on_next_DSP_Flag(void)
{
  ft8_xmit_counter = 0;
  ft8_transmit_sequence();
  xmit_flag = 1;
}

void terminate_QSO(void)
{
  ft8_receive_sequence();
  receive_sequence();
}

void set_Rcvr_Freq(void)
{
  F_Receive = ((start_freq * 1000ULL - 10000ULL) * 100ULL * 4ULL);
  si5351.set_freq(F_Receive, SI5351_CLK1);
}
