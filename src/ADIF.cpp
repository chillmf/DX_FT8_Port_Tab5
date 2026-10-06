
/*
 * ADIF.c
 *
 *  Created on: Jun 18, 2023
 *      Author: Charley
 */
#include <Arduino.h>
#include <M5Unified.h>
#include <M5GFX.h>
#include <SPI.h>
#include <SD.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <SD.h>
#include <SPI.h>
#include "si5351.h"
#include "button.h"
#include "display.h"
#include "ADIF.h"
#include "gen_ft8.h"
#include "decode_ft8.h"
#include "main.h"
#include "Geodesy.h"

static const double EARTH_RAD = 6371; // radius in km

static char log_rtc_date_string[9];
static char log_rtc_time_string[9];
static char display_rtc_time_string[9];

static char file_name_string[24];

static int ADIF_distance;
static int ADIF_map_distance;
static int ADIF_map_bearing;

static int16_t start_x;
static int16_t start_y;
static int16_t center_x;
static int16_t center_y;

static int16_t map_width;
static int16_t map_height;
static int16_t map_center_x;
static int16_t map_center_y;

static char map_locator[7];
static int map_key_index = 0;
static Map_Memory stored_log_entries[100] = {0};
static int number_logged = 0;
static float Station_Latitude, Station_Longitude;
static float Map_Latitude, Map_Longitude;
static float Target_Latitude, Target_Longitude;

static float QTH_Distance;
static float QTH_Bearing;

static File Log_File;

// convert degrees to radians
inline double deg2rad(double deg)
{
  return deg * (PI / 180.0);
}

inline double rad2deg(double rad)
{
  return (rad * 180) / PI;
}

static void make_date(void)
{
  auto dt = M5.Rtc.getDateTime();
  sprintf(log_rtc_date_string, "%04i%02i%02i", dt.date.year,dt.date.month,dt.date.date);
}

void make_time(void)
{
 auto dt = M5.Rtc.getDateTime();
  sprintf(log_rtc_time_string, "%02i%02i%02i", dt.time.hours,dt.time.minutes,dt.time.seconds);
  sprintf(display_rtc_time_string, "%2.2i:%2.2i:%2.2i", dt.time.hours,dt.time.minutes,dt.time.seconds);
  
}

void make_File_Name(void)
{
  make_date();
  sprintf((char *)file_name_string, "/ %s.adi", log_rtc_date_string);
}

static void write_log_data(char *data)
{
  File logFile = SD.open(file_name_string, FILE_APPEND);
  if (logFile) {
  logFile.println(data);
  logFile.close();
  }
}

void Open_Log_File(void){

  auto cfg = M5.config();
  M5.begin(cfg);
  int sck_pin  = 43;
  int miso_pin = 39;
  int mosi_pin = 44;
  int cs_pin   = 42;

    SPI.begin(sck_pin, miso_pin, mosi_pin, cs_pin);

    M5.Display.setCursor(0, 300);
    if (!SD.begin(cs_pin, SPI)) { 
     M5.Display.print("SD Card NOT Found");
     }

  File logFile = SD.open(file_name_string, FILE_APPEND);
  if (logFile) {
  
    if (logFile.size() == 0)
    {
      logFile.println("ADIF EXPORT");
      logFile.println("<eoh>");
    }

  logFile.close();

   }

  }





void Init_Log_File(void)
{
  make_File_Name();
  Open_Log_File();
}



// distance (km) on earth's surface from point 1 to point 2
static double distance(double lat1, double lon1, double lat2, double lon2)
{
  double lat1r = deg2rad(lat1);
  double lon1r = deg2rad(lon1);
  double lat2r = deg2rad(lat2);
  double lon2r = deg2rad(lon2);

  return acos(sin(lat1r) * sin(lat2r) + cos(lat1r) * cos(lat2r) * cos(lon2r - lon1r)) * EARTH_RAD;
}

float Target_Distance(const char *target)
{
  LatLong ll = QRAtoLatLong(target);
  if (ll.isValid)
  {
    Target_Latitude = ll.latitude;
    Target_Longitude = ll.longitude;
  }
  else
  {
    Target_Latitude = Target_Longitude = 0.0;
  }

  return distance(Station_Latitude, Station_Longitude, Target_Latitude, Target_Longitude);
}

static float Map_Distance(const char *target)
{
  LatLong ll = QRAtoLatLong(map_locator);
  if (ll.isValid)
  {
    Map_Latitude = ll.latitude;
    Map_Longitude = ll.longitude;
  }
  else
  {
    Map_Latitude = Map_Longitude = 0.0;
  }

  ll = QRAtoLatLong(target);
  if (ll.isValid)
  {
    Target_Latitude = ll.latitude;
    Target_Longitude = ll.longitude;
  }
  else
  {
    Target_Latitude = Target_Longitude = 0.0;
  }
  return distance(Map_Latitude, Map_Longitude, Target_Latitude, Target_Longitude);
}

static double bearing(double lat1, double long1, double lat2, double long2)
{
  double dlon = deg2rad(long2 - long1);
  lat1 = deg2rad(lat1);
  lat2 = deg2rad(lat2);
  double a1 = sin(dlon) * cos(lat2);
  double a2 = sin(lat1) * cos(lat2) * cos(dlon);
  a2 = cos(lat1) * sin(lat2) - a2;
  a2 = atan2(a1, a2);
  if (a2 < 0.0)
  {
    a2 += (2 * PI);
  }
  return rad2deg(a2);
}

float Map_Bearing(const char *target)
{
  LatLong ll = QRAtoLatLong(map_locator);
  if (ll.isValid)
  {
    Map_Latitude = ll.latitude;
    Map_Longitude = ll.longitude;
  }
  else
  {
    Map_Latitude = Map_Longitude = 0.0;
  }

  ll = QRAtoLatLong(target);
  if (ll.isValid)
  {
    Target_Latitude = ll.latitude;
    Target_Longitude = ll.longitude;
  }
  else
  {
    Target_Latitude = Target_Longitude = 0.0;
  }

  return bearing(Map_Latitude, Map_Longitude, Target_Latitude, Target_Longitude);
}

static unsigned num_digits(int num)
{
  int count = 0;
  if ((num <= -100) && (num > -1000))
    count = 4;
  else if (((num >= 100) && (num < 1000)) || ((num <= -10) && (num > -100)))
    count = 3;
  else if (((num >= 10) && (num < 100)) || ((num <= -1) && num > -10))
    count = 2;
  else if (num >= 0 && num < 10)
    count = 1;
  return count;
}

static const char *trim_front(const char *ptr)
{
  while (isspace((int)*ptr))
    ++ptr;
  return ptr;
}

static unsigned num_chars(const char *ptr)
{
  return (unsigned)strlen(trim_front(ptr));
}

void write_ADIF_Log()
{
  static char log_line[300];
  char freq[10];

  make_time();
  make_date();

  sprintf(freq, "%u.%.3u", sBand_Data[BandIndex].Frequency / 1000, sBand_Data[BandIndex].Frequency % 1000);

  int offset = sprintf(log_line, "<call:%1u>%s ", num_chars(Target_Call), trim_front(Target_Call));
  int target_locator_len = num_chars(Target_Locator);
  if (target_locator_len > 0)
    offset += sprintf(log_line + offset, "<gridsquare:%1u>%s ", target_locator_len, trim_front(Target_Locator));
  offset += sprintf(log_line + offset, "<mode:3>FT8<qso_date:%1u>%s ", num_chars(log_rtc_date_string), trim_front(log_rtc_date_string));
  offset += sprintf(log_line + offset, "<time_on:%1u>%s ", num_chars(log_rtc_time_string), trim_front(log_rtc_time_string));
  offset += sprintf(log_line + offset, "<freq:%1u>%s ", num_chars(freq), trim_front(freq));
  offset += sprintf(log_line + offset, "<station_callsign:%1u>%s ", num_chars(Station_Call), trim_front(Station_Call));
  offset += sprintf(log_line + offset, "<my_gridsquare:%1u>%s ", num_chars(Station_Locator), trim_front(Station_Locator));

  int rsl_len = num_digits(Target_RSL);
  if (rsl_len > 0)
    offset += sprintf(log_line + offset, "<rst_sent:%1u:N>%i ", rsl_len, Target_RSL);

  rsl_len = num_digits(Station_RSL);
  if (rsl_len > 0)
    offset += sprintf(log_line + offset, "<rst_rcvd:%1u:N>%i ", rsl_len, Station_RSL);

  strcpy(log_line + offset, "<tx_pwr:4>0.5 <eor>");

  // Force NULL termination
  log_line[sizeof(log_line) - 1] = 0;

  write_log_data(log_line);
  if (Auto_QSO) store_logged_CQ_Call(Target_Call);


char received_message[22];
        sprintf(received_message, "%s %s %s", Target_Call, Target_Locator, display_rtc_time_string);
        strcpy(current_message, received_message);
        update_message_log_display(1);
        display_logged_messages();

}

void set_Station_Coordinates()
{
  LatLong ll = QRAtoLatLong(Station_Locator);
  if (ll.isValid)
  {
    Station_Latitude = ll.latitude;
    Station_Longitude = ll.longitude;
  }
  else
  {
    Station_Latitude = Station_Longitude = 0.0;
  }
}




