/*
 * decode_ft8.h
 *
 *  Created on: Nov 2, 2019
 *      Author: user
 */

#ifndef DECODE_FT8_H_
#define DECODE_FT8_H_

#include <Arduino.h>
#include <M5Unified.h>
#include <M5GFX.h>

int ft8_decode(void);

extern int max_sync_score;
extern int max_sync_score_index;
extern int auto_called;
extern int auto_logged;

enum Sequence
{
    Seq_RSL = 0,
    Seq_Locator
};

struct Decode
{
    char call_to[14];
    char call_from[14];
    char locator[7];
    int freq_hz;
    int sync_score;
    int snr;
    int received_snr;
    char target_locator[7];
    int target_distance;
    int slot;
    Sequence sequence;
    int calling_CQ;
};

struct display_message_details
{
    char message[22];
    int text_color;
};

typedef enum _MsgColor
{
    Black = 0,
    White,
    Red,
    Green,
    Blue,
    Yellow,
    LastColor
} MsgColor;

const int32_t lcd_color_map[LastColor] = {
    
    0x0000, // BLACK
    0xFFFF, // WHITE
    0xF800, // RED
    0x07E0, // GREEN
    0x001F, // BLUE
    0xFFE0 // YELLOW
    
};

struct Called_Stations
{
    char call[14];
    float distance;
    int sync_score;
};

void process_selected_Station(int stations_decoded, int TouchIndex);

void display_line(bool right, int line, MsgColor background, MsgColor textcolor, const char *text);
void display_messages(Decode new_decoded[], int decoded_messages);
void clear_rx_region(void);
void clear_qso_region(void);
void display_queued_message(const char *msg);
void display_txing_message(const char *msg);
void display_qso_state(const char *txt);
char *add_worked_qso(void);
bool display_worked_qsos(void);

void display_call_list_item(int left, int line, MsgColor background, MsgColor textcolor, const char *text);
void display_call_list(int number_calls);

void display_logged_list_item(int left, int line, MsgColor background, MsgColor textcolor, const char *text);
void display_logged_list(int number_calls);

int check_call_list(int message_index);
int check_log_list(int message_index);
void store_CQ_Call(void);
void store_logged_CQ_Call(const char *call);
void clear_auto_memories(void);

int strindex(const char *s, const char *t);
int ft8_decode(void);

extern struct Decode new_decoded[];
extern size_t kMax_message_length;
extern int was_txing;
extern int Valid_CQ_Candidate;

#endif /* DECODE_FT8_H_ */
