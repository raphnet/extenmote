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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <time.h>

#include <linux/joystick.h>

char joyname[256];

int (*displayer)(int num_axes, short *axes, int num_buttons, short *buttons) = NULL;

int event_displayer(int num_axes, short *axes, int num_buttons, short *buttons)
{
	return 0;
}




static int generic_displayer(int num_axes, short *axes, int num_buttons, short *buttons)
{
	int i;
	char *comment;

	for (i=0; i<num_axes; i++) {
		comment = "";
		if (i==0) {
			if (axes[i]<0) comment = "left";
			if (axes[i]>0) comment = "right";
		} else if (i==1) {
			if (axes[i]<0) comment = "up";
			if (axes[i]>0) comment = "down";
		}
		printf("Axe %02d: %-6d  (%s)\n", i, axes[i], comment);
	}
	for (i=0; i<num_buttons; i++) {
		printf("Button %02d: %d\n", i, buttons[i]);
	}
	printf("-----------------------\n");
	return 0;
}

#define THR	10000

static const char *axePairToName(int h_axe, int v_axe)
{
	const char *winds[3][3] = {
		{ "L/U", "U", "R/U" },
		{ "L",   "C", "R"  },
		{ "L/D", "D", "R/D" },
	};
	int x,y;


	if (h_axe < -THR)
		x = 0;
	else if (h_axe > THR)
		x = 2;
	else 
		x = 1;
	
	if (v_axe < -THR) 
		y = 0;
	else if (v_axe > THR) 
		y = 2;
	else 
		y = 1;

	return winds[y][x];
}

static void printButton(const char *caption, int id, short *buttons)
{
	printf("%s %s\n", caption, buttons[id] ? "On" : "__");
}

static int wii_classic_displayer(int num_axes, short *axes, int num_buttons, short *buttons)
{
	int i;
	int disp_buttons = 15;
	const char *btn_labels[15] = {
		"Plus...:", // 0
		"Y......:", // 1
		"X......:", // 2
		"B......:", // 3

		"A......:", // 4
		"L......:", // 5
		"R......:", // 6
		"Zr.....:", // 7

		"D-up...:", // 8
		"D-dn...:", // 9
		"D-rt...:", // 10
		"D-lf...:", // 11

		"Zl.....:", // 12
		"Minus..:", // 13
		"Home...:"  // 14
	};
	int disp_order[15] = {
		4,3,2,1,5,6,12,7,8,9,10,11,0,13,14
	};

	printf("\033[0;0H");
	
	printf(" - %s (Wii classic controller)\n", joyname);

	printf("\n");

	printf("Left stick  : %6d,%6d    %3s\n", axes[0], axes[1], axePairToName(axes[0],axes[1]));
	
	printf("Right stick : %6d,%6d    %3s\n", axes[2], axes[3], axePairToName(axes[2],axes[3]));
	printf("                             \n");
	
	for (i=0; i<disp_buttons; i++) {
		int d;
		
		d = disp_order[i];

		if (d >= num_buttons) {
			break;
		}

		printButton(btn_labels[d], d, buttons);
	}

	return 0;
}

int main(int argc, char **argv)
{
	int fd;
	int offset_buttons = 0;
	char *jdev=NULL;
	int num_buttons=0, num_axes=0;

	short *axes=NULL, *buttons=NULL;

	if (argc<2) {
		printf("Usage: ./maptest device\n");
		printf("\n");
		printf("Example: ./maptest /dev/input/js0\n");
		return 1;
	}
	
	jdev = argv[1];
	printf("Using joystick device \"%s\"\n", jdev);
	
	while (1)
	{
reopen:
		fd = open(jdev, O_RDWR);
		if (fd==-1) {
			
			if (errno == EAGAIN) {
				usleep(250000);
				continue;
			}

			perror("open");

			break;
		}

		printf("\007\007\007"); fflush(stdout);

		memset(joyname, 0, sizeof(joyname));
		ioctl(fd, JSIOCGNAME(sizeof(joyname)), joyname);

		ioctl(fd, JSIOCGAXES, &num_axes);
		ioctl(fd, JSIOCGBUTTONS, &num_buttons);

		printf("Joystick name: %s\n", joyname);
		printf("Axes: %d\n", num_axes);
		printf("Buttons: %d\n", num_buttons);


		printf("\n\n************************\n");

		if (displayer == NULL) {
			do 
			{
				printf("Select displayer:\n");
				printf("A - Generic (page mode)\n");
				printf("B - Generic (event mode)\n");
				printf("C - Mapping tester\n");

				switch(getchar())
				{
					case 'a':
					case 'A':
						displayer = generic_displayer;
						break;

					case 'b':
					case 'B':
						displayer = event_displayer;
						break;

					case 'C':
					case 'c':
						displayer = wii_classic_displayer;
						break;

					default:
						break;
				}

			} while (!displayer);
		}

		axes = calloc(1, 2*num_axes);
		if (!num_axes) {
			perror("calloc");
			goto shdn;
		}

		buttons = calloc(1, 2*num_buttons);
		if (!num_buttons) {
			perror("calloc");
			goto shdn;
		}

		printf("\033[2J");

		while(1)
		{
			struct js_event ev;
			char *name = "?";

			if (read(fd, &ev, sizeof(struct js_event))<=0) {
				perror("read");
				close(fd);
				fd = -1;
				goto reopen;
			}


			switch(ev.type)
			{
				case JS_EVENT_BUTTON:
					name = "Button";
					if (ev.number>num_buttons) {
						fprintf(stderr, "Out of range button event!\n");
						break;
					}

					buttons[ev.number] = ev.value;

					break;
				case JS_EVENT_AXIS:
					name = "Axis";
					if (ev.number > num_axes) {
						fprintf(stderr, "Out of range axis event!\n");
						break;
					}
					
					axes[ev.number] = ev.value;

					break;
				case JS_EVENT_INIT:
					name = "Init";
					break;
			}

			if (displayer == event_displayer) {
				time_t t = time(NULL);
				struct tm tm;
				char timestamp[64];
				struct timespec ts;
				

				if (ev.type == JS_EVENT_AXIS && ev.number >= 4) {
					continue; // ignore L/R gamecube axis
				}

				localtime_r(&t, &tm);
				clock_gettime(CLOCK_MONOTONIC, &ts);

				strftime(timestamp, 64, "%T", &tm);				

				printf("%s.%3ld : %5s -> id=%-2d val=%d\n", 
					timestamp, ts.tv_nsec / 1000 / 1000, name, ev.number, ev.value);
				continue;
			}

			if (offset_buttons) {
				if (displayer(num_axes, axes, num_buttons-1, buttons+1))
					break;
			} else {
				if (displayer(num_axes, axes, num_buttons, buttons))
					break;
			}
		}

	}

shdn:
	free(axes);
	free(buttons);
	close(fd);

	return 0;
}

