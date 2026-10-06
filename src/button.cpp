#include <Arduino.h>
#include <M5Unified.h>
#include <M5GFX.h>
#include <Wire.h>
#include "si5351.h"
#include "main.h"
#include "traffic_manager.h"
#include "Process_DSP.h"
#include <SPI.h>
#include <EEPROM.h>
#include <Wire.h>

#include "button.h"
#include "Display.h"
#include "gen_ft8.h"
#include "traffic_manager.h"
#include "decode_ft8.h"
#include "Process_DSP.h"
#include "options.h"
#include "ADIF.h"
#include "main.h"
#include "PskInterface.h"
#include "autoseq_engine.h"

#define RxSw_PIN 6

#define CQSOTA 16
#define CQPOTA 17
#define QRPP 18
#define StandardCQ 15
#define FreeText1 19
#define FreeText2 20
#define CQFree 4

#define USB 2

Si5351 si5351;

const int button_delay = 5;

uint16_t draw_x, draw_y, touch_x, touch_y;


#define ft8_shift 6.25

uint16_t display_cursor_line;

uint8_t RX_volume;
int RF_Gain;

int FT8_Message_Touch;

int Beacon_On;
int Auto_Sync;
int Auto_QSO;

uint16_t start_freq;
int Arm_Tune;
int Clock_Correction;
int32_t cal_factor = 2000;
int bfo_offset;
int BandIndex;
int Band_Minimum;
int QSO_Fix;
int CQ_Mode_Index;
int Free_Index;
int Map_Index;
int Skip_Tx1;

uint16_t tx, ty;
uint32_t last_touch;


FreqStruct sBand_Data[NumBands] = {

    {7074,
     "7074"},

    {10136,
     "10136"},

    {14074,
     "14074"},

    {18100,
     "18100"},

    {21074,
     "21074"},

    {24915,
     "24915"},

    {28074,
     "28074"}};


void transmit_sequence(void)
{
  digitalWrite(RxSw_PIN, HIGH);
  delay(10);
}

void receive_sequence(void)
{
  digitalWrite(RxSw_PIN, LOW);
  delay(10);
}

void set_RF_Gain(int rfgain)
{


}

void set_Attenuator_Gain(float att_gain)
{

}

void terminate_transmit_armed(void)
{
  ft8_receive_sequence();

}

int testButton(uint8_t index)
{
  if ((draw_x > sButtonData[index].x) && (draw_x < sButtonData[index].x + sButtonData[index].w) && (draw_y > sButtonData[index].y) && (draw_y <= sButtonData[index].y + sButtonData[index].h))
  {
    return 1;
  }
  else
    return 0;
}

int FT8_Touch(int draw_x, int draw_y)
{
  int y_test;

  if (draw_x < 300 && (draw_y > 200 && draw_y < 700) && Beacon_On == 0)
  {
    y_test = draw_y - 200;

    FT_8_TouchIndex = y_test / LINE_HT;
    return 1;
  }
  else
    return 0;
}

int Xmit_message_Touch(int draw_x, int draw_y)
{
  return ((draw_x > 400 && draw_x < 640) && (draw_y > 380 && draw_y < 550));
}

void check_WF_Touch(int draw_x , int draw_y )
{
  if (draw_x < 600 && draw_y < 90)
  {
    display_cursor_line = draw_x;
    cursor_line = display_cursor_line / 2;
    cursor_freq = (uint16_t)((float)(cursor_line + ft8_min_bin) * ft8_shift);
    show_variable(cursor_freq_X , cursor_freq_line, cursor_freq);
  }
}

void set_startup_freq(void)
{
  display_cursor_line = 224;
  cursor_line = display_cursor_line / 2;
  cursor_freq = (uint16_t)((float)(cursor_line + ft8_min_bin) * ft8_shift);
  show_variable(cursor_freq_X , cursor_freq_line, cursor_freq);
}


void display_Free_Text(void)
{

  
  M5.Display.setTextSize(3);
  M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);

  M5.Display.setCursor(100, line4 + 20);
  M5.Display.write(Free_Text1, 14);

  M5.Display.setCursor(100, line5 + 20);
  M5.Display.write(Free_Text2, 14);
  
}

void reset_buttons(int btn1, int btn2, int btn3, const char *button_text)
{
  sButtonData[btn1].state = 0;
  drawButton(btn1);
  sButtonData[btn2].state = 0;
  drawButton(btn2);
  sButtonData[btn3].state = 0;
  drawButton(btn3);
  sButtonData[4].text0 = (char *)button_text;
  drawButton(4);
}

void update_CQFree_button()
{
  sButtonData[CQFree].state = 0;
  drawButton(CQFree);
}

void erase_Cal_Display(void)
{
  clear_reply_message_box();
  M5.Display.fillRect(0, 100, 600, 280, TFT_BLACK); // move tune to left hand pane
  erase_CQ();
  for (int i = 11; i < 23; i++)
    sButtonData[i].Active = 0;
}

void EEPROMWriteInt(int address, int value)
{
  uint16_t internal_value = 32768 + value;

  byte byte1 = internal_value >> 8;
  byte byte2 = internal_value & 0xFF;
  EEPROM.write(address, byte1);
  EEPROM.write(address + 1, byte2);
}

int EEPROMReadInt(int address)
{
  uint16_t byte1 = EEPROM.read(address);
  uint16_t byte2 = EEPROM.read(address + 1);
  uint16_t internal_value = (byte1 << 8 | byte2);

  return (int)internal_value - 32768;
}

void Set_Cursor_Frequency(void)
{
  cursor_freq = (uint16_t)((float)(cursor_line + ft8_min_bin) * ft8_shift);

  show_variable(670, cursor_freq_line, cursor_freq);
}

const uint64_t F_boot = 11229600000ULL;

void start_Si5351(void)
{
  si5351.init(SI5351_CRYSTAL_LOAD_0PF, 26000000, 0);
  si5351.drive_strength(SI5351_CLK0, SI5351_DRIVE_8MA);
  si5351.drive_strength(SI5351_CLK1, SI5351_DRIVE_2MA);
  si5351.drive_strength(SI5351_CLK2, SI5351_DRIVE_2MA);
  si5351.set_freq(F_boot, SI5351_CLK1);
  delay(10);
  si5351.output_enable(SI5351_CLK1, 1);
  delay(20);
  set_Rcvr_Freq();
}


void init_RxSW(void){
    pinMode(RxSw_PIN, OUTPUT);
    digitalWrite(RxSw_PIN, LOW); // Start with the relay open/de-energized
    Serial.println("RxSw_PIN");
}

void FT8_Sync(void)
{
	start_time = millis();
	ft8_flag = 1;
	FT_8_counter = 0;
	ft8_marker = 1;
   WF_counter = 0;
}