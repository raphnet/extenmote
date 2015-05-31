/*  Extenmote : NES, SNES, N64 and Gamecube to Wii remote adapter firmware
 *  Copyright (C) 2012,2013  Raphael Assenat <raph@raphnet.net>
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
#ifndef _eeprom_h__
#define _eeprom_h__

#define EEPROM_MAGIC_SIZE		9 /* EXTENMOTE */
#define EEPROM_BASE_PTR			((void*)0x0000)

struct eeprom_data_struct {
	unsigned char magic[EEPROM_MAGIC_SIZE];
	unsigned char g_n64_mapping_mode;
	unsigned char g_n64_curve_id;
	unsigned char g_gc_mapping_mode;
	unsigned char g_snes_nes_mode;
	unsigned char g_snes_analog_dpad;
};

extern struct eeprom_data_struct g_current_config;

void sync_config(void);
void init_config(void);

#define MODE_N64_STANDARD		0
#define MODE_MARIOKART64		1
#define MODE_OCARINA			2
#define MODE_SSMB				3
#define MODE_SIN_AND_PUNISHMENT	4
#define MODE_OGRE_BATTLE		5
#define MODE_F_ZERO_X			6
#define MODE_YOSHI_STORY		7
#define MODE_TEST				8

#define MODE_GC_STANDARD		0
#define MODE_GC_SNES			1
#define MODE_GC_ZLR				2
#define MODE_GC_ASR				3 // Sega all stars racing transformed
#define MODE_GC_DEV				4
#define MODE_GC_EXTRA1			5

extern char disable_config;
void chgMap(unsigned char *cfg_ptr, unsigned char new_value);


#endif // _eeprom_h__

