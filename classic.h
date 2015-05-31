#ifndef _classic_h__
#define _classic_h__

#include "gamepads.h"

#define PACKED_CLASSIC_DATA_SIZE	17

void pack_classic_data(classic_pad_data *src, unsigned char dst[15], int analog_style);
void dataToClassic(const gamepad_data *src, classic_pad_data *dst, char first_read);

#endif // _classic_h__

