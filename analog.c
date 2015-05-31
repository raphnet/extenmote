#include "rlut.h"


unsigned char applyCurve(char input, int curve_id)
{
	char x;
	unsigned char lx = 0x20;

#if defined(WITH_GAMECUBE)||defined(WITH_N64)
	x = input;

	if (x>=0) {
		lx = 0x20 + rlut7to5(x, curve_id);
	} else {
		lx = 0x20 - rlut7to5(-x, curve_id);
	}
#endif

	return lx;
}


