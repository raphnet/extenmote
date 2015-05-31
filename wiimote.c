/**
 * This file is based on:
 * http://code.google.com/p/circle-of-current/wiki/WiiExtensionLibrary
 *
 *
 * 2013-07-13:
 *   - Calibration data is now copied entirely
 *   - Add defines for known register names (from libOGC)
 */
#include <string.h>
#include "wiimote.h"
#include "wm_crypto.h"

// The following adapted from libOGC wiiuse_internal.h
#define WM_EXP_ID                   0xFA
#define WM_EXP_MOTION_PLUS_ENABLE   0xFE
#define WM_EXP_MEM_CALIBR           0x20
#define WM_EXP_MEM_KEY              0x40
#define WM_EXP_MEM_ENABLE2          0xFB
#define WM_EXP_MEM_ENABLE1          0xF0


// pointer to user function
static void (*wm_sample_event)();

static volatile unsigned char g_enc_on = 0;

// crypto data
static volatile unsigned char wm_rand[10];
static volatile unsigned char wm_key[6];
static volatile unsigned char wm_ft[8];
static volatile unsigned char wm_sb[8];

// virtual register
static volatile unsigned char twi_reg[256];
static volatile unsigned int twi_reg_addr;

static volatile unsigned char twi_first_addr_flag; // set address flag
static volatile unsigned char twi_rw_len; // length of most recent operation

static volatile unsigned char alt_id_set;
static volatile unsigned char alt_id_enabled;
static volatile unsigned char alt_id[6];
static volatile unsigned char default_id[6];

unsigned char wm_getReg(unsigned char reg)
{
	return twi_reg[reg];
}

static void twi_slave_init(unsigned char addr)
{
	// initialize stuff
	twi_reg_addr = 0;

	// set slave address
	TWAR = addr << 1;
	
	// enable twi module, acks, and twi interrupt
	TWCR = _BV(TWIE) | _BV(TWEA);
}

void twi_clear_int(unsigned char ack)
{
	// get ready by clearing interrupt, with or without ack
	if(ack != 0)
	{
		TWCR = _BV(TWEN) | _BV(TWIE) | _BV(TWINT) | _BV(TWEA);
	}
	else
	{
		TWCR = _BV(TWEN) | _BV(TWIE) | _BV(TWINT);
	}
}

/*

I'd like to thank Hector Martin for posting his encryption method!
His website is http://www.marcansoft.com/
Decryption method found at http://www.derkeiler.com/pdf/Newsgroups/sci.crypt/2008-11/msg00110.pdf

*/

unsigned char wm_ror8(unsigned char a, unsigned char b)
{
	// bit shift with roll-over
	return (a >> b) | ((a << (8 - b)) & 0xFF);
}

void wm_gentabs()
{
	unsigned char idx;
	unsigned char i;

	// check all idx
	for(idx = 0; idx < 7; idx++)
	{
		// generate test key
		unsigned char ans[6];
		unsigned char tkey[6];
		unsigned char t0[10];
		
		for(i = 0; i < 6; i++)
		{
			ans[i] = pgm_read_byte(&(ans_tbl[idx][i]));
		}	
		for(i = 0; i < 10; i++)
		{
			t0[i] = pgm_read_byte(&(sboxes[0][wm_rand[i]]));
		}
	
		tkey[0] = ((wm_ror8((ans[0] ^ t0[5]), (t0[2] % 8)) - t0[9]) ^ t0[4]);
		tkey[1] = ((wm_ror8((ans[1] ^ t0[1]), (t0[0] % 8)) - t0[5]) ^ t0[7]);
		tkey[2] = ((wm_ror8((ans[2] ^ t0[6]), (t0[8] % 8)) - t0[2]) ^ t0[0]);
		tkey[3] = ((wm_ror8((ans[3] ^ t0[4]), (t0[7] % 8)) - t0[3]) ^ t0[2]);
		tkey[4] = ((wm_ror8((ans[4] ^ t0[1]), (t0[6] % 8)) - t0[3]) ^ t0[4]);
		tkey[5] = ((wm_ror8((ans[5] ^ t0[7]), (t0[8] % 8)) - t0[5]) ^ t0[9]);

		// compare with actual key
		if(memcmp(tkey, (void*)wm_key, 6) == 0) break; // if match, then use this idx
	}
	if (idx == 7) {
		g_enc_on = 0;
		return;
	}

	// generate encryption from idx key and rand
	wm_ft[0] = pgm_read_byte(&(sboxes[idx + 1][wm_key[4]])) ^ pgm_read_byte(&(sboxes[idx + 2][wm_rand[3]]));
	wm_ft[1] = pgm_read_byte(&(sboxes[idx + 1][wm_key[2]])) ^ pgm_read_byte(&(sboxes[idx + 2][wm_rand[5]]));
	wm_ft[2] = pgm_read_byte(&(sboxes[idx + 1][wm_key[5]])) ^ pgm_read_byte(&(sboxes[idx + 2][wm_rand[7]]));
	wm_ft[3] = pgm_read_byte(&(sboxes[idx + 1][wm_key[0]])) ^ pgm_read_byte(&(sboxes[idx + 2][wm_rand[2]]));
	wm_ft[4] = pgm_read_byte(&(sboxes[idx + 1][wm_key[1]])) ^ pgm_read_byte(&(sboxes[idx + 2][wm_rand[4]]));
	wm_ft[5] = pgm_read_byte(&(sboxes[idx + 1][wm_key[3]])) ^ pgm_read_byte(&(sboxes[idx + 2][wm_rand[9]]));
	wm_ft[6] = pgm_read_byte(&(sboxes[idx + 1][wm_rand[0]])) ^ pgm_read_byte(&(sboxes[idx + 2][wm_rand[6]]));
	wm_ft[7] = pgm_read_byte(&(sboxes[idx + 1][wm_rand[1]])) ^ pgm_read_byte(&(sboxes[idx + 2][wm_rand[8]]));
	
	wm_sb[0] = pgm_read_byte(&(sboxes[idx + 1][wm_key[0]])) ^ pgm_read_byte(&(sboxes[idx + 2][wm_rand[1]]));
	wm_sb[1] = pgm_read_byte(&(sboxes[idx + 1][wm_key[5]])) ^ pgm_read_byte(&(sboxes[idx + 2][wm_rand[4]]));
	wm_sb[2] = pgm_read_byte(&(sboxes[idx + 1][wm_key[3]])) ^ pgm_read_byte(&(sboxes[idx + 2][wm_rand[0]]));
	wm_sb[3] = pgm_read_byte(&(sboxes[idx + 1][wm_key[2]])) ^ pgm_read_byte(&(sboxes[idx + 2][wm_rand[9]]));
	wm_sb[4] = pgm_read_byte(&(sboxes[idx + 1][wm_key[4]])) ^ pgm_read_byte(&(sboxes[idx + 2][wm_rand[7]]));
	wm_sb[5] = pgm_read_byte(&(sboxes[idx + 1][wm_key[1]])) ^ pgm_read_byte(&(sboxes[idx + 2][wm_rand[8]]));
	wm_sb[6] = pgm_read_byte(&(sboxes[idx + 1][wm_rand[3]])) ^ pgm_read_byte(&(sboxes[idx + 2][wm_rand[5]]));
	wm_sb[7] = pgm_read_byte(&(sboxes[idx + 1][wm_rand[2]])) ^ pgm_read_byte(&(sboxes[idx + 2][wm_rand[6]]));
	g_enc_on = 1;
}

void wm_slaveTxStart(unsigned char addr)
{
	if(addr >= 0x00 && addr < 0x06)
	{
		// call user event
		wm_sample_event();
	}
}

void wm_slaveRx(unsigned char addr, unsigned char l)
{
	unsigned int i;

	// if encryption data is sent, store them accordingly
	if(addr >= 0x40 && addr < 0x46)
	{
		for(i = 0; i < 6; i++)
		{
			wm_rand[9 - i] = twi_reg[0x40 + i];
		}
		//	wm_gentabs();
	}
	else if(addr >= 0x46 && addr < 0x4C)
	{
		for(i = 6; i < 10; i++)
		{
			wm_rand[9 - i] = twi_reg[0x40 + i];
		}
		for(i = 0; i < 2; i++)
		{
			wm_key[5 - i] = twi_reg[0x40 + 10 + i];
		}
		//	wm_gentabs();
	}
	else if(addr >= 0x4C && addr < 0x50)
	{
		for(i = 2; i < 6; i++)
		{
			wm_key[5 - i] = twi_reg[0x40 + 10 + i];
		}
		if (addr + l == 0x50) {
			// generate decryption once all data is loaded
			wm_gentabs();
		}
	}
}

void wm_newaction(unsigned char * d, unsigned char len)
{
	// load button data from user application
	memcpy((void*)twi_reg, d, len);
}

void wm_init(unsigned char * id, unsigned char * t, unsigned char len, unsigned char * cal_data, void (*function)(void))
{
	unsigned int i,j;

	// link user function
	wm_sample_event = function;

	// start state
	wm_newaction(t, len);
	twi_reg[WM_EXP_MEM_ENABLE1] = 0; // disable encryption

	// set id
	memcpy(default_id, id, 6);
	memcpy(twi_reg + WM_EXP_ID, default_id, 6);

	// set calibration data
	for(i = 0, j = WM_EXP_MEM_CALIBR; i < 32; i++, j++)
	{
		twi_reg[j] = cal_data[i];
	}

#ifdef USE_DEV_DETECT_PIN
	// initialize device detect pin
	dev_detect_port &= 0xFF ^ _BV(dev_detect_pin);
	dev_detect_ddr |= _BV(dev_detect_pin);
	_delay_ms(500); // delay to simulate disconnect
#endif

	// ready twi bus, no pull-ups
	twi_port &= 0xFF ^ _BV(twi_scl_pin);
	twi_port &= 0xFF ^ _BV(twi_sda_pin);
//	twi_ddr |= _BV(twi_scl_pin); // test: Pull clk while busy initializing

	// start twi slave, link events
	twi_slave_init(0x52);

#ifdef USE_DEV_DETECT_PIN
	// make the wiimote think something is connected
	dev_detect_port |= _BV(dev_detect_pin);
#endif
}

static char wm_started;

char wm_isStarted(void)
{
	return wm_started;
}

void wm_start(void)
{
	if (!wm_started) {
		// Start I2C
		TWCR |= _BV(TWEN);
		wm_started = 1;
	}
}

char wm_altIdEnabled(void)
{
	return alt_id_enabled;
}

void wm_setAltId(unsigned char id[6])
{
	memcpy(alt_id, id, 6);
	alt_id_set = 1;
}

ISR(TWI_vect)
{
	switch(TW_STATUS)
	{
		// Slave Rx
		case TW_SR_SLA_ACK: // addressed, returned ack
		case TW_SR_GCALL_ACK: // addressed generally, returned ack
		case TW_SR_ARB_LOST_SLA_ACK: // lost arbitration, returned ack
		case TW_SR_ARB_LOST_GCALL_ACK: // lost arbitration generally, returned ack
			// get ready to receive pointer
			twi_first_addr_flag = 0;
			// ack
			twi_clear_int(1);
			break;
		case TW_SR_DATA_ACK: // data received, returned ack
		case TW_SR_GCALL_DATA_ACK: // data received generally, returned ack
		if(twi_first_addr_flag != 0)
		{
			// put byte in register
			unsigned char t = TWDR;

			if ((twi_reg_addr == 0xF0) && (t == 0x55 || t == 0xAA)) {
				g_enc_on = 0;
				memcpy(twi_reg + WM_EXP_ID, default_id, 6);
				alt_id_enabled = 0;
			}

			// Writing 0x64 to register 0x00 after disabling encryption but
			// before reading the extension id enables an alternate extension
			// id. Adapted controller data is the reported as is.
			if ((twi_reg_addr == 0x00) && (t == 0x64) && alt_id_set) {
				memcpy(twi_reg + WM_EXP_ID, alt_id, 6);
				alt_id_enabled = 1;
			}
			
			if(g_enc_on ) // if encryption is on
			{
				// decrypt
				twi_reg[twi_reg_addr] = (t ^ wm_sb[twi_reg_addr % 8]) + wm_ft[twi_reg_addr % 8];
			}
			else
			{
				twi_reg[twi_reg_addr] = t;
			}
			twi_reg_addr++;
			twi_rw_len++;
		}
		else
		{
			// set address
			twi_reg_addr = TWDR;
			twi_first_addr_flag = 1;
			twi_rw_len = 0;
		}
		twi_clear_int(1); // ack
			break;
		case TW_SR_STOP: // stop or repeated start condition received
			// run user defined function
			wm_slaveRx(twi_reg_addr - twi_rw_len, twi_rw_len);
			twi_clear_int(1); // ack future responses
			break;
		case TW_SR_DATA_NACK: // data received, returned nack
		case TW_SR_GCALL_DATA_NACK: // data received generally, returned nack
			twi_clear_int(0); // nack back at master
			break;
		
		// Slave Tx
		case TW_ST_SLA_ACK:	// addressed, returned ack
		case TW_ST_ARB_LOST_SLA_ACK: // arbitration lost, returned ack
			// run user defined function
			wm_slaveTxStart(twi_reg_addr);
			twi_rw_len = 0;
		case TW_ST_DATA_ACK: // byte sent, ack returned
			// ready output byte
			if(g_enc_on) // encryption is on
			{
				// encrypt
				TWDR = (twi_reg[twi_reg_addr] - wm_ft[twi_reg_addr % 8]) ^ wm_sb[twi_reg_addr % 8];
			}
			else
			{
				TWDR = twi_reg[twi_reg_addr];
			}
			twi_reg_addr++;
			twi_rw_len++;
			twi_clear_int(1); // ack
			break;
		case TW_ST_DATA_NACK: // received nack, we are done 
		case TW_ST_LAST_DATA: // received ack, but we are done already!
			// ack future responses
			twi_clear_int(1);
			break;
		default:
			twi_clear_int(0);
			break;
	}
}



