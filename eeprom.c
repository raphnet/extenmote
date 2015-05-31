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

#include <avr/eeprom.h>
#include <string.h>
#include "eeprom.h"
#include "rlut.h"

static struct eeprom_data_struct g_eeprom_data;

static void eeprom_commit(void)
{
	eeprom_update_block(&g_eeprom_data, EEPROM_BASE_PTR, sizeof(struct eeprom_data_struct));
}

// return 1 if eeprom was blank
static char eeprom_init(void)
{
	char *magic = "EXTENMOTE";

	eeprom_read_block(&g_eeprom_data, EEPROM_BASE_PTR, sizeof(struct eeprom_data_struct));

	/* Check for magic number */
	if (memcmp(g_eeprom_data.magic, magic, EEPROM_MAGIC_SIZE)) {
		memset(&g_eeprom_data, 0, sizeof(struct eeprom_data_struct));
		memcpy(g_eeprom_data.magic, magic, EEPROM_MAGIC_SIZE);
		eeprom_commit();

		return 1;
	}
	return 0;
}

struct eeprom_data_struct g_current_config = {
	.magic = { 'E','X','T','E','N','M','O','T','E' },
	.g_n64_mapping_mode = MODE_N64_STANDARD,
	.g_n64_curve_id = RLUT_V1_5,

	.g_gc_mapping_mode = MODE_GC_STANDARD,

	.g_snes_nes_mode = 0,
	.g_snes_analog_dpad = 0,
};

void sync_config()
{
	memcpy(&g_eeprom_data, &g_current_config, sizeof(struct eeprom_data_struct));
	eeprom_commit();
}

void init_config()
{
	if (eeprom_init()) {
		// If 1 is returned, the eeprom was blank. Commit our default values.
		sync_config();
	}
	else {
		// otherwise, previously stored values have been loaded. make them active.
		memcpy(&g_current_config, &g_eeprom_data, sizeof(struct eeprom_data_struct));
	}
}

char disable_config = 0;

void chgMap(unsigned char *cfg_ptr, unsigned char new_value)
{
	if (!disable_config)
	{
		*cfg_ptr = new_value;
		sync_config();
	}
}
