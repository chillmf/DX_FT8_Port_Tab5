
/* Copyright (C) 2025 Paul Winwood G8KIG - All Rights Reserved. */

#ifndef PSK_INTERFACE_H
#define PSK_INTERFACE_H

void requestTimeSync();
void getTime();
bool addSenderRecord(const char *callsign, const char *gridSquare, const char *software);
bool addReceivedRecord(const char *callsign, uint32_t frequency, uint8_t snr);
bool sendRequest(void);

#endif