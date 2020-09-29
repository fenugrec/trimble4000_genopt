/** (c) fenugrec 2019
 * Licensed under GPLv3
 *
 * Option password generator for front-panel activation of options, for Trimble 4000 series GPS receivers.
 *
 * Tested only on one unit, "works for me"
 *
 * see https://rplstoday.com/community/surveying-geomatics/help-please-trimble-4000ssi-configuration/paged/4/#post-503658
 *
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "stypes.h"

//decode shit to %5d%2d%2x", serno, optnum, newval !

const u8 key[]={7,7,5,5,9,8,1,4,2};

#define TEST_SERNO 20377
#define TEST_OPTNUM 6
#define TEST_NEWVAL 0

//reverse + invert lower 4 bits ??
u8 mangle_38d78(u8 val) {
	u8 mask = 0;
	u8 cnt = 0;

	for (; cnt < 4; cnt++) {
		mask <<= 1;
		if ( ((val >> cnt) & 1)) {
			mask ^= 1;
		}
	}
	return mask;
}



// 38DBE
u8 pwd_bitcount(u8 *ppass, u8 len)  {
	u8 out = 0;
	for (; len; len--) {
		// ppass[len] is within 00-0F.
		//count # of bits set ?
		u8 d2, tmp;
		for (d2=4; d2; d2--) {
			tmp = ppass[len-1];
			tmp = (tmp >> (d2-1)) & 1;
			out += tmp;
		}
	}
	return out & 0x0F;
}

u8 hexdig2asc(u8 val) {
	val &= 0x0F;
	if (val <= 9) {
		return 0x30 + val;
	}
	return 'A' + val - 10;
}

u8 hexdig(u8 asc) {
	if (asc <= '9') {
		return (asc - '0') & 0x0F;
	}
	return (0x0A + asc - 'A') & 0x0F;
}

//part of check_passwd. takes input ascii string, modifies in-place
void decode_loop(u8 *out, u8 *ppass, u32 serno) {
	u8 cnt;
	u8 dcnt ;

	//un-ascii
	for (cnt = 0; cnt < 10; cnt++) {
		ppass[cnt] = hexdig(ppass[cnt]);
	}

	u8 par = pwd_bitcount(ppass, 9) ;	//parity after unascii
	if (par != ppass[9]) {
		printf("bad par!\n");
	}

	for (cnt = 0, dcnt = 8; cnt < 9; cnt++, dcnt--) {
		u8 tmp = key[cnt];
		tmp = tmp ^ serno & 0x0F;
		//printf("dec: %02X->", ppass[cnt]);
		ppass[cnt] ^= tmp;
		tmp = mangle_38d78(ppass[cnt]);
		out[dcnt] = hexdig2asc(tmp);
		//printf("%02X\n", out[dcnt]);
	}
}

void encode_raw(u8 *out, u8 *optstring) {
	u8 cnt = 0;
	u8 dcnt = 8;
	u32 serno = 0;

	sscanf(optstring, "%5d", &serno);

	
	for (; cnt < 9; cnt++, dcnt--) {
		u8 tmp;
		//printf("enc: %02X->", optstring[dcnt]);
		tmp = hexdig(optstring[dcnt]); //start from the end
		tmp = mangle_38d78(tmp);
		out[cnt] = tmp ^ (serno & 0x0F) ^ key[cnt];
		//printf("%02X\n", out[cnt]);
	}

	out[9] = pwd_bitcount(out, 9);	//parity
	//printf("enc par=%x\n", out[9]);

	for (cnt = 0; cnt < 10; cnt++) {
		out[cnt] = hexdig2asc(out[cnt]);
	}

	return;
}

void encode(u8 *out, u32 serno, u8 optnum, u8 newval) {

	u8 optstring[11];
	sprintf(optstring, "%05d%02d%02x", serno, optnum, newval);
	encode_raw(out, optstring);
}

//round-trip test varying SN
void test_sn(void) {
	u8 pass[11];
	u8 shit2[11];

	u8 optstring[11];
	u32 serno;

	for (serno = 0; serno < 0x3FFF; serno++) {
		//just test enough to need 5 digits
		sprintf(optstring, "%05d%02d%02x", serno, TEST_OPTNUM, TEST_NEWVAL);
		encode(pass, serno, TEST_OPTNUM, TEST_NEWVAL);
		decode_loop(shit2, pass, serno);
		if (memcmp(shit2, optstring, 9) != 0) {
			printf("serno mismatch : %.9s, %.9s\n", optstring, shit2);
			break;
		}
	}
}

//round-trip test varying newval
void test_newval(void) {
	u8 pass[11];
	u8 shit2[11];

	u8 optstring[11];
	u8 newval;

	for (newval = 0; newval < 0x7F; newval++) {
		sprintf(optstring, "%05d%02d%02X", TEST_SERNO, TEST_OPTNUM, newval);
		encode(pass, TEST_SERNO, TEST_OPTNUM, newval);
		decode_loop(shit2, pass, TEST_SERNO);
		if (memcmp(shit2, optstring, 9) != 0) {
			printf("newval mismatch : %.9s, %.9s\n", optstring, shit2);
			break;
		}
	}
}

void testshit(void) {
	u8 tmp;

	//test round-trip for hex2asc2hex
	for (tmp=0; tmp <= 0x0F; tmp++) {
		u8 asc = hexdig2asc(tmp);
		if (tmp != hexdig(asc)) {
			printf("\t hex2asc mismatch\n");
			break;
		}
	}
	//test round-trip for mangle
	for (tmp=0; tmp<=0x0F; tmp++) {
		u8 mang = mangle_38d78(tmp);
		if (tmp != mangle_38d78(mang)) {
			printf("\t mangle_38d78 mismatch\n");
			break;
		}
	}

	test_sn();
	test_newval();
	return;
}


void usage(void) {
	printf("usage: genopt XXXXXYYZZ, where\n"
			"\t XXXXX=serial #, decimal\n"
			"\t YY=opt #, decimal\n"
			"\t ZZ=new value, hex\n\te.g. 2029904FF (sn 20299, option 04, newval=0xFF)\n");
}
	
int main(int argc, char **argv) {
	u8 out[11];
	testshit();

	if (argc != 2) {
		usage();
		return -1;
	}

	if (strlen(argv[1]) != 9) {
		printf("bad len\n");
		usage();
		return -1;
	}

	encode_raw(out, argv[1]);
	printf("pass: %.10s\n", out);
	return 0;
}