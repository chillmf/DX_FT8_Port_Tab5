#include <Arduino.h>
#include <M5Unified.h>
#include <M5GFX.h>
#include "Process_DSP.h"
#include "Display.h"
#include "WF_Table.h"
#include <math.h>
#include "string.h"
#include "math.h"

#include "decode_ft8.h"
#include "ADIF.h"
#include "button.h"
#include "main.h"
#include "gen_ft8.h"
#include "ini.h"
#include "autoseq_engine.h"
#include <SPI.h>
#include <SD.h>
#include <IniFile.h>


#define FFT_X 0
#define FFT_Y 1
#define FFT_W (ft8_buffer_size - ft8_min_bin)
#define TX_X 240

#define SCREEN_WIDTH  M5.Display.width()
#define SCREEN_HEIGHT M5.Display.height()

 //char WIFI_PASSWORD[]  = "politetrain921";
 //char WIFI_SSID[] =      "NETGEAR6567890"; 

 char WIFI_PASSWORD[20];
 char WIFI_SSID[20]; 

 #define NTP_TIMEZONE  "JST-9"
 #define NTP_SERVER1   "0.pool.ntp.org"
 #define NTP_SERVER2   "1.pool.ntp.org"
 #define NTP_SERVER3   "2.pool.ntp.org"

#if defined ( ARDUINO )
#if __has_include(<WiFi.h>)
 #include <WiFi.h>
#endif


// Different versions of the framework have different SNTP header file names and availability.
 #if __has_include (<esp_sntp.h>)
  #include <esp_sntp.h>
  #define SNTP_ENABLED 1
 #elif __has_include (<sntp.h>)
  #include <sntp.h>
  #define SNTP_ENABLED 1
 #endif

#endif

#ifndef SNTP_ENABLED
#define SNTP_ENABLED 0
#endif


 File stationData_File;

static const int max_log_messages = 9;
display_message_details log_messages[max_log_messages];
char current_message[22];

static int old_rtc_hour = -1;


int FT_8_TouchIndex;
//uint16_t cursor = 192;
int FT8_Touch_Flag;
static uint8_t WF_Bfr[FFT_H * FFT_W];


const int max_noise_count = 3;
const int max_noise_free_sets_count = 3;
static int noise_free_sets_count = 0;

extern uint32_t dsp_current_time, dsp_old_time, dsp_start_time, dsp_flag_time;


void init_display (void) {

    M5.Display.setRotation(0);
    M5.Display.setTextColor(TFT_GREEN, TFT_BLACK);
    M5.Display.setTextSize(3);

}

void display_time(int x, int y) {

	auto dt = M5.Rtc.getDateTime();
	char string[11]; // print format stuff
	sprintf(string, "%02d/%02d/%02d \r\n"
	
               , dt.time.hours
               , dt.time.minutes
               , dt.time.seconds
               );

	M5.Display.setTextSize(3);
	M5.Display.setTextColor(TFT_YELLOW, TFT_BLACK);
	M5.Display.setCursor(x, y);
	M5.Display.print(string);

	
		if (dt.time.hours < old_rtc_hour)
		{
		Init_Log_File();
		//display_date(650, 30);
		//clear_auto_memories();
		}
  	old_rtc_hour = dt.time.hours;

}

void display_date(int x, int y) {

	auto dt = M5.Rtc.getDateTime();
	char string[30]; // print format stuff
	sprintf(string, "%04d/%02d/%02d  \r\n"
		
               , dt.date.year
               , dt.date.month
               , dt.date.date
               );

			  M5.Display.setTextSize(3);
				M5.Display.setTextColor(TFT_YELLOW, TFT_BLACK);
				M5.Display.setCursor(x, y);
			  M5.Display.print(string);

	}



void show_variable(uint16_t x, uint16_t y, int variable)
{
	char string[5]; // print format stuff
	sprintf(string, "%4i", variable);
	M5.Display.setTextSize(3);
	M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
	M5.Display.setCursor(x, y);
	M5.Display.print(string);
}

void show_short(uint16_t x, uint16_t y, uint8_t variable)
{
	char string[4]; // print format stuff
	sprintf(string, "%2i", variable);
	M5.Display.setTextSize(3);
	M5.Display.setTextColor(TFT_YELLOW, TFT_BLACK);
	M5.Display.setCursor(x, y);
	M5.Display.print(string);
}


void show_decimal(uint16_t x, uint16_t y, float variable)
{
	char string[10];
	int units, fraction;
	float remainder;
	units = (int)variable;

	sprintf(string, "%5i", units);

	M5.Display.setTextSize(3);
	M5.Display.setCursor(x, y);
	M5.Display.setTextColor(TFT_BLACK, TFT_BLACK);
	M5.Display.print("           ");
	M5.Display.setTextColor(TFT_GREEN, TFT_BLACK);
	M5.Display.print(string);

}

void show_wide(uint16_t x, uint16_t y, int variable)
{
  char string[7];
  sprintf(string, "%6i", variable);
  M5.Display.setTextColor(TFT_YELLOW, TFT_BLACK);
  M5.Display.setCursor(x, y);
  M5.Display.setTextSize(3);
  M5.Display.print(string);
}

static int setup_station_call(const char *call_part)
{
  int result = 0;
  if (call_part != NULL)
  {
    size_t i = strlen(call_part);
    result = i > 0 && i < sizeof(Station_Call) ? 1 : 0;
    if (result != 0)
    {
      strcpy(Station_Call, call_part);
      M5.Display.setTextSize(3);
      M5.Display.setTextColor(TFT_YELLOW, TFT_BLACK);
      M5.Display.setCursor(0, 1100);
		  M5.Display.print(Station_Call);
    }
  }
  return result;
}

static int setup_locator(const char *locator_part)
{
  int result = 0;
  if (locator_part != NULL)
  {
    size_t i = strlen(locator_part);
    result = i > 0 && i < sizeof(Station_Locator) ? 1 : 0;
    if (result != 0)
    {
      strcpy(Station_Locator, locator_part);
      memcpy(Short_Station_Locator, locator_part, 4);
      Short_Station_Locator[4] = 0;
      M5.Display.setTextSize(3);
      M5.Display.setTextColor(TFT_YELLOW, TFT_BLACK);
      M5.Display.setCursor(100, 1100);
		  M5.Display.print(Station_Locator);
    }
  }
  return result;
}






static int setup_WIFI_SSID(const char *SSID_part)
{
  int result = 0;
  if (SSID_part != NULL)
  {
    size_t i = strlen(SSID_part);
    result = i > 0 && i < sizeof(WIFI_SSID) ? 1 : 0;
    if (result != 0)
    {
      strcpy(WIFI_SSID, SSID_part);
      M5.Display.setTextSize(3);
      M5.Display.setTextColor(TFT_YELLOW, TFT_BLACK);
      M5.Display.setCursor(0, 1000);
		  M5.Display.print(WIFI_SSID);
    }
  }
  return result;
}

static int setup_password(const char *PASSWORD_part)
{
  int result = 0;
  if (PASSWORD_part != NULL)
  {
    size_t i = strlen(PASSWORD_part);
    result = i > 0 && i < sizeof(WIFI_PASSWORD) ? 1 : 0;
    if (result != 0)
    {
      strcpy(WIFI_PASSWORD, PASSWORD_part);
      M5.Display.setTextColor(TFT_YELLOW, TFT_BLACK);
      M5.Display.setCursor(350, 1000);
		  M5.Display.print(WIFI_PASSWORD);
    }
  }
  return result;
}

enum
{
  FreeText1,
  FreeText2
};

static int setup_free_text(const char *free_text, int field_id)
{
  int result = 0;
  if (free_text != NULL)
  {
    size_t i = strlen(free_text);
    switch (field_id)
    {
    case FreeText1:
      result = i < sizeof(Free_Text1) ? 1 : 0;
      if (i > 0 && result != 0)
        strcpy(Free_Text1, free_text);
      break;
    case FreeText2:
      result = i < sizeof(Free_Text2) ? 1 : 0;
      if (i > 0 && result != 0)
        strcpy(Free_Text2, free_text);
      break;
    default:
      result = 1;
    }
  }
  return result;
}


bool open_stationData_file(void)
{
  Station_Call[0] = 0;
  Station_Locator[0] = 0;
  Short_Station_Locator[0] = 0;
  Free_Text1[0] = 0;
  Free_Text2[0] = 0;

  char read_buffer[256];

  M5.Display.setTextColor(TFT_YELLOW, TFT_BLACK);
  M5.Display.setTextSize(3);
  

    // Define the correct hardware SPI pins for the Tab5 MicroSD Slot
    int sck_pin  = 43;
    int miso_pin = 39;
    int mosi_pin = 44;
    int cs_pin   = 42;

    // Initialize the custom SPI bus using the native hardware pins
    SPI.begin(sck_pin, miso_pin, mosi_pin, cs_pin);

    // Initialize the SD card library using the dedicated SPI bus and CS pin
	M5.Display.setCursor(0, 300);
    if (!SD.begin(cs_pin, SPI)) { 
        M5.Display.print("SD Card not found");
        return false;
    }
   // M5.Display.print("SD Card mounted successfully.");

	  // Define the file and maximum buffer size for reading configuration lines
    const char* filename = "/StationData.ini";
    const size_t bufferLen = 64;
    char buffer[bufferLen];

    IniFile ini(filename);
    
	M5.Display.setCursor(200, 300);
    if (!ini.open()) {
		M5.Display.print("Failed to open INI file.");
        return false;
    }
   // M5.Display.print("INI file opened successfully.");

    // Check if the INI file structure is valid
    if (!ini.validate(buffer, bufferLen)) {
		    M5.Display.setCursor(0, 300);
        M5.Display.print("INI file validation failed: ");
        return false;
    }

	  if (ini.getValue("Station", "Call", buffer, bufferLen)) {
		setup_station_call(buffer);
    } 

		if (ini.getValue("Station", "Locator", buffer, bufferLen)) {
		setup_locator(buffer);
    } 

    if (ini.getValue("Wifi", "WIFI_SSID", buffer, bufferLen)) {
		setup_WIFI_SSID(buffer);
    } 

if (ini.getValue("Wifi", "WIFI_PASSWORD", buffer, bufferLen)) {
		setup_password(buffer);
    } 

 
	ini.close();

	return true;

}


void display_station_data(int x, int y)
{
  char str[13];
  sprintf(str, "%7s %4s", Station_Call, Short_Station_Locator);

  M5.Display.setTextColor(TFT_YELLOW, TFT_BLACK);
  M5.Display.setCursor(x, y);
  M5.Display.setTextSize(3);
  M5.Display.print(str);
}

void update_message_log_display(int mode)
{
  const char blank[] = "                      ";

  for (int i = 0; i < max_log_messages - 1; i++)
  {
    strcpy(log_messages[i].message, blank);
    strcpy(log_messages[i].message, log_messages[i + 1].message);
    log_messages[i].text_color = log_messages[i + 1].text_color;
  }

  if (mode)
  {
    strcpy(log_messages[max_log_messages - 1].message, blank);
    strcpy(log_messages[max_log_messages - 1].message, current_message);
    log_messages[max_log_messages - 1].text_color = 1;
  }
  else
  {
    strcpy(log_messages[max_log_messages - 1].message, blank);
    strcpy(log_messages[max_log_messages - 1].message, current_message);
    log_messages[max_log_messages - 1].text_color = 0;
  }

  log_display_flag = 1;
}

void display_logged_messages(void)
{
  char blank[] = "                      ";

  for (int i = 0; i < max_log_messages; i++)
  {
    
  display_line(true, i + 2, Black, Black, blank);
    
  if (log_messages[i].text_color)
  display_line(true, i + 2, Black, Yellow, log_messages[i].message);
  else
  display_line(true, i + 2, Black, Red, log_messages[i].message);
  }
}

void Display_WF(void) {

	M5.Display.startWrite();

  dsp_start_time = millis();
	const int byte_count_to_last_line = FFT_W * (FFT_H - 1);
	int xmit_flag;

	// shift data in memory by one time step (FFT_W)
	memmove(WF_Bfr, &WF_Bfr[FFT_W], byte_count_to_last_line);

	// set the new data in the last line unless a marker line is to be drawn
	for (int x = 0; x < FFT_W; x++)
	{
		WF_Bfr[byte_count_to_last_line + x] = (ft8_marker) ? marker_line_colour_index : FFT_Buffer[x + ft8_min_bin];
		
	}

	// Draw the waterfall from the bottom to the top
	uint8_t *ptr = &WF_Bfr[byte_count_to_last_line];
	for (int y = FFT_H - 1; y >= 0; y--)
	{
		for (int x = 0; x < FFT_W; x++)
		{
			
			// Each FFT datum is 6.25hz, the transmit bandwidth is 50Hz (= 8 pixels)
			if ((x == cursor_line) || (x == cursor_line + 8))
			{
				M5.Display.drawPixel(2*x, y, xmit_flag ? TFT_RED : TFT_DARKSLATEBLUE);
			}
			else
			{
				M5.Display.drawPixel(2*x, y, WFPalette[*ptr]);
			}
			
		//	M5.Display.drawPixel(2*x, y, WFPalette[*ptr]);
			ptr++;
		}

		ptr -= (FFT_W * 2);
	}

	ft8_marker = 0;

 // dsp_current_time = millis();
  //dsp_flag_time = dsp_current_time - dsp_start_time;
  //show_variable(500, 1240,(int)dsp_flag_time);

	M5.Display.endWrite();
}




 void setup_RTC(void)
{
  auto cfg = M5.config();

  cfg.external_rtc  = true;  // default=false. use Unit RTC.

  M5.begin(cfg);

    M5.Display.setRotation(0);
    M5.Display.setTextColor(TFT_YELLOW, TFT_BLACK);
    M5.Display.setTextSize(3);
    M5.Display.setCursor(0, 200);

  if (!M5.Rtc.isEnabled())
  {
	M5.Display.println("RTC not found.");
    for (;;) { M5.delay(500); }
  }
	M5.Display.println("RTC Found.");


/// setup RTC ( NTP auto setting )
  configTzTime(NTP_TIMEZONE, NTP_SERVER1, NTP_SERVER2, NTP_SERVER3);
#if __has_include(<WiFi.h>)
  M5.Display.println("WiFi:");

#if defined (CONFIG_IDF_TARGET_ESP32P4)
  if (M5.getBoard() == m5::board_t::board_M5Tab5) {
    WiFi.setPins(GPIO_NUM_12, GPIO_NUM_13, GPIO_NUM_11, GPIO_NUM_10, GPIO_NUM_9, GPIO_NUM_8, GPIO_NUM_15);
  }
#endif

  WiFi.begin( WIFI_SSID, WIFI_PASSWORD );

  for (int i = 20; i && WiFi.status() != WL_CONNECTED; --i)
  {
    //M5.Display.println(".");
    M5.delay(500);
  }
  if (WiFi.status() == WL_CONNECTED) {
    M5.Display.println("\r\nWiFi Connected.");
    M5.Display.println("NTP:");
#if SNTP_ENABLED
    while (sntp_get_sync_status() != SNTP_SYNC_STATUS_COMPLETED)
    {
      //M5.Display.println(".");
      M5.delay(1000);
    }
#else
    M5.delay(1600);
    struct tm timeInfo;
    while (!getLocalTime(&timeInfo, 1000))
    {
      M5.Log.print('.');
    };
#endif
    M5.Display.println("\r\nNTP Connected.");
    delay(1000);
    time_t t = time(nullptr)+1; // Advance one second.
    while (t > time(nullptr));  /// Synchronization in seconds
    M5.Rtc.setDateTime( gmtime( &t ) );
  }
  else
  {
    M5.Display.println("\r\nWiFi none...");
    delay(1000);
  }
#endif

WiFi.disconnectAsync(true, true);

  clear_rx_region();
}