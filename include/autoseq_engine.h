/* ====================================================
 *  Public API implementation
 * ==================================================== */

#include <stdbool.h>
#include "decode_ft8.h"
#include "main.h"
#include "button.h"

#define MAX_MSG_LEN 22
#define MAX_LINE_LEN 22
#define MAX_RX_ROWS 10
#define MAX_QSO_ROWS 9
#define MAX_QSO_ENTRIES 100
#define START_X_LEFT 0
#define START_X_RIGHT 350
#define START_Y 200 // FFT_H
#define LINE_HT 50
const size_t CALLSIGN_SIZE = 14;
const size_t LOCATOR_SIZE = 7;
const size_t RSL_SIZE = 5;

void autoseq_init(const char *myCall, const char *myGrid);

void autoseq_start_cq(void);

/* === Called for selected decode (manual response) === */
void autoseq_on_touch(const Decode *msg);

/* === Called for **every** new decode (auto response) === */
/* Return whether TX is needed */
bool autoseq_on_decode(const Decode *msg);

/* === Provide the message we should transmit this slot (if any) === */
bool autoseq_get_next_tx(char *out_text);

/* === Populate the string for displaying the current QSO state  === */
void autoseq_get_qso_state(char *out_text);

/* === Slot timer / time‑out manager === */
void autoseq_tick(void);
