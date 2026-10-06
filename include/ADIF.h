/*
 * ADIF.h
 *
 *  Created on: Jun 18, 2023
 *      Author: Charley
 */

#ifndef ADIF_H_
#define ADIF_H_

#include <SD.h>

struct Map_Memory
{
    float ADIF_distance;
    float ADIF_map_distance;
    float ADIF_map_bearing;
};

void draw_map(int16_t index);
void write_ADIF_Log(void);
void Init_Log_File(void);

void set_Station_Coordinates();
float Target_Distance(const char *target);

#endif /* ADIF_H_ */
