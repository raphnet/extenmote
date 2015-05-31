#ifndef _analog_h__
#define _analog_h__

#define ANALOG_STYLE_DEFAULT	0
#define ANALOG_STYLE_N64		1
#define ANALOG_STYLE_GC			2

unsigned char applyCurve(char input, int curve_id);

#endif
