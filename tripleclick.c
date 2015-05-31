#include <avr/io.h>

/* Due to the fact that the CPU stops to save power (see main.c), using
 * a timer is not possible.
 *
 * The effective delay heres depends on the polling frequency.
 */
#define TICKS_BETWEEN_TRANSITIONS	20
#define TRIGGERED_TICKS 20

char _isTripleClick(int b)
{
	static char transition_count = 0;
	static char ticks = 0;
	static char triggered = 0;
	static int last_b;

	if (triggered) {
		triggered--;
		return 1;
	}

	ticks++;
	if (ticks > TICKS_BETWEEN_TRANSITIONS) {
		transition_count = 0;
		ticks = 0;
		last_b = b;
		return 0;
	}

	if (b != last_b) {
		transition_count++;
		ticks = 0;
	}

	if (transition_count >= 6) {
		transition_count = 0;
		ticks = 0;
		triggered = TRIGGERED_TICKS;
		return 1;
	}

	last_b = b;
	return 0;
}

#define DEBOUNCE_CYCLES	4

char isTripleClick(int b)
{
	static int last_b = 0;
	static unsigned char debounce_cycles = 0;

	if (b != last_b) {
		debounce_cycles++;
		if (debounce_cycles > DEBOUNCE_CYCLES) {
			last_b = b;
			debounce_cycles = 0;
		}
	} else {
		debounce_cycles = 0;
	}

	return _isTripleClick(last_b);
}
