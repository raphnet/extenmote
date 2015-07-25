/* Nes/Snes/Genesis/SMS/Atari to USB
 * Copyright (C) 2006-2015 Raphaël Assénat
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * The author may be contacted at raph@raphnet.net
 */
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include <string.h>
#include "gamepads.h"
#include "db9.h"

#define REPORT_SIZE		3
#define GAMEPAD_BYTES	3

#define CTL_ID_ATARI	0x00 // up/dn/lf/rt/btn_b
#define CTL_ID_SMS		0x01 // up/dn/lf/rt/btn_b/btn_c
#define CTL_ID_GENESIS3	0x02 // up/dn/lf/rt/btn_a/btn_b/btn_c/start
#define CTL_ID_GENESIS6 0x03 // all bits
#define CTL_ID_PADDLE	0x04	// HPD-200 Japanese

#define isGenesis(a) ( (a) >= CTL_ID_GENESIS3)

static unsigned char cur_id = CTL_ID_GENESIS3;

static inline unsigned char SAMPLE()
{
	unsigned char c;
	unsigned char b;
	unsigned char res;

	c = PINC;
	b = PINB;

	/* Target bits in 'res' are:
	 *
	 * 0: Up/Up/Z
	 * 1: Down/Down/Y
	 * 2: Left/0/X
	 * 3: Right/0
	 * 4: BtnB/BtnA
	 * 5: Btnc/BtnStart
	 * 6: Extra button (for direct wiring)
	 * 7:
	 */

	res = b & 0x0f;
	res |= (c & 0x3) << 4;

	return res;
}

#define SELECT_PORT		PORTC
#define SELECT_BIT		PORTC2
#define SELECT_DDR		DDRC

#define SET_SELECT()	SELECT_PORT |= 1<<SELECT_BIT;
#define CLR_SELECT()	SELECT_PORT &= ~(1<<SELECT_BIT);

/*********** prototypes *************/
static char db9Init(void);
static char db9Update(void);


#define PADDLE_MAP_HORIZ	0
#define PADDLE_MAP_VERT		1
#define PADDLE_MAP_1BTN		0
#define PADDLE_MAP_2BTN		2

static db9_pad_data last_read_state, last_reported_state;

#define READ_CONTROLLER_SIZE 5
static void readController(unsigned char bits[READ_CONTROLLER_SIZE])
{
	unsigned char a,b,c,d,e;

	// total delays: 160 microseconds.

	/* |   1 |  2  |  3  |  4  | 5 ...
	 * ___    __    __    __    __
	 *    |__|  |__|  |__|  |__|
	 *  ^  ^     ^     ^   ^
	 *  A  B     D     E   C
	 *
	 *  ABC are used when reading controllers.
	 *  D and E are used for auto-detecting the genesis 6 btn controller.
	 *
	 */

	DDRC |= 0x08;
	PORTC |= 0x08;

	/* 1 */
	SET_SELECT();
	_delay_us(20);
	a = SAMPLE();

	PORTC &= ~0x08;

	if (cur_id == CTL_ID_ATARI ||
		cur_id == CTL_ID_SMS) {
		bits[0] = a;
		bits[1] = 0xff;
		bits[2] = 0xff;

		return;
	}

	CLR_SELECT();
	_delay_us(20);		
	b = SAMPLE();

	/* 2 */
	SET_SELECT();
	_delay_us(20);		
	CLR_SELECT();
	_delay_us(20);		
	d = SAMPLE();

	/* 3 */
	SET_SELECT();
	_delay_us(20);		
	CLR_SELECT();
	_delay_us(20);			
	e = SAMPLE();

	/* 4 */
	SET_SELECT();
	_delay_us(20);			
	c = SAMPLE();

	CLR_SELECT();
	_delay_us(20);			

	/* 5 */
	SET_SELECT();

	bits[0] = a;
	bits[1] = b;
	bits[2] = c;
	bits[3] = d;
	bits[4] = e;
}

static char db9Init(void)
{
	unsigned char bits[READ_CONTROLLER_SIZE];

	SELECT_DDR |= (1<<SELECT_BIT);
	SELECT_PORT |= (1<<SELECT_BIT);

	// Directions as input with pull-up
	DDRB &= 0xF0;
	PORTB |= 0x0F;

	// Buttons B/A and C/Start
	DDRC &= 0xFC;
	PORTC |= 0x03;

	readController(bits);

	cur_id = CTL_ID_SMS;

	if ((bits[0]&0xf) == 0xf) {
		if ((bits[1]&0xf) == 0x3) 
		{
			if (	((bits[3] & 0xf) != 0x3)  ||
					((bits[4] & 0xf) != 0x3) ) {
				/* 6btn controllers return 0x0 and 0xf here. But
				 * for greater compatibility, I only test
				 * if it is different from a 3 button controller. */
				cur_id = CTL_ID_GENESIS6;
			}
			else {
				cur_id = CTL_ID_GENESIS3;
			}
		}
	}

	/* Force 6 Button genesis controller. Useful if auto-detection
	 * fails for some reason. */
	if (!(bits[1] & 0x20)) { // if start button initially held down
		cur_id = CTL_ID_GENESIS6;
	}

	db9Update();

	return 0;
}

static char db9Update(void)
{
	unsigned char data[READ_CONTROLLER_SIZE];

	/* 0: Up//Z
	 * 1: Down//Y
	 * 2: Left//X
	 * 3: Right//Mode
	 * 4: Btn B/A
	 * 5: Btn C/Start/
	 */
	readController(data);

	/* Buttons are active low. Invert the bits
	 * here to simplify subsequent 'if' statements... */
	data[0] = data[0] ^ 0xff;
	data[1] = data[1] ^ 0xff;
	data[2] = data[2] ^ 0xff;

	memset(&last_read_state, 0, sizeof(db9_pad_data));

	if (data[0] & 1) { last_read_state.buttons |= DB9_BTN_DPAD_UP; }
	if (data[0] & 2) { last_read_state.buttons |= DB9_BTN_DPAD_DOWN; }
	if (data[0] & 4) { last_read_state.buttons |= DB9_BTN_DPAD_LEFT; }
	if (data[0] & 8) { last_read_state.buttons |= DB9_BTN_DPAD_RIGHT; }

	if (isGenesis(cur_id)) {
		last_read_state.pad_type = PAD_TYPE_MD;

		if (data[1]&0x10) { last_read_state.buttons |= DB9_BTN_A; } // A
		if (data[0]&0x10) { last_read_state.buttons |= DB9_BTN_B; } // B
		if (data[0]&0x20) { last_read_state.buttons |= DB9_BTN_C; } // C
		if (data[1]&0x20) { last_read_state.buttons |= DB9_BTN_START; } // Start
		if (cur_id == CTL_ID_GENESIS6) {
			if (data[2]&0x04) { last_read_state.buttons |= DB9_BTN_X; } // X
			if (data[2]&0x02) { last_read_state.buttons |= DB9_BTN_Y; } // Y
			if (data[2]&0x01) { last_read_state.buttons |= DB9_BTN_Z; } // Z
			if (data[2]&0x08) { last_read_state.buttons |= DB9_BTN_MODE; } // Mode
		}
	}
	else {
		last_read_state.pad_type = PAD_TYPE_SMS;

		/* The button IDs for 1 and 2 button joysticks should start
		 * at '1'. Some Atari emulators don't support button remapping so
		 * this is pretty important! */
		if (data[0]&0x10) { last_read_state.buttons |= DB9_BTN_B; } // Button 1
		if (data[0]&0x20) { last_read_state.buttons |= DB9_BTN_C; } // Button 2
	}

	return 0;
}

static char db9Changed(void)
{
	return memcmp(&last_read_state, &last_reported_state, sizeof(db9_pad_data));
}

static void db9Report(gamepad_data *dst)
{
	if (dst) {
		memcpy(dst, &last_read_state, sizeof(db9_pad_data));
		memcpy(&last_reported_state, &last_read_state, sizeof(db9_pad_data));
	}
}

static Gamepad db9Gamepad = {
	.init		=	db9Init,
	.update		=	db9Update,
	.changed	=	db9Changed,
	.getReport	=	db9Report,
};

Gamepad *db9GetGamepad(void)
{
	return &db9Gamepad;
}

