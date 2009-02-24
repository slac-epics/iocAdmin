#include <stdio.h>
#include "basicIoOps.h"
#include <arpa/inet.h>
#include <netinet/in.h>

#define TYPE_32	unsigned
#define TYPE_16	unsigned short

TYPE_32
io32_be(TYPE_32 *addr, TYPE_32 val)
{
	out_be32(addr,val);
	return in_be32(addr);
}

TYPE_32
io32_le(TYPE_32 *addr, TYPE_32 val)
{
	out_le32(addr,val);
	return in_le32(addr);
}
TYPE_16
io16_be(TYPE_16 *addr, TYPE_16 val)
{
	out_be16(addr,val);
	return in_be16(addr);
}

TYPE_16
io16_le(TYPE_16 *addr, TYPE_16 val)
{
	out_le16(addr,val);
	return in_le16(addr);
}

unsigned char
io8(unsigned char *addr, unsigned char val)
{
	out_8(addr,val);
	return in_8(addr);
}

TYPE_32
b2ll(TYPE_32 arg)
{
int i;
union { TYPE_32 i; unsigned char c[sizeof(arg)];} u;
	u.i=arg;
	for (i=0; i < sizeof(arg); i++) {
		u.c[i] = arg;
		arg  >>= 8;
	}
	return u.i;
}

TYPE_16
b2ls(TYPE_16 arg)
{
int i;
union { TYPE_16 i; unsigned char c[sizeof(arg)];} u;
	u.i=arg;
	for (i=0; i < sizeof(arg); i++) {
		u.c[i] = arg;
		arg  >>= 8;
	}
	return u.i;
}

int
main(int argc, char **argv)
{
union   { int i; char c; } etest;
int	isbig;

TYPE_32		i=0xdeadbeef, res32, out32;
TYPE_16		s=0xcafe,     res16, out16;
unsigned char	c=0xf0,       res8,  out8;

	etest.i = 1;

	isbig = (etest.c == 0);
		
	printf("This is a %s-endian machine\n\n", isbig ? "big":"little");

	res32 = io32_be(&out32,i);
	printf("Writing BE 0x%08x: 0x%08x, read back 0x%08x\n", i, out32, res32);
	if (isbig)
	printf("  expected 0x%08x: 0x%08x, read back 0x%08x\n", i, i, i);
	else
	printf("  expected 0x%08x: 0x%08x, read back 0x%08x\n", i, htonl(i), i);

	printf("\n");

	res32 = io32_le(&out32,i);
	printf("Writing LE 0x%08x: 0x%08x, read back 0x%08x\n", i, out32, res32);
	if (isbig)
	printf("  expected 0x%08x: 0x%08x, read back 0x%08x\n", i, b2ll(i), i);
	else
	printf("  expected 0x%08x: 0x%08x, read back 0x%08x\n", i, i, i);

	printf("\n");

	res16 = io16_be(&out16,s);
	printf("Writing BE 0x%04x: 0x%04x, read back 0x%04x\n", s, out16, res16);
	if (isbig)
	printf("  expected 0x%04x: 0x%04x, read back 0x%04x\n", s, s, s);
	else
	printf("  expected 0x%04x: 0x%04x, read back 0x%04x\n", s, htons(s), s);

	printf("\n");

	res16 = io16_le(&out16,s);
	printf("Writing LE 0x%04x: 0x%04x, read back 0x%04x\n", s, out16, res16);
	if (isbig)
	printf("  expected 0x%04x: 0x%04x, read back 0x%04x\n", s, b2ls(s), s);
	else
	printf("  expected 0x%04x: 0x%04x, read back 0x%04x\n", s, s, s);

	printf("\n");

	res8  = io8(&out8,c);
	printf("Writing BYTE 0x%02x: 0x%02x, read back 0x%02x\n", c, out8, res8);
	printf("  expected   0x%02x: 0x%02x, read back 0x%02x\n", c, out8, res8);
	
return 0;
}
