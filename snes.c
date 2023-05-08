/* Nes/Snes/N64/Gamecube to Wiimote
 * Copyright (C) 2012 Rapha�l Ass�nat
 *
 * Based on earlier work:
 *
 * Nes/Snes/Genesis/SMS/Atari to USB
 * Copyright (C) 2006-2007 Rapha�l Ass�nat
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
 *
 *  Changes pertaining to SNES Mouse and NES lightgun  Copyright (C) 2023 Akerasoft
 *  The author may be contacted at robert.kolski@akerasoft.com
 */
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include <string.h>
#include "gamepads.h"
#include "snes.h"

// (C) Akerasoft 2023 -- BEGIN --
// to support the mouse it has to be 4 bytes
// 1 byte for NES
// 1 byte for GUN
// 2 bytes for SNES
// 4 bytes for Mouse
#define GAMEPAD_BYTES	4
// (C) Akerasoft 2023 -- END --

/******** IO port definitions **************/
#define SNES_LATCH_DDR	DDRC
#define SNES_LATCH_PORT	PORTC
#define SNES_LATCH_BIT	(1<<1)

#define SNES_CLOCK_DDR	DDRC
#define SNES_CLOCK_PORT	PORTC
#define SNES_CLOCK_BIT	(1<<0)

#define SNES_DATA_PORT	PORTC
#define SNES_DATA_DDR	DDRC
#define SNES_DATA_PIN	PINC
#define SNES_DATA_BIT	(1<<2)

/********* IO port manipulation macros **********/
#define SNES_LATCH_LOW()	do { SNES_LATCH_PORT &= ~(SNES_LATCH_BIT); } while(0)
#define SNES_LATCH_HIGH()	do { SNES_LATCH_PORT |= SNES_LATCH_BIT; } while(0)
#define SNES_CLOCK_LOW()	do { SNES_CLOCK_PORT &= ~(SNES_CLOCK_BIT); } while(0)
#define SNES_CLOCK_HIGH()	do { SNES_CLOCK_PORT |= SNES_CLOCK_BIT; } while(0)

#define SNES_GET_DATA()	(SNES_DATA_PIN & SNES_DATA_BIT)

/*********** prototypes *************/
static char snesInit(void);
static char snesUpdate(void);


// the most recent bytes we fetched from the controller
static unsigned char last_read_controller_bytes[GAMEPAD_BYTES];
// the most recently reported bytes
static unsigned char last_reported_controller_bytes[GAMEPAD_BYTES];

static char nes_mode = 0;
// (C) Akerasoft 2023 -- BEGIN --
static char gun_mode = 0;
static char mouse_mode = 0;
// (C) Akerasoft 2023 -- END --

static char snesInit(void)
{
	unsigned char sreg;
	sreg = SREG;
	cli();

	// clock and latch as output
	SNES_LATCH_DDR |= SNES_LATCH_BIT;
	SNES_CLOCK_DDR |= SNES_CLOCK_BIT;

	// data as input
	SNES_DATA_DDR &= ~(SNES_DATA_BIT);
	// enable pullup. This should prevent random toggling of pins
	// when no controller is connected.
	SNES_DATA_PORT |= SNES_DATA_BIT;

	// clock is normally high
	SNES_CLOCK_PORT |= SNES_CLOCK_BIT;

	// LATCH is Active HIGH
	SNES_LATCH_PORT &= ~(SNES_LATCH_BIT);

	snesUpdate();

	SREG = sreg;

	return 0;
}


/*
 *
       Clock Cycle     Button Reported
        ===========     ===============
        1               B
        2               Y
        3               Select
        4               Start
        5               Up on joypad
        6               Down on joypad
        7               Left on joypad
        8               Right on joypad
        9               A
        10              X
        11              L
        12              R
        13              none (always high)
        14              none (always high)
        15              none (always high)


// (C) Akerasoft 2023 -- BEGIN --
// Though this information came from:
// https://www.repairfaq.org/REPAIR/F_SNES.html
// So Rafael Assenat is the copyright holder anyway for this section

        16              none (always high) (SNES controller present)

        16              low - mouse present
        17              Y direction (0=up, 1=down)
        18              Y motion bit 6
        19              Y motion bit 5
        20              Y motion bit 4
        21              Y motion bit 3
        22              Y motion bit 2
        23              Y motion bit 1
        24              Y motion bit 0
        25              X direction (0=left, 1=right)
        26              X motion bit 6
        27              X motion bit 5
        28              X motion bit 4
        29              X motion bit 3
        30              X motion bit 2
        31              X motion bit 1
        32              X motion bit 0

// (C) Akerasoft 2023 -- END --

 *
 */

static char snesUpdate(void)
{
	int i;
	unsigned char tmp=0;

// (C) Akerasoft 2023 -- BEGIN --
	if (gun_mode)
	{
		tmp = (PIND & 0x0C) << 4;
		last_read_controller_bytes[0] = tmp;
		return 0;
	}
// (C) Akerasoft 2023 -- END --

	SNES_LATCH_HIGH();
	_delay_us(12);
	SNES_LATCH_LOW();

	for (i=0; i<8; i++)
	{
		_delay_us(6);
		SNES_CLOCK_LOW();

		tmp <<= 1;
		if (!SNES_GET_DATA()) { tmp |= 0x01; }

		_delay_us(6);

		SNES_CLOCK_HIGH();
	}
	last_read_controller_bytes[0] = tmp;
	for (i=0; i<8; i++)
	{
		_delay_us(6);

		SNES_CLOCK_LOW();

		tmp <<= 1;
		if (!SNES_GET_DATA()) { tmp |= 0x01; }

		_delay_us(6);
		SNES_CLOCK_HIGH();
	}
	last_read_controller_bytes[1] = tmp;
// (C) Akerasoft 2023 -- BEGIN --
	for (i=0; i<8; i++)
	{
		_delay_us(6);

		SNES_CLOCK_LOW();

		tmp <<= 1;
		if (!SNES_GET_DATA()) { tmp |= 0x01; }

		_delay_us(6);
		SNES_CLOCK_HIGH();
	}
	last_read_controller_bytes[2] = tmp;
	for (i=0; i<8; i++)
	{
		_delay_us(6);

		SNES_CLOCK_LOW();

		tmp <<= 1;
		if (!SNES_GET_DATA()) { tmp |= 0x01; }

		_delay_us(6);
		SNES_CLOCK_HIGH();
	}
	last_read_controller_bytes[3] = tmp;
// (C) Akerasoft 2023 -- END --

	return 0;
}

static char snesChanged(void)
{
	return memcmp(last_read_controller_bytes,
					last_reported_controller_bytes, GAMEPAD_BYTES);
}

static void snesGetReport(gamepad_data *dst)
{
	unsigned char h, l, d2, d3;

	if (dst != NULL)
	{
		l = last_read_controller_bytes[0];
		h = last_read_controller_bytes[1];
// (C) Akerasoft 2023 -- BEGIN --
		d2 = last_read_controller_bytes[2];
		d3 = last_read_controller_bytes[3];
// (C) Akerasoft 2023 -- END --

		// The 4 last bits are always high if an SNES controller
		// is connected. With a NES controller, they are low.
		// (High on the wire is a 0 here due to the snesUpdate() implementation)
		//

// (C) Akerasoft 2023 -- BEGIN --
		// The 3 just before last bits are high for the SNES mouse
		// but the very last bit in byte 2 is low.  So 0x1 is the expected value here
		if (l == 0x00 && h == 0x00 && d2 == 0x00 && d3 == 0x00)
		{
			gun_mode = 1;
			nes_mode = 0;
			mouse_mode = 0;
		} else if ((h & 0x0f) == 0x0f) {
			nes_mode = 1;
			gun_mode = 0;
			mouse_mode = 0;
		} else if ((h & 0x0f) == 0x01) {
			mouse_mode = 1;
			gun_mode = 0;
			nes_mode = 0;
		} else {
			// SNES Controller
			nes_mode = 0;
			mouse_mode = 0;
			gun_mode = 0;
		}
// (C) Akerasoft 2023 -- END --

		if (nes_mode) {
			// Nes controllers send the data in this order:
			// A B Sel St U D L R
			dst->nes.pad_type = PAD_TYPE_NES;
			dst->nes.buttons = l;
			dst->nes.raw_data[0] = l;
// (C) Akerasoft 2023 -- BEGIN --
		} else if (mouse_mode) {
			dst->mouse.pad_type = PAD_TYPE_MOUSE;
			dst->mouse.buttons = l;
			dst->mouse.buttons |= h<<8;
			dst->mouse.raw_data[0] = l;
			dst->mouse.raw_data[1] = h;
			dst->mouse.raw_data[2] = d2;
			dst->mouse.raw_data[3] = d3;
		} else if (gun_mode) {
			dst->gun.pad_type = PAD_TYPE_GUN;
			dst->gun.buttons = l;
			dst->gun.raw_data[0] = l;
// (C) Akerasoft 2023 -- END --
		} else {
			dst->nes.pad_type = PAD_TYPE_SNES;
			dst->snes.buttons = l;
			dst->snes.buttons |= h<<8;
			dst->snes.raw_data[0] = l;
			dst->snes.raw_data[1] = h;
		}
	}
	memcpy(last_reported_controller_bytes,
			last_read_controller_bytes,
			GAMEPAD_BYTES);
}

static Gamepad SnesGamepad = {
	.init		= snesInit,
	.update		= snesUpdate,
	.changed	= snesChanged,
	.getReport	= snesGetReport
};

Gamepad *snesGetGamepad(void)
{
	return &SnesGamepad;
}

