/*  Extenmote : NES, SNES, N64 and Gamecube to Wii remote adapter firmware
 *  Copyright (C) 2012-2014  Raphael Assenat <raph@raphnet.net>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <string.h>

#include "classic.h"
#include "eeprom.h"
#include "analog.h"
#include "rlut.h"
#include "tripleclick.h"

#define C_DEFLECTION	100

/*       |                 Bit                                |
 * Byte  |   7   |   6   |   5   |  4  |  3 |  2 |  1  |  0   |
 * ------+---------------+-------+----------------------------+
 *  0    |    RX<4:3>    |            LX<5:0>                 |
 *  1    |    RX<2:1>    |           LY<5:0>                  |
 *  2    | RX<0> |    LT<4:3>    |      RY<4:0>               |
 *  3    |        LT<2:0>        |      RT<4:0>               |
 *  4    | BDR   |  BDD  |  BLT  |  B- | BH | B+  | BRT | 1   |
 *  5    | BZL   |   BB  |  BY   | BA  | BX | BZR | BDL | BDU |
 *
 *  6    | 0x52 ('R')                                         |
 *  7    | Controller ID byte 0                               |
 *  8    | Controller ID byte 1                               |
 *
 *  9    | Controller specific data 0                         |
 *  10   | Controller specific data 1                         |
 *  11   | Controller specific data 2                         |
 *  12   | Controller specific data 3                         |
 *  13   | Controller specific data 4                         |
 *  14   | Controller specific data 5                         |
 *  15   | Controller specific data 6                         |
 *  16   | Controller specific data 7                         |
 *
 *  Controller ID:
 *
 *   0   |  1
 *  -----+-----
 *   '6' | '4'    N64 controller
 *   'G' | 'C'    Gamecube controller
 *   'S' | 'F'    SNES
 *   'F' | 'C'    NES
 *
 * Writing at byte 6 might one day control the rumble motor. Rumbles on when non-zero.
 */

void pack_classic_data(classic_pad_data *src, unsigned char dst[15], int analog_style)
{
	unsigned char rx,ry,lx=0x20,ly=0x20; // down sized
	unsigned char shoulder_left=0, shoulder_right=0; // lower 5 bits only

	if (analog_style == ANALOG_STYLE_N64)
	{
		if (g_current_config.g_n64_mapping_mode != MODE_TEST) {
#ifdef WITH_N64
			lx = applyCurve(src->lx, g_current_config.g_n64_curve_id);
			ly = applyCurve(src->ly, g_current_config.g_n64_curve_id);
#endif
		} else {
			lx = 0x20 + src->lx;
			ly = 0x20 + src->ly;
		}

	} else if (analog_style == ANALOG_STYLE_GC) {
#ifdef WITH_GAMECUBE
		lx = applyCurve(src->lx, RLUT_GC1);
		ly = applyCurve(src->ly, RLUT_GC1);
#endif
	}
	else {
		lx = ((0x80 + src->lx) >> 2) & 0x3F;
		ly = ((0x80 + src->ly) >> 2) & 0x3F;
	}

	rx = ((0x80 + src->rx) >> 3) & 0x1F;
	ry = ((0x80 + src->ry) >> 3) & 0x1F;

	shoulder_left = (src->lt >> 3);
	shoulder_right = (src->rt >> 3);

	dst[0] = ((rx<<3) & 0xC0) | lx;
	dst[1] = ((rx<<5) & 0xC0) | ly;
	dst[2] = rx<<7 | ry | (shoulder_left & 0x18) << 2;

	dst[3] = (shoulder_left << 5) | (shoulder_right & 0x1F);
	dst[4] = (src->buttons >> 8) ^ 0xFF;
	dst[5] = (src->buttons) ^ 0xFF;

	dst[6] = 'R';
	memcpy(dst+7, src->controller_id, 2);
	memcpy(dst+9, src->controller_raw_data, 8);
}

void dataToClassic(const gamepad_data *src, classic_pad_data *dst, char first_read)
{
	static char test_x = 0, test_y = 0;

	memset(dst, 0, sizeof(classic_pad_data));

	switch (src->pad_type)
	{
		case PAD_TYPE_NONE:
			break;

		case PAD_TYPE_SNES:
			dst->controller_id[0] = 'S';
			dst->controller_id[1] = 'F';
			memcpy(dst->controller_raw_data, src->snes.raw_data, SNES_RAW_SIZE);

			if (g_current_config.g_snes_nes_mode) {
				if (src->snes.buttons & SNES_BTN_Y) { dst->buttons |= CPAD_BTN_B; }
				if (src->snes.buttons & SNES_BTN_B) { dst->buttons |= CPAD_BTN_A; }
			} else {
				if (src->snes.buttons & SNES_BTN_B) { dst->buttons |= CPAD_BTN_B; }
				if (src->snes.buttons & SNES_BTN_Y) { dst->buttons |= CPAD_BTN_Y; }
				if (src->snes.buttons & SNES_BTN_A) { dst->buttons |= CPAD_BTN_A; }
				if (src->snes.buttons & SNES_BTN_X) { dst->buttons |= CPAD_BTN_X; }
			}

			if (g_current_config.g_snes_analog_dpad) {
				if (src->snes.buttons & SNES_BTN_DPAD_UP) { dst->ly = 100; }
				if (src->snes.buttons & SNES_BTN_DPAD_DOWN) { dst->ly = -100; }
				if (src->snes.buttons & SNES_BTN_DPAD_LEFT) { dst->lx = -100; }
				if (src->snes.buttons & SNES_BTN_DPAD_RIGHT) { dst->lx = 100; }
			} else {
				if (src->snes.buttons & SNES_BTN_DPAD_UP) { dst->buttons |= CPAD_BTN_DPAD_UP; }
				if (src->snes.buttons & SNES_BTN_DPAD_DOWN) { dst->buttons |= CPAD_BTN_DPAD_DOWN; }
				if (src->snes.buttons & SNES_BTN_DPAD_LEFT) { dst->buttons |= CPAD_BTN_DPAD_LEFT; }
				if (src->snes.buttons & SNES_BTN_DPAD_RIGHT) { dst->buttons |= CPAD_BTN_DPAD_RIGHT; }
			}

			if (src->snes.buttons & SNES_BTN_SELECT) { dst->buttons |= CPAD_BTN_MINUS; }
			if (src->snes.buttons & SNES_BTN_START) { dst->buttons |= CPAD_BTN_PLUS; }
			if (src->snes.buttons & SNES_BTN_L) { dst->buttons |= CPAD_BTN_TRIG_LEFT; }
			if (src->snes.buttons & SNES_BTN_R) { dst->buttons |= CPAD_BTN_TRIG_RIGHT; }

			if (IS_SIMULTANEOUS(src->snes.buttons, SNES_BTN_START|SNES_BTN_SELECT|SNES_BTN_L|SNES_BTN_R|SNES_BTN_DPAD_UP)) {
				g_current_config.g_snes_nes_mode = 0;
				g_current_config.g_snes_analog_dpad = 0;
				sync_config();
			}
			if (IS_SIMULTANEOUS(src->snes.buttons, SNES_BTN_START|SNES_BTN_SELECT|SNES_BTN_L|SNES_BTN_R|SNES_BTN_DPAD_DOWN)) {
				g_current_config.g_snes_nes_mode = 1;
				g_current_config.g_snes_analog_dpad = 0;
				sync_config();
			}
			if (IS_SIMULTANEOUS(src->snes.buttons, SNES_BTN_START|SNES_BTN_SELECT|SNES_BTN_L|SNES_BTN_R|SNES_BTN_DPAD_LEFT)) {
				g_current_config.g_snes_analog_dpad = 1;
				g_current_config.g_snes_nes_mode = 0;
				sync_config();
			}

			if (isTripleClick(src->snes.buttons & SNES_BTN_START)) {
				dst->buttons |= CPAD_BTN_HOME;
			}

			// Simulate L/R fully pressed values (like the analogue-less classic controller pro does)
			if (dst->buttons & CPAD_BTN_TRIG_LEFT) {
				dst->lt = 0xff;
			}
			if (dst->buttons & CPAD_BTN_TRIG_RIGHT) {
				dst->rt = 0xff;
			}
			break;

		case PAD_TYPE_NES:
//			if (first_read && src->nes.buttons & NES_BTN_START) {
//				disable_config = 1;
//			}

			dst->controller_id[0] = 'F';
			dst->controller_id[1] = 'C';
			memcpy(dst->controller_raw_data, src->nes.raw_data, NES_RAW_SIZE);

			if (src->nes.buttons & NES_BTN_A) { dst->buttons |= CPAD_BTN_A; }
			if (src->nes.buttons & NES_BTN_B) { dst->buttons |= CPAD_BTN_B; }
			if (src->nes.buttons & NES_BTN_SELECT) { dst->buttons |= CPAD_BTN_MINUS; }
			if (src->nes.buttons & NES_BTN_START) { dst->buttons |= CPAD_BTN_PLUS; }
			if (src->nes.buttons & NES_BTN_DPAD_UP) { dst->buttons |= CPAD_BTN_DPAD_UP; }
			if (src->nes.buttons & NES_BTN_DPAD_DOWN) { dst->buttons |= CPAD_BTN_DPAD_DOWN; }
			if (src->nes.buttons & NES_BTN_DPAD_LEFT) { dst->buttons |= CPAD_BTN_DPAD_LEFT; }
			if (src->nes.buttons & NES_BTN_DPAD_RIGHT) { dst->buttons |= CPAD_BTN_DPAD_RIGHT; }

			if (isTripleClick(src->nes.buttons & NES_BTN_START)) {
				dst->buttons |= CPAD_BTN_HOME;
			}

			break;

#ifdef WITH_GAMECUBE
		case PAD_TYPE_GAMECUBE:
			/* Raw data */
			dst->controller_id[0] = 'G';
			dst->controller_id[1] = 'C';
			memcpy(dst->controller_raw_data, src->gc.raw_data, GC_RAW_SIZE);

			if (first_read && src->gc.buttons & GC_BTN_START) {
				disable_config = 1;
			}

			switch (g_current_config.g_gc_mapping_mode) {
				case MODE_GC_STANDARD:
					if (src->gc.buttons & GC_BTN_A) { dst->buttons |= CPAD_BTN_A; }
					if (src->gc.buttons & GC_BTN_B) { dst->buttons |= CPAD_BTN_B; }
					if (src->gc.buttons & GC_BTN_X) { dst->buttons |= CPAD_BTN_X; }
					if (src->gc.buttons & GC_BTN_Y) { dst->buttons |= CPAD_BTN_Y; }
					if (src->gc.buttons & GC_BTN_Z) { dst->buttons |= CPAD_BTN_ZL|CPAD_BTN_ZR; }
					if (src->gc.buttons & GC_BTN_L) { dst->buttons |= CPAD_BTN_TRIG_LEFT; }
					if (src->gc.buttons & GC_BTN_R) { dst->buttons |= CPAD_BTN_TRIG_RIGHT; }
					break;

				case MODE_GC_SNES:
					if (src->gc.buttons & GC_BTN_B) { dst->buttons |= CPAD_BTN_Y; }
					if (src->gc.buttons & GC_BTN_A) { dst->buttons |= CPAD_BTN_B; }
					if (src->gc.buttons & GC_BTN_Y) { dst->buttons |= CPAD_BTN_X; }
					if (src->gc.buttons & GC_BTN_X) { dst->buttons |= CPAD_BTN_A; }
					if (src->gc.buttons & GC_BTN_Z) { dst->buttons |= CPAD_BTN_MINUS; }
					if (src->gc.buttons & GC_BTN_L) { dst->buttons |= CPAD_BTN_TRIG_LEFT; }
					if (src->gc.buttons & GC_BTN_R) { dst->buttons |= CPAD_BTN_TRIG_RIGHT; }
					break;

				case MODE_GC_ZLR:
					if (src->gc.buttons & GC_BTN_A) { dst->buttons |= CPAD_BTN_A; }
					if (src->gc.buttons & GC_BTN_B) { dst->buttons |= CPAD_BTN_B; }
					if (src->gc.buttons & GC_BTN_X) { dst->buttons |= CPAD_BTN_X; }
					if (src->gc.buttons & GC_BTN_Y) { dst->buttons |= CPAD_BTN_Y; }
					if (src->gc.buttons & GC_BTN_Z) { dst->buttons |= CPAD_BTN_TRIG_RIGHT; }
					if (src->gc.buttons & GC_BTN_L) { dst->buttons |= CPAD_BTN_ZL; }
					if (src->gc.buttons & GC_BTN_R) { dst->buttons |= CPAD_BTN_ZR; }
					break;

				case MODE_GC_ASR:
					if (src->gc.buttons & GC_BTN_A) { dst->buttons |= CPAD_BTN_ZR; } // Gas
					if (src->gc.buttons & GC_BTN_B) { dst->buttons |= CPAD_BTN_A; } // Exchange item
					if (src->gc.buttons & GC_BTN_X) { dst->buttons |= CPAD_BTN_B; } // Use item
					if (src->gc.buttons & GC_BTN_Y) { dst->buttons |= CPAD_BTN_X; } // ?
					if (src->gc.buttons & GC_BTN_Z) { dst->buttons |= CPAD_BTN_B; }	// Use item
					if (src->gc.buttons & GC_BTN_L) { dst->buttons |= CPAD_BTN_ZL; }// Break/reverse
					if (src->gc.buttons & GC_BTN_R) { dst->buttons |= CPAD_BTN_Y; }	// Rear view
					break;

				case MODE_GC_DEV:
					if (src->gc.buttons & GC_BTN_A) { dst->buttons |= CPAD_BTN_B; }
					if (src->gc.buttons & GC_BTN_B) { dst->buttons |= CPAD_BTN_Y; }
					if (src->gc.buttons & GC_BTN_X) { dst->buttons |= CPAD_BTN_A; }
					if (src->gc.buttons & GC_BTN_Y) { dst->buttons |= CPAD_BTN_X; }
					if (src->gc.buttons & GC_BTN_L) { dst->buttons |= CPAD_BTN_TRIG_LEFT; }
					if (src->gc.buttons & GC_BTN_R) { dst->buttons |= CPAD_BTN_TRIG_RIGHT; }
					if (src->gc.buttons & GC_BTN_Z) { dst->buttons |= CPAD_BTN_ZR; }
					break;

				case MODE_GC_EXTRA1:
					/* This was suggested for Super mario 3D World */
					if (src->gc.buttons & GC_BTN_A) { dst->buttons |= CPAD_BTN_A; } // Jump/accept
					if (src->gc.buttons & GC_BTN_B) { dst->buttons |= CPAD_BTN_X; } // Run/ability
					if (src->gc.buttons & GC_BTN_X) { dst->buttons |= CPAD_BTN_TRIG_RIGHT; } // Bubble
					if (src->gc.buttons & GC_BTN_Y) { dst->buttons |= CPAD_BTN_B; } // Cancel/Change character
					if (src->gc.buttons & GC_BTN_L) { dst->buttons |= CPAD_BTN_ZL; } // Stomp/Long jump
					if (src->gc.buttons & GC_BTN_R) { dst->buttons |= CPAD_BTN_ZR; } // Stomp/Long jump
					if (src->gc.buttons & GC_BTN_Z) { dst->buttons |= CPAD_BTN_MINUS; } // Stored power
					// Main stick -> Movement
					// C-Stick -> Camera
					break;
			}

			if (src->gc.buttons & GC_BTN_START) { dst->buttons |= CPAD_BTN_PLUS; }
			if (src->gc.buttons & GC_BTN_DPAD_UP) { dst->buttons |= CPAD_BTN_DPAD_UP; }
			if (src->gc.buttons & GC_BTN_DPAD_DOWN) { dst->buttons |= CPAD_BTN_DPAD_DOWN; }
			if (src->gc.buttons & GC_BTN_DPAD_LEFT) { dst->buttons |= CPAD_BTN_DPAD_LEFT; }
			if (src->gc.buttons & GC_BTN_DPAD_RIGHT) { dst->buttons |= CPAD_BTN_DPAD_RIGHT; }

			// Main/Left stick to classic left stick
			dst->lx = src->gc.x;
			dst->ly = src->gc.y;

			// C-Stick/right stick to classic right stick
			dst->rx = src->gc.cx;
			dst->ry = src->gc.cy;

			// Left/right triggers
			dst->lt = src->gc.lt;
			dst->rt = src->gc.rt;

			if (IS_SIMULTANEOUS(src->gc.buttons, GC_BTN_A|GC_BTN_B|GC_BTN_X|GC_BTN_Y|GC_BTN_DPAD_UP)) {
				chgMap(&g_current_config.g_gc_mapping_mode, MODE_GC_STANDARD);
			}
			else if (IS_SIMULTANEOUS(src->gc.buttons, GC_BTN_A|GC_BTN_B|GC_BTN_X|GC_BTN_Y|GC_BTN_DPAD_DOWN)) {
				chgMap(&g_current_config.g_gc_mapping_mode, MODE_GC_SNES);
			}
			else if (IS_SIMULTANEOUS(src->gc.buttons, GC_BTN_A|GC_BTN_B|GC_BTN_X|GC_BTN_Y|GC_BTN_DPAD_LEFT)) {
				chgMap(&g_current_config.g_gc_mapping_mode, MODE_GC_ZLR);
			}
			else if (IS_SIMULTANEOUS(src->gc.buttons, GC_BTN_A|GC_BTN_B|GC_BTN_X|GC_BTN_Y|GC_BTN_DPAD_RIGHT)) {
				chgMap(&g_current_config.g_gc_mapping_mode, MODE_GC_ASR);
			}
			else if (IS_SIMULTANEOUS(src->gc.buttons, GC_BTN_A|GC_BTN_B|GC_BTN_X|GC_BTN_Y|GC_BTN_DPAD_RIGHT)) {
				chgMap(&g_current_config.g_gc_mapping_mode, MODE_GC_ASR);
			}
			else if (IS_SIMULTANEOUS(src->gc.buttons, GC_BTN_A|GC_BTN_B|GC_BTN_X|GC_BTN_Y|GC_BTN_Z)) {
				if (src->gc.cx < -64) { // A + B + X + Y + Z + C-Left
					chgMap(&g_current_config.g_gc_mapping_mode, MODE_GC_DEV);
				}

				if (src->gc.cx > 64) { // A + B + X + Y + Z + C-Right
					chgMap(&g_current_config.g_gc_mapping_mode, MODE_GC_EXTRA1);
				}

				/*
				// Combo for HOME
				if (src->gc.cy < -64) {
					dst->buttons |= CPAD_BTN_HOME;
				}*/
			}

			if (isTripleClick(src->gc.buttons & GC_BTN_START)) {
				dst->buttons |= CPAD_BTN_HOME;
			}

			break;
#endif

#ifdef WITH_N64
		case PAD_TYPE_N64:
			dst->controller_id[0] = '6';
			dst->controller_id[1] = '4';
			memcpy(dst->controller_raw_data, src->n64.raw_data, N64_RAW_SIZE);

			if (first_read && src->n64.buttons & N64_BTN_START) {
				disable_config = 1;
			}

			if (IS_SIMULTANEOUS(src->n64.buttons, N64_BTN_L|N64_BTN_R|N64_BTN_Z|N64_BTN_DPAD_UP)) {
				chgMap(&g_current_config.g_n64_mapping_mode, MODE_N64_STANDARD);
			} else if (IS_SIMULTANEOUS(src->n64.buttons, N64_BTN_L|N64_BTN_R|N64_BTN_Z|N64_BTN_DPAD_DOWN)) {
				chgMap(&g_current_config.g_n64_mapping_mode, MODE_MARIOKART64);
			} else if (IS_SIMULTANEOUS(src->n64.buttons, N64_BTN_L|N64_BTN_R|N64_BTN_Z|N64_BTN_DPAD_LEFT)) {
				chgMap(&g_current_config.g_n64_mapping_mode, MODE_OCARINA);
			} else if (IS_SIMULTANEOUS(src->n64.buttons, N64_BTN_L|N64_BTN_R|N64_BTN_Z|N64_BTN_DPAD_RIGHT)) {
				chgMap(&g_current_config.g_n64_mapping_mode, MODE_SSMB);
			} else if (IS_SIMULTANEOUS(src->n64.buttons, N64_BTN_L|N64_BTN_R|N64_BTN_Z|N64_BTN_C_UP)) {
				chgMap(&g_current_config.g_n64_mapping_mode, MODE_SIN_AND_PUNISHMENT);
			} else if (IS_SIMULTANEOUS(src->n64.buttons, N64_BTN_L|N64_BTN_R|N64_BTN_Z|N64_BTN_C_DOWN)) {
				chgMap(&g_current_config.g_n64_mapping_mode, MODE_OGRE_BATTLE);
			}
			else if (IS_SIMULTANEOUS(src->n64.buttons,  N64_BTN_L|N64_BTN_R|N64_BTN_Z|N64_BTN_C_LEFT)) {
				chgMap(&g_current_config.g_n64_mapping_mode, MODE_F_ZERO_X);
			} else if (IS_SIMULTANEOUS(src->n64.buttons,  N64_BTN_L|N64_BTN_R|N64_BTN_Z|N64_BTN_C_RIGHT)) {
				chgMap(&g_current_config.g_n64_mapping_mode, MODE_YOSHI_STORY);
			}

			// Curves
			if (!disable_config) {
				if (IS_SIMULTANEOUS(src->n64.buttons,  N64_BTN_L|N64_BTN_R|N64_BTN_Z|N64_BTN_A)) {
					g_current_config.g_n64_curve_id = RLUT_V1_5;
					sync_config();
				} else if (IS_SIMULTANEOUS(src->n64.buttons,  N64_BTN_L|N64_BTN_R|N64_BTN_Z|N64_BTN_B)) {
					g_current_config.g_n64_curve_id = RLUT_V1_4;
					sync_config();
				}
			}

			if (src->n64.buttons & N64_BTN_A) { dst->buttons |= CPAD_BTN_A; }
			if (src->n64.buttons & N64_BTN_B) { dst->buttons |= CPAD_BTN_B; }
			if (src->n64.buttons & N64_BTN_START) { dst->buttons |= CPAD_BTN_PLUS; }

			if (g_current_config.g_n64_mapping_mode != MODE_TEST) {
				if (src->n64.buttons & N64_BTN_DPAD_UP) { dst->buttons |= CPAD_BTN_DPAD_UP; }
				if (src->n64.buttons & N64_BTN_DPAD_DOWN) { dst->buttons |= CPAD_BTN_DPAD_DOWN; }
				if (src->n64.buttons & N64_BTN_DPAD_LEFT) { dst->buttons |= CPAD_BTN_DPAD_LEFT; }
				if (src->n64.buttons & N64_BTN_DPAD_RIGHT) { dst->buttons |= CPAD_BTN_DPAD_RIGHT; }
			}

			//g_current_config.g_n64_mapping_mode = MODE_TEST;

			switch (g_current_config.g_n64_mapping_mode)
			{
				case MODE_TEST:
				case MODE_N64_STANDARD:
					if (src->n64.buttons & N64_BTN_Z) { dst->buttons |= CPAD_BTN_ZL | CPAD_BTN_ZR; }
					if (src->n64.buttons & N64_BTN_R) { dst->buttons |= CPAD_BTN_TRIG_RIGHT; }
					if (src->n64.buttons & N64_BTN_L) { dst->buttons |= CPAD_BTN_TRIG_LEFT; }

					// Pushing all C directions at once sends X and Y down (wii64 menu)
					// This is made to work only in standard mode because that is the mode
					// suitable for wii64.
					if (IS_SIMULTANEOUS(src->n64.buttons,
							N64_BTN_C_RIGHT | N64_BTN_C_LEFT | N64_BTN_C_UP | N64_BTN_C_DOWN | N64_BTN_DPAD_LEFT | N64_BTN_Z)) {
						dst->buttons |= CPAD_BTN_Y|CPAD_BTN_X;
					}
					break;
				case MODE_MARIOKART64:
					if (src->n64.buttons & N64_BTN_Z) { dst->buttons |= CPAD_BTN_TRIG_LEFT; }
					if (src->n64.buttons & N64_BTN_R) { dst->buttons |= CPAD_BTN_TRIG_RIGHT; }
					if (src->n64.buttons & N64_BTN_L) { dst->buttons |= CPAD_BTN_ZL | CPAD_BTN_ZR; }
					break;
				case MODE_OCARINA:
					if (src->n64.buttons & N64_BTN_Z) { dst->buttons |= CPAD_BTN_TRIG_LEFT; }
					if (src->n64.buttons & N64_BTN_R) { dst->buttons |= CPAD_BTN_TRIG_RIGHT; }
					if (src->n64.buttons & N64_BTN_L) { dst->buttons |= CPAD_BTN_DPAD_DOWN; }
					break;
				case MODE_SSMB:
					if (src->n64.buttons & N64_BTN_L) { dst->buttons |= CPAD_BTN_DPAD_DOWN; }
					if (src->n64.buttons & N64_BTN_R) { dst->buttons |= CPAD_BTN_ZL | CPAD_BTN_ZR; }
					if (src->n64.buttons & N64_BTN_Z) { dst->buttons |= CPAD_BTN_TRIG_LEFT | CPAD_BTN_TRIG_RIGHT; }
					if (src->n64.buttons & N64_BTN_C_LEFT) { dst->buttons |= CPAD_BTN_Y; }
					if (src->n64.buttons & N64_BTN_C_DOWN) { dst->buttons |= CPAD_BTN_X; }
					break;
				case MODE_SIN_AND_PUNISHMENT:
					if (src->n64.buttons & N64_BTN_L) { dst->buttons |= CPAD_BTN_ZL | CPAD_BTN_ZR; }
					if (src->n64.buttons & N64_BTN_R) { dst->buttons |= CPAD_BTN_TRIG_RIGHT; }
					if (src->n64.buttons & N64_BTN_Z) { dst->buttons |= CPAD_BTN_TRIG_LEFT; }
					if (src->n64.buttons & N64_BTN_C_LEFT) { dst->buttons |= CPAD_BTN_Y; }
					if (src->n64.buttons & N64_BTN_C_RIGHT) { dst->buttons |= CPAD_BTN_X; }
					break;
				case MODE_OGRE_BATTLE:
					if (src->n64.buttons & N64_BTN_L) { dst->buttons |= CPAD_BTN_TRIG_LEFT; }
					if (src->n64.buttons & N64_BTN_R) { dst->buttons |= CPAD_BTN_TRIG_RIGHT; }
					if (src->n64.buttons & N64_BTN_Z) { dst->buttons |= CPAD_BTN_TRIG_LEFT; }
					break;
				case MODE_F_ZERO_X: // Mapping 6
					if (src->n64.buttons & N64_BTN_L) { dst->buttons |= CPAD_BTN_DPAD_RIGHT; }
					if (src->n64.buttons & N64_BTN_R) { dst->buttons |= CPAD_BTN_TRIG_RIGHT; }
					if (src->n64.buttons & N64_BTN_Z) { dst->buttons |= CPAD_BTN_TRIG_LEFT; }
					if (src->n64.buttons & N64_BTN_C_DOWN) { dst->buttons |= CPAD_BTN_X; }
					if (src->n64.buttons & N64_BTN_C_LEFT) { dst->buttons |= CPAD_BTN_Y; }
					if (src->n64.buttons & N64_BTN_C_RIGHT) { dst->buttons |= CPAD_BTN_ZR | CPAD_BTN_ZL; }
					break;
				case MODE_YOSHI_STORY: // Mapping 7
					if (src->n64.buttons & N64_BTN_L) { dst->buttons |= CPAD_BTN_ZL | CPAD_BTN_ZR; }
					if (src->n64.buttons & N64_BTN_R) { dst->buttons |= CPAD_BTN_TRIG_RIGHT; }
					if (src->n64.buttons & N64_BTN_Z) { dst->buttons |= CPAD_BTN_X | CPAD_BTN_Y; }
					break;
			}

			if (g_current_config.g_n64_mapping_mode != MODE_TEST)
			{
				switch (g_current_config.g_n64_mapping_mode)
				{
					case MODE_SIN_AND_PUNISHMENT:
						// In sin and punishment, both the left and right analog sticks on the wii classic
						// controller are used for aim. On a N64 controller, C-left and C-right are used to
						// move. Up down are not used and we do not want accidental button presses there to
						// mess with aiming!
					case MODE_F_ZERO_X:
						// In F-Zero, 3 of the C-stick directions are mapped to buttons.
						break;

					default:
						if (src->n64.buttons & N64_BTN_C_UP) { dst->ry = C_DEFLECTION; }
						if (src->n64.buttons & N64_BTN_C_DOWN) { dst->ry = -C_DEFLECTION; }
						if (src->n64.buttons & N64_BTN_C_LEFT) { dst->rx = -C_DEFLECTION; }
						if (src->n64.buttons & N64_BTN_C_RIGHT) { dst->rx = C_DEFLECTION; }
				}
			}
			else {
				static char active;
				// Special test mode
				if (src->n64.buttons & N64_BTN_C_UP) { if (!(active&1)) { test_y++; active |= 1; } } else { active &= ~1; }
				if (src->n64.buttons & N64_BTN_C_DOWN) { if (!(active&2)) { test_y--; active |= 2; } } else { active &= ~2; }
				if (src->n64.buttons & N64_BTN_C_LEFT) { if (!(active&4)) { test_x++; active |= 4; } } else { active &= ~4; }
				if (src->n64.buttons & N64_BTN_C_RIGHT) { if (!(active&8)) { test_x--; active |= 8; } } else { active &= ~8; }
			}

			if (g_current_config.g_n64_mapping_mode != MODE_TEST) {
				dst->lx = src->n64.x;
				dst->ly = src->n64.y;
			} else {
				if (src->n64.buttons & N64_BTN_DPAD_UP) { dst->ly = test_y; }
				if (src->n64.buttons & N64_BTN_DPAD_DOWN) { dst->ly = -test_y; }
				if (src->n64.buttons & N64_BTN_DPAD_LEFT) { dst->lx = test_x; }
				if (src->n64.buttons & N64_BTN_DPAD_RIGHT) { dst->lx = -test_x; }
			}

			if (isTripleClick(src->n64.buttons & N64_BTN_START)) {
				dst->buttons |= CPAD_BTN_HOME;
			}

			// Simulate L/R fully pressed values (like the analogue-less classic controller pro does)
			if (dst->buttons & CPAD_BTN_TRIG_LEFT) {
				dst->lt = 0xff;
			}
			if (dst->buttons & CPAD_BTN_TRIG_RIGHT) {
				dst->rt = 0xff;
			}

			/*
			if (src->n64.buttons & N64_BTN_C_UP) { 	dst->ly = 100; 		}
			if (src->n64.buttons & N64_BTN_C_DOWN) { 	dst->ly = -100; 		}
			if (src->n64.buttons & N64_BTN_C_LEFT) { 	dst->lx = -100; 		}
			if (src->n64.buttons & N64_BTN_C_RIGHT) { 	dst->lx = 100; 		}
			*/
			break;
#endif
	}
}
