/*  Extenmote : NES, SNES, N64 and Gamecube to Wii remote adapter firmware
 *  Copyright (C) 2012-2015  Raphael Assenat <raph@raphnet.net>
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
 *
 *  Changes pertaining to SNES Mouse and NES lightgun  Copyright (C) 2023 Akerasoft
 *  The author may be contacted at robert.kolski@akerasoft.com
 *
 */

#include <avr/io.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>
#include <math.h>
#include "wiimote.h"
#include "gcn64_protocol.h"
#include "snes.h"
#include "n64.h"
#include "gamecube.h"
#include "eeprom.h"
#include "classic.h"
#include "analog.h"
#include "db9.h"

static unsigned char classic_id[6] = { 0x00, 0x00, 0xA4, 0x20, 0x01, 0x01 };
#ifndef DB9_V2
static unsigned char adapter_n64_id[6] = { 0x00, 0x00, 0xA4, 0x20, 0x52, 0x64 };
static unsigned char adapter_gc_id[6] = { 0x00, 0x00, 0xA4, 0x20, 0x52, 0x47 };
static unsigned char adapter_snes_id[6] = { 0x00, 0x00, 0xA4, 0x20, 0x52, 0x10 };
static unsigned char adapter_nes_id[6] = { 0x00, 0x00, 0xA4, 0x20, 0x52, 0x08 };
#endif

// Classic controller (1)
// e1 15 82  e3 1a 7e  e3 1d 82  e4 1a 81  1a 18  7b d0
// Classic controller (2)
// e1 1b 7e  ea 1b 83  e5 1b 82  e4 16 80  26 22  9b f0
//

static unsigned char cal_data[32] = { 
		0xE0, 0x20, 0x80, // Left stick: Max X, Min X, Center X
		0xE0, 0x20, 0x80, // Left stick: Max Y, Min Y, Center Y
		0xE0, 0x20, 0x80, // Right stick: Max X, Min X, Center X
		0xE0, 0x20, 0x80, // Right stick: Max Y, Min Y, Center Y
		0x00, 0x00, 0, 0, 	// Shoulder Max? Min? checksum?

		0xE0, 0x20, 0x80, // Left stick: Max X, Min X, Center X
		0xE0, 0x20, 0x80, // Left stick: Max Y, Min Y, Center Y
		0xE0, 0x20, 0x80, // Right stick: Max X, Min X, Center X
		0xE0, 0x20, 0x80, // Right stick: Max Y, Min Y, Center Y
		0x00, 0x00, 0, 0,	// Shoulder Max? Min? checksum?
};

static volatile char performupdate;

static char db9_mode = 0;

static void hwInit(void)
{
	/* PORTD
	 * 0: out0
	 * 1: out0
// (C) Akerasoft 2023 - proposal PORT D2 and D3 are for NES Zapper.  When no other controller found
//                      use NES Zapper / lightgun in the SNES.c file.
	 * 2: in
	 * 3: in
// (C) Akerasoft 2023 - END
	 * 4: out1 // Tied to VCC on Multiuse PCB2 for routing reasons
	 * 5: out0
	 * 6: in-pu // Pulled to GND on mutluse db9 v3. Wired to VCC on multiuse PCB2.
	 * 7: out0
	 */
// (C) Akerasoft 2023 - proposal PORT D2 and D3 are for NES Zapper.
	PORTD = 0x6C;
// (C) Akerasoft 2023 - END

	DDRD = 0xbf;

	_delay_ms(1);

#ifdef DB9_V2
	db9_mode = 1;
#else
	if (!(PIND & 0x40)) {
		db9_mode = 1;
	}
#endif


	if (!db9_mode)
	{
		/* PORTC
		 *
		 * #  Signal			Direction/level
		 * 7: N/A				Out 0
		 * 6: N/A				Out 0
		 * 5: SCL (io)			In + pull-up
		 * 4: SDA (io)			In + upll-up
		 * 3: N64/Gamecube IO	in-pu
		 * 2: SNES/NES DATA		in-pu
		 * 1: SNES/NES LATCH	out1
		 * 0: SNES/NES CLK 		out1
		 *
		 * [*] Needs an external 1.5k pullup.
		 *
		 */
		PORTC = 0x07;
		DDRC = 0xc3;

		/* PORTB
		 *
		 * 0: out0
		 * 1: out0
		 * 2: out0
		 * 3: out0
		 * 4: out0
		 * 5: out0
		 * 6: out0
		 * 7: out0
		 *
		 */
		PORTB = 0x00;
		DDRB = 0xff;
	}
	else
	{
		DDRC = 0x00;
		PORTC = 0xff;
		DDRB = 0x00;
		PORTB = 0xff;
	}
}

static char initial_controller = PAD_TYPE_NONE;

static void do_earlyDetection()
{
	Gamepad *gamepad = NULL;
	unsigned char ngc;
	gamepad_data paddata;

//			initial_controller = PAD_TYPE_N64;
//			wm_setAltId(adapter_n64_id);
//			return;
#if defined(WITH_N64) || defined(WITH_GAMECUBE)
	ngc = gcn64_detectController();
	switch(ngc)
	{
		case CONTROLLER_IS_N64:
			initial_controller = PAD_TYPE_N64;
			wm_setAltId(adapter_n64_id);
			return;

		case CONTROLLER_IS_GC:
			initial_controller = PAD_TYPE_GAMECUBE;
			wm_setAltId(adapter_gc_id);
			return;
	}
#endif

	if (db9_mode) {
#ifdef WITH_DB9
		gamepad = db9GetGamepad();
		gamepad->update();
		gamepad->getReport(&paddata);
#endif
	} else {
#ifdef WITH_SNES
		gamepad = snesGetGamepad();
		gamepad->update();
		gamepad->getReport(&paddata);

		if (paddata.pad_type == PAD_TYPE_SNES) {
			initial_controller = PAD_TYPE_SNES;
			wm_setAltId(adapter_snes_id);
		} else {
			initial_controller = PAD_TYPE_NES;
			wm_setAltId(adapter_nes_id);
		}
#endif
	}
}


static void pollfunc(void)
{
	performupdate = 1;
}

#define ERROR_THRESHOLD			10

#define STATE_NO_CONTROLLER		0
#define STATE_CONTROLLER_ACTIVE	1

#define N64_GC_DETECT_PERIOD	120

int main(void)
{
	Gamepad *snes_gamepad = NULL;
	Gamepad *n64_gamepad = NULL;
	Gamepad *gc_gamepad	= NULL;
	Gamepad *db9_gamepad = NULL;
	Gamepad *cur_gamepad = NULL;
	Gamepad *default_gamepad = NULL;
	unsigned char mainState = STATE_NO_CONTROLLER;
	unsigned char analog_style = ANALOG_STYLE_DEFAULT;
	gamepad_data lastReadData;
	classic_pad_data classicData;
	unsigned char current_report[PACKED_CLASSIC_DATA_SIZE];
	int error_count = 0;
	char first_controller_read=0;
	int detect_time = 0;

	hwInit();
	init_config();
#if defined(WITH_N64) || defined(WITH_GAMECUBE)
	gcn64protocol_hwinit();
#endif

#ifdef WITH_SNES
	snes_gamepad = snesGetGamepad();
	default_gamepad = snes_gamepad;
#endif

#ifdef WITH_N64
	n64_gamepad = n64GetGamepad();
#endif

#ifdef WITH_GAMECUBE
	gc_gamepad = gamecubeGetGamepad();
#endif

#ifdef WITH_DB9
	db9_gamepad = db9GetGamepad();
	if (db9_mode) {
		db9_gamepad->init();
		default_gamepad = db9_gamepad;
	}
#endif

	dataToClassic(NULL, &classicData, 0);
	pack_classic_data(&classicData, current_report, ANALOG_STYLE_DEFAULT, CLASSIC_MODE_1);

	if (!db9_mode) {
		do_earlyDetection();
	}

	wm_init(classic_id, current_report, PACKED_CLASSIC_DATA_SIZE, cal_data, pollfunc);
	wm_start();
	sei();

	while(1)
	{
		// Adapter without sleep: 4mA
		// Adapter with sleep: 1.6mA

		set_sleep_mode(SLEEP_MODE_EXT_STANDBY);
		sleep_enable();
		sleep_cpu();
		sleep_disable();

		while (!performupdate) { }
		performupdate = 0;

		// With this delay, the controller read is postponed until just before
		// the next I2C read from the wiimote. This is to reduce latency to a
		// minimum (2.2ms at the moment).
		//
		// The N64 or Gamecube controller read will be corrupted if interrupted
		// by an I2C interrupt. The timing of the I2C read varies (menu vs in-game)
		// so we have to be careful not to let the N64/GC transaction overlap
		// with the I2C communication..
		//
		// This is why I chose to maintain a margin.
		//
		_delay_ms(2.35); // delay A

		//                                        |<----------- E ----------->|
		//                               C  -->|  |<--
		//         ____________________________    ________  / ______________________ ...
		// N64/GC:                             ||||         /
		//         _____  __    __    ___________________  / ____  __    __    _____ ...
		// I2C:         ||  ||||  ||||                    /      ||  ||||  ||||
		//                                               /
		//              |<---- D --->|
		//                      |<----- A ---->|
		//              |<-------------------B------------------>|
		// A = 2.35ms (Configured above)
		// B = 5ms (Wiimote classic controller poll rate)
		// C = 0.4ms (GC controller poll time)
		// D = 1.2ms, 1.1ms, 1.5ms (Wiimote I2C communication time. Varies [menu/game])
		// E = 2.34ms (menu), 2.84ms (in game)
		//

		switch(mainState)
		{
			default:
				mainState = STATE_NO_CONTROLLER;
				break;

			case STATE_NO_CONTROLLER:
				disable_config = 0;
				first_controller_read = 1;
#if defined(WITH_GAMECUBE) || defined(WITH_N64)
				if (!db9_mode) {
					detect_time++;
					if (detect_time > N64_GC_DETECT_PERIOD)
					{
						detect_time = 0;
						switch (gcn64_detectController())
						{
							case CONTROLLER_IS_N64:
								cur_gamepad = n64_gamepad;
								mainState = STATE_CONTROLLER_ACTIVE;
								analog_style = ANALOG_STYLE_N64;
								break;

							case CONTROLLER_IS_GC:
								cur_gamepad = gc_gamepad;
								mainState = STATE_CONTROLLER_ACTIVE;
								analog_style = ANALOG_STYLE_GC;
								break;
						}
					}
				}
#endif
				if (default_gamepad) {
					default_gamepad->update();
					default_gamepad->getReport(&lastReadData);
				}
				break;

			case STATE_CONTROLLER_ACTIVE:
				if (cur_gamepad->update()) {
					error_count++;
					if (error_count > 10)
						mainState = STATE_NO_CONTROLLER;
					break;
				}
				error_count = 0;
				cur_gamepad->getReport(&lastReadData);
				if (first_controller_read)
					first_controller_read++;
				break;
		}

		if (!wm_altIdEnabled())
		{
			unsigned char mode;

			switch(wm_getReg(0xFE))
			{
				default:
				case 0x01: mode = CLASSIC_MODE_1; break;
				case 0x03: mode = CLASSIC_MODE_3; break;
				case 0x02: mode = CLASSIC_MODE_2; break;
			}

			dataToClassic(&lastReadData, &classicData, first_controller_read);
			if (first_controller_read > 2) {
				first_controller_read = 0;
			}
			pack_classic_data(&classicData, current_report, analog_style, mode);
			wm_newaction(current_report, PACKED_CLASSIC_DATA_SIZE);
		}
		else
		{
			unsigned char raw[8];

			// Changing controller is not possible in this mode
			// unless we control the device_detect line.
			if (lastReadData.pad_type != initial_controller)
				 continue;

			switch (initial_controller)
			{
				case PAD_TYPE_GAMECUBE:
					memcpy(raw, lastReadData.gc.raw_data, sizeof(lastReadData.gc.raw_data));
					wm_newaction(raw, sizeof(lastReadData.gc.raw_data));
					break;

				case PAD_TYPE_N64:
					memcpy(raw, lastReadData.n64.raw_data, sizeof(lastReadData.n64.raw_data));
					wm_newaction(raw, sizeof(lastReadData.n64.raw_data));
					break;

				case PAD_TYPE_SNES:
					memcpy(raw, lastReadData.snes.raw_data, sizeof(lastReadData.snes.raw_data));
					wm_newaction(raw, sizeof(lastReadData.snes.raw_data));
					break;

				case PAD_TYPE_NES:
					memcpy(raw, lastReadData.nes.raw_data, sizeof(lastReadData.nes.raw_data));
					wm_newaction(raw, sizeof(lastReadData.nes.raw_data));
					break;

// (C) Akerasoft 2023 - BEGIN
				case PAD_TYPE_GUN:
					memcpy(raw, lastReadData.gun.raw_data, sizeof(lastReadData.gun.raw_data));
					wm_newaction(raw, sizeof(lastReadData.gun.raw_data));
					break;

				case PAD_TYPE_MOUSE:
					memcpy(raw, lastReadData.mouse.raw_data, sizeof(lastReadData.mouse.raw_data));
					wm_newaction(raw, sizeof(lastReadData.mouse.raw_data));
					break;
// (C) Akerasoft 2023 - END
			}

			// TODO : Controller specific report format
		}
	}

	return 0;
}
