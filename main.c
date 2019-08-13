#include <stdint.h>
#include <stdio.h>

#include "stypes.h"

//decode shit to %5d%2d%2x", serno, optnum, newval !

const u8 key[]={7,7,5,5,9,8,1,4,2};
//const u32 serno = 20377;

#define TEST_SERNO 20377
#define TEST_OPTNUM 6
#define TEST_NEWVAL 55

//reverse + invert lower 4 bits ??
u8 mangle_38d78(u8 val) {
	u8 mask = 0;
	u8 cnt = 0;

	for (; cnt < 4; cnt++) {
		mask <<= 1;
		if ( !((val >> cnt) & 1)) {
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
		for (d2=3; d2; d2--) {
			tmp = ppass[len];
			tmp = (tmp >> d2) & 1;
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
	u8 cnt = 0;
	u8 dcnt = 8;

	for (; cnt < 9; cnt++, dcnt--) {
		u8 tmp = key[cnt];
		tmp = tmp ^ serno & 0x0F;
		printf("dec: %02X->", ppass[cnt]);
		ppass[cnt] ^= tmp;
		tmp = mangle_38d78(ppass[cnt]);
		out[dcnt] = hexdig2asc(tmp);
		printf("%02X\n", out[dcnt]);
	}
}

//TODO : parity
void encode(u8 *out, u32 serno, u8 optnum, u8 newval) {

	u8 optstring[11];
	sprintf(optstring, "%05d%02d%02x", serno, optnum, newval);
	printf("optstring: %s\n", optstring);

	u8 cnt = 0;
	u8 dcnt = 8;
	for (; cnt < 9; cnt++, dcnt--) {
		u8 tmp;
		printf("enc: %02X->", optstring[dcnt]);
		tmp = hexdig(optstring[dcnt]); //start from the end
		tmp = mangle_38d78(tmp);
		out[cnt] = tmp ^ (serno & 0x0F) ^ key[cnt];
		out[cnt] = hexdig2asc(out[cnt]);	//conv back to asc
		printf("%02X\n", out[cnt]);
	}

	u8 par = pwd_bitcount(out, 9) ;
	out[9] = hexdig2asc(par);
	return;
}

void testshit(void) {
	u8 tmp;
	u8 pass[11];
	u8 shit2[11];

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

	encode(pass, TEST_SERNO, TEST_OPTNUM, TEST_NEWVAL);	
	printf("pwd : %.10s\n", pass);
	decode_loop(shit2, pass, TEST_SERNO);
	printf("redecoded to %.9s\n", shit2);
	return;
}


int main(int argc, char **argv) {
	testshit();
	return 0;
}