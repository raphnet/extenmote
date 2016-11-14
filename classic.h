#ifndef _classic_h__
#define _classic_h__

#include "gamepads.h"

#define PACKED_CLASSIC_DATA_SIZE	17

/* The 6-byte report documented at http://wiibrew.org/wiki/Wiimote/Extension_Controllers/Classic_Controller */
#define CLASSIC_MODE_1	0
/* The 8-byte report with 8-bit axes */
#define CLASSIC_MODE_2	1
/* The 9-byte report with presumed 10-bit axes */
#define CLASSIC_MODE_3	2

void pack_classic_data(classic_pad_data *src, unsigned char dst[PACKED_CLASSIC_DATA_SIZE], int analog_style, int mode);
void dataToClassic(const gamepad_data *src, classic_pad_data *dst, char first_read);

#endif // _classic_h__

