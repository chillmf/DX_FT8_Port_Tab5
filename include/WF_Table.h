/*
 * WF_Table.h
 *
 *  Created on: Dec 25, 2018
 *      Author: user
 */

#ifndef WF_TABLE_H_
#define WF_TABLE_H_

#include <Arduino.h>
#include <M5Unified.h>
#include <M5GFX.h>

const uint32_t WFPalette[] = {
		TFT_BLACK, 		//  0
		TFT_BLACK, 		//  1
		TFT_DARKSLATEBLUE, 	//  2
		TFT_BLUE,			//  3
		TFT_LIGHTBLUE ,	//  4
		TFT_DARKOLIVEGREEN,	//  5
		TFT_GREEN,		//  6
		TFT_LIGHTGREEN,	//  7
		TFT_DARKRED ,	 	//  8
		TFT_RED,		 	//  9
		TFT_LIGHTPINK ,	 	// 10
		TFT_DARKORANGE,	 	// 11
		TFT_GREENYELLOW,	// 12
		TFT_YELLOW ,	 	// 13
		TFT_LIGHTYELLOW, 	// 14
		TFT_WHITE,		// 15
		TFT_GHOSTWHITE	// 16
};

const int marker_line_colour_index = 16; // GRAY index in WFPalette

#endif /* WF_TABLE_H_ */

