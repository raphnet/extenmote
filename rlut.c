#include "rlut.h"

#ifdef RLUT_V1_1
unsigned char rlut_v1_1[32] = {
	0,1,2,3,6,12,18,23,34,47,60,67,71,75,78,81,83,84 //,85,86,
};
#endif


unsigned char rlut_v1_4[32] = {
	0,1,2,3,6,12,18,22,31,40,50,56,60,64,67,69,70,71,72,73,74
};

// Good for Zelda!
unsigned char rlut_v1_5[32] = {
//	0,1,2,3,4,6,8,11,14,16,19,22,24,27,29,32,35,37,40,43,45,48,51,53,56,58,61,64,66,69,72,74,
//	0,1,2,3,4,6,8,10,12,14,16,18,25,32,38,44,51,57,61,62,63,64,65,66,67,68,69,70,71,72,73,74,
	0,1,2,3,
	4,6,8,10,
	12,14,16,18,
	25,32,38,44,
	52,60,61,62,
	63,64,65,66,
	67,68,69,70,
	71,72,73,74,
};

unsigned char rlut_gc1[32] = {
	0,1,4,7,11,15,19,23,27,31,37,43,50,57,63,69,73,77,80,83,86,89,92,94,96,97,98,99,100,

/*
	0,1,3,5,
	7,9,12,15,
	18,22,26,30,
	35,46,55,64,
	69,74,79,83,
	86,89,92,94,
	96,97,98,99,
	100,
*/
/*

	0,1,3,5,
	7,9,12,15,
	18,22,26,30,
	35,41,49,54,
	59,64,69,72,
	75,78,81,84,
	87,90,92,94,
	96,98,99,100,*/
};


unsigned char rlut7to5_convert(char input, unsigned char *lut, int lut_len)
{
	unsigned char i;

	if (input == 0)
		return 0;

	for (i=1; i<lut_len; i++) {
		if (lut[i] > input)
			return i-1;
	}
	return 0x1f;
}

unsigned char rlut7to5(char in, char version)
{
	switch(version)
	{
		default:
#ifdef RLUT_V1_1
		case RLUT_V1_1: 	return rlut7to5_convert(in, rlut_v1_1, sizeof(rlut_v1_1));
#endif
		case RLUT_V1_4: 	return rlut7to5_convert(in, rlut_v1_4, sizeof(rlut_v1_4));
		case RLUT_V1_5:		return rlut7to5_convert(in, rlut_v1_5, sizeof(rlut_v1_5));
		case RLUT_GC1:		return rlut7to5_convert(in, rlut_gc1, sizeof(rlut_gc1));
	}
}

