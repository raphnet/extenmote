/*  Extenmote : NES, SNES, N64 and Gamecube to Wii remote adapter firmware
 *  Copyright (C) 2012  Raphael Assenat <raph@raphnet.net>
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
#ifndef _gamepad_h__
#define _gamepad_h__

#define PAD_TYPE_NONE		0
#define PAD_TYPE_CLASSIC	1
#define PAD_TYPE_SNES		2
#define PAD_TYPE_NES		3
#define PAD_TYPE_N64		4
#define PAD_TYPE_GAMECUBE	5

#define NES_RAW_SIZE		1
#define SNES_RAW_SIZE		2
#define N64_RAW_SIZE		4
#define GC_RAW_SIZE			8
#define RAW_SIZE_MAX		GC_RAW_SIZE

/* Big thanks to the authors of 
 *
 * http://wiibrew.org/wiki/Wiimote/Extension_Controllers/Classic_Controller  */

typedef struct _classic_pad_data {
	unsigned char pad_type; // PAD_TYPE_CLASSIC
	char lx, ly; /* left analog stick */
	char rx, ry; /* right analog stick */
	char lt, rt; /* left and right triggers (sliders) */
	unsigned short buttons;

	// raphnet extension (raw N64/Gamecube values)
	unsigned char controller_id[2];
	unsigned char controller_raw_data[8];
} classic_pad_data;

#define CPAD_BTN_DPAD_RIGHT	0x8000
#define CPAD_BTN_DPAD_DOWN	0x4000
#define CPAD_BTN_DPAD_LEFT	0x0002
#define CPAD_BTN_DPAD_UP	0x0001

#define CPAD_BTN_MINUS		0x1000
#define CPAD_BTN_HOME		0x0800
#define CPAD_BTN_PLUS		0x0400

#define CPAD_BTN_TRIG_LEFT	0x2000
#define CPAD_BTN_TRIG_RIGHT	0x0200
#define CPAD_BTN_RSVD_HIGH	0x0100

#define CPAD_BTN_B			0x0040
#define CPAD_BTN_Y			0x0020
#define CPAD_BTN_A			0x0010
#define CPAD_BTN_X			0x0008

#define CPAD_BTN_ZL			0x0080
#define CPAD_BTN_ZR			0x0004


typedef struct _snes_pad_data {
	unsigned char pad_type; // PAD_TYPE_SNES
	unsigned short buttons;
	unsigned char raw_data[SNES_RAW_SIZE];
} snes_pad_data;

#define SNES_BTN_B			0x0080
#define SNES_BTN_Y			0x0040
#define SNES_BTN_SELECT		0x0020
#define SNES_BTN_START		0x0010
#define SNES_BTN_DPAD_UP	0x0008
#define SNES_BTN_DPAD_DOWN	0x0004
#define SNES_BTN_DPAD_LEFT	0x0002
#define SNES_BTN_DPAD_RIGHT	0x0001
#define SNES_BTN_A			0x8000
#define SNES_BTN_X			0x4000
#define SNES_BTN_L			0x2000
#define SNES_BTN_R			0x1000


typedef struct _nes_pad_data {
	unsigned char pad_type; // PAD_TYPE_NES
	unsigned char buttons;
	unsigned char raw_data[NES_RAW_SIZE];
} nes_pad_data;

#define NES_BTN_A			0x0080
#define NES_BTN_B			0x0040
#define NES_BTN_SELECT		0x0020
#define NES_BTN_START		0x0010
#define NES_BTN_DPAD_UP		0x0008
#define NES_BTN_DPAD_DOWN	0x0004
#define NES_BTN_DPAD_LEFT	0x0002
#define NES_BTN_DPAD_RIGHT	0x0001

typedef struct _n64_pad_data {
	unsigned char pad_type; // PAD_TYPE_N64
	char x,y;
	unsigned short buttons;
	unsigned char raw_data[N64_RAW_SIZE];
} n64_pad_data;

#define N64_BTN_A			0x0001
#define N64_BTN_B			0x0002
#define N64_BTN_Z			0x0004
#define N64_BTN_START		0x0008
#define N64_BTN_DPAD_UP		0x0010
#define N64_BTN_DPAD_DOWN	0x0020
#define N64_BTN_DPAD_LEFT	0x0040
#define N64_BTN_DPAD_RIGHT	0x0080

#define N64_BTN_RSVD_LOW1	0x0100
#define N64_BTN_RSVD_LOW2	0x0200

#define N64_BTN_L			0x0400
#define N64_BTN_R			0x0800
#define N64_BTN_C_UP		0x1000
#define N64_BTN_C_DOWN		0x2000
#define N64_BTN_C_LEFT		0x4000
#define N64_BTN_C_RIGHT		0x8000

typedef struct _gc_pad_data {
	unsigned char pad_type; // PAD_TYPE_GAMECUBE
	char x,y,cx,cy,lt,rt;
	unsigned short buttons;
	unsigned char raw_data[GC_RAW_SIZE];
} gc_pad_data;

#define GC_BTN_RSVD0		0x0001
#define GC_BTN_RSVD1		0x0002
#define GC_BTN_RSVD2		0x0004

#define GC_BTN_START		0x0008
#define GC_BTN_Y			0x0010
#define GC_BTN_X			0x0020
#define GC_BTN_B			0x0040
#define GC_BTN_A			0x0080

#define GC_BTN_RSVD3		0x0100

#define GC_BTN_L			0x0200
#define GC_BTN_R			0x0400
#define GC_BTN_Z			0x0800
#define GC_BTN_DPAD_UP		0x1000
#define GC_BTN_DPAD_DOWN	0x2000
#define GC_BTN_DPAD_RIGHT	0x4000
#define GC_BTN_DPAD_LEFT	0x8000

#define GC_ALL_BUTTONS		(GC_BTN_START|GC_BTN_Y|GC_BTN_X|GC_BTN_B|GC_BTN_A|GC_BTN_L|GC_BTN_R|GC_BTN_Z|GC_BTN_DPAD_UP|GC_BTN_DPAD_DOWN|GC_BTN_DPAD_RIGHT|GC_BTN_DPAD_LEFT)

typedef struct _gamepad_data {
	union {
		unsigned char pad_type; // PAD_TYPE_*
		classic_pad_data classic;
		snes_pad_data snes;
		nes_pad_data nes;
		n64_pad_data n64;
		gc_pad_data gc;
	};
} gamepad_data;


typedef struct {
	char (*init)(void);
	char (*update)(void);
	char (*changed)(void);
	void (*getReport)(gamepad_data *dst);
	void (*setVibration)(int value);
	char (*probe)(void);	
} Gamepad;

#define IS_SIMULTANEOUS(x,mask)	(((x)&(mask)) == (mask))

#endif // _gamepad_h__

