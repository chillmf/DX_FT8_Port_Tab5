#ifndef DISPLAY_
#define DISPLAY_

#include <stdint.h>
#include "math.h"
#include "WString.h" //This defines String data type

#define FFT_H  91
#define FFT_Resolution 6.25  //8000/2/1280

extern char current_message[];
void init_display (void);
void Display_WF(void);
void show_variable(uint16_t x, uint16_t y, int variable);
void show_short(uint16_t x, uint16_t y, uint8_t variable);
void show_decimal(uint16_t x, uint16_t y, float variable);
void draw_fft_spectrum(void);
void show_time(uint16_t x, uint16_t y);

void display_time(int x, int y);
void display_date(int x, int y);
void display_station_data(int x, int y);
void show_wide(uint16_t x, uint16_t y, int variable);

bool open_stationData_file(void);
void update_message_log_display(int mode);
void display_logged_messages(void);
void Be_Patient(void);

static int setup_password(const char *PASSWORD_part);
static int setup_WIFI_SSID(const char *SSID_part);
 void setup_RTC(void);






#endif /* DISPLAY_*/