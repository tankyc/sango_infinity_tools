/*

Big number library - arithmetic on multiple-precision unsigned integers.

This library is an implementation of arithmetic on arbitrarily large integers.

The difference between this and other implementations, is that the data structure
has optimal memory utilization (i.e. a 1024 bit integer takes up 128 bytes RAM),
and all memory is allocated statically: no dynamic allocation for better or worse.

Primary goals are correctness, clarity of code and clean, portable implementation.
Secondary goal is a memory footprint small enough to make it suitable for use in
embedded applications.


The current state is correct functionality and adequate performance.
There may well be room for performance-optimizations and improvements.

*/

#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include "bn.h"
#include <string.h>
#include <stdlib.h>

const DTYPE kuBase = 1000000000; // 10^9
const int kcchBase = 9;
DTYPE AddCarry(DTYPE* u1, DTYPE u2, DTYPE uCarry) {
	DTYPE_TMP uu = (DTYPE_TMP)(*u1) + u2 + uCarry;
	*u1 = (DTYPE)uu;
	return (DTYPE)(uu >> kcbitUint);
}
void ApplyCarry(struct bn* a, int iu, int validALen) {
	for (; ; iu++) {
		if (iu > validALen) {
			a->array[++validALen] = 1;
			break;
		}
		if (++a->array[iu] > 0)
			break;
	}
}
void ApplyBorrow(struct bn* a, int iuMin, int validALen)
{
	for (int iu = iuMin; iu <= validALen; iu++) {
		DTYPE u = a->array[iu]--;
		if (u > 0)
			return;
	}
}
DTYPE SubBorrow(DTYPE* u1, DTYPE u2, DTYPE uBorrow) {
	DTYPE_TMP uu = (DTYPE_TMP)*u1 - u2 - uBorrow;
	*u1 = (DTYPE)uu;
	return (DTYPE)-(int)(uu >> kcbitUint);
}
DTYPE SubRevBorrow(DTYPE* u1, DTYPE u2, DTYPE uBorrow) {
	DTYPE_TMP uu = (DTYPE_TMP)u2 - *u1 - uBorrow;
	*u1 = (DTYPE)uu;
	return (DTYPE)-(int)(uu >> kcbitUint);
}
DTYPE MulCarry(DTYPE* u1, DTYPE u2, DTYPE uCarry) {
	// This is guaranteed not to overflow.
	DTYPE_TMP uuRes = (DTYPE_TMP)(*u1) * u2 + uCarry;
	*u1 = (DTYPE)uuRes;
	return (DTYPE)(uuRes >> kcbitUint);
}
DTYPE AddMulCarry(DTYPE* uAdd, DTYPE uMul1, DTYPE uMul2, DTYPE uCarry) {
	// This is guaranteed not to overflow.
	DTYPE_TMP uuRes = (DTYPE_TMP)uMul1 * uMul2 + (*uAdd) + uCarry;
	*uAdd = (DTYPE)uuRes;
	return (DTYPE)(uuRes >> kcbitUint);
}
DTYPE GetLo(DTYPE_TMP uu) {
	return (DTYPE)uu;
}
DTYPE GetHi(DTYPE_TMP uu) {
	return (DTYPE)(uu >> kcbitUint);
}
int GetDiffLength(DTYPE* rgu1, DTYPE* rgu2, int cu)
{
	for (int iv = cu; --iv >= 0; )
	{
		if (rgu1[iv] != rgu2[iv])
			return iv + 1;
	}
	return 0;
}
int _Length(DTYPE* rgu)
{
	int cu = BN_ARRAY_SIZE;
	if (rgu[cu - 1] != 0)
		return cu;
	return cu - 1;
}
DTYPE_TMP _MakeUlong(DTYPE uHi, DTYPE uLo) {
	return ((DTYPE_TMP)uHi << 32) | uLo;
}

/* Functions for shifting number in-place. */
static void _lshift_one_bit(struct bn* a);
static void _rshift_one_bit(struct bn* a);
static void _lshift_word(struct bn* a, int nwords);
static void _rshift_word(struct bn* a, int nwords);



/* Public / Exported functions. */
void bignum_init(struct bn* n)
{
	require(n, "n is null");
	memset(n->array, 0, BN_ARRAY_MEMORY_SIZE);
}


void bignum_from_int(struct bn* n, DTYPE_TMP i)
{
	require(n, "n is null");

	bignum_init(n);

	/* Endianness issue if machine is not little-endian? */
#ifdef WORD_SIZE
#if (WORD_SIZE == 1)
	n->array[0] = (i & 0x000000ff);
	n->array[1] = (i & 0x0000ff00) >> 8;
	n->array[2] = (i & 0x00ff0000) >> 16;
	n->array[3] = (i & 0xff000000) >> 24;
#elif (WORD_SIZE == 2)
	n->array[0] = (i & 0x0000ffff);
	n->array[1] = (i & 0xffff0000) >> 16;
#elif (WORD_SIZE == 4)
	n->array[0] = i;
	DTYPE_TMP num_32 = 32;
	DTYPE_TMP tmp = i >> num_32; /* bit-shift with U64 operands to force 64-bit results */
	n->array[1] = tmp;
#endif
#endif
}

void bignum_from_double(struct bn* n, double d)
{
	require(n, "n is null");
	bignum_init(n);
	char dd[2048];
	sprintf(dd, "%.0f", d);
	bignum_from_string(n, dd);
}

int bignum_to_int(struct bn* n)
{
	require(n, "n is null");

	int ret = 0;

	/* Endianness issue if machine is not little-endian? */
#if (WORD_SIZE == 1)
	ret += n->array[0];
	ret += n->array[1] << 8;
	ret += n->array[2] << 16;
	ret += n->array[3] << 24;
#elif (WORD_SIZE == 2)
	ret += n->array[0];
	ret += n->array[1] << 16;
#elif (WORD_SIZE == 4)
	ret += n->array[0];
#endif

	return ret;
}


void bignum_from_hex_string(struct bn* n, char* str, int nbytes)
{
	require(n, "n is null");
	require(str, "str is null");
	require(nbytes > 0, "nbytes must be positive");
	require((nbytes & 1) == 0, "string format must be in hex -> equal number of bytes");
	require((nbytes % (sizeof(DTYPE) * 2)) == 0, "string length must be a multiple of (sizeof(DTYPE) * 2) characters");

	bignum_init(n);

	DTYPE tmp;                        /* DTYPE is defined in bn.h - uint{8,16,32,64}_t */
	int i = nbytes - (2 * WORD_SIZE); /* index into string */
	int j = 0;                        /* index into array */

	/* reading last hex-byte "MSB" from string first -> big endian */
	/* MSB ~= most significant byte / block ? :) */
	while (i >= 0)
	{
		tmp = 0;
		sscanf(&str[i], SSCANF_FORMAT_STR, &tmp);
		n->array[j] = tmp;
		i -= (2 * WORD_SIZE); /* step WORD_SIZE hex-byte(s) back in the string. */
		j += 1;               /* step one element forward in the array. */
	}
}

void bignum_to_hex_string(struct bn* n, char* str, int nbytes)
{
	require(n, "n is null");
	require(str, "str is null");
	require(nbytes > 0, "nbytes must be positive");
	require((nbytes & 1) == 0, "string format must be in hex -> equal number of bytes");

	int j = BN_ARRAY_SIZE - 1; /* index into array - reading "MSB" first -> big-endian */
	int i = 0;                 /* index into string representation. */

	/* reading last array-element "MSB" first -> big endian */
	while ((j >= 0) && (nbytes > (i + 1)))
	{
		sprintf(&str[i], SPRINTF_FORMAT_STR, n->array[j]);
		i += (2 * WORD_SIZE); /* step WORD_SIZE hex-byte(s) forward in the string. */
		j -= 1;               /* step one element back in the array. */
	}

	/* Count leading zeros: */
	j = 0;
	while (str[j] == '0')
	{
		j += 1;
	}

	/* Move string j places ahead, effectively skipping leading zeros */
	for (i = 0; i < (nbytes - j); ++i)
	{
		str[i] = str[i + j];
	}

	/* Zero-terminate string */
	str[i] = 0;
}

void bignum_dec(struct bn* n)
{
	require(n, "n is null");

	DTYPE tmp; /* copy of n */
	DTYPE res;

	int i;
	for (i = 0; i < BN_ARRAY_SIZE; ++i)
	{
		tmp = n->array[i];
		res = tmp - 1;
		n->array[i] = res;

		if (!(res > tmp))
		{
			break;
		}
	}
}

void bignum_inc(struct bn* n)
{
	require(n, "n is null");

	DTYPE res;
	DTYPE_TMP tmp; /* copy of n */

	int i;
	for (i = 0; i < BN_ARRAY_SIZE; ++i)
	{
		tmp = n->array[i];
		res = tmp + 1;
		n->array[i] = res;

		if (res > tmp)
		{
			break;
		}
	}
}

int _validLen(struct bn* n)
{
	int maxn = BN_ARRAY_SIZE - 1;
	for (; maxn >= 0; --maxn)
	{
		if (n->array[maxn] != 0)
			break;
	}
	return ++maxn;
}

int _valid_(struct bn* n)
{
	int maxn = BN_ARRAY_SIZE - 1;
	for (; maxn >= 0; --maxn)
	{
		if (n->array[maxn] != 0)
			break;
	}
	return maxn;
}

int CbitHighZero(DTYPE u) {
	if (u == 0)
		return 32;

	int cbit = 0;
	if ((u & 0xFFFF0000) == 0) {
		cbit += 16;
		u <<= 16;
	}
	if ((u & 0xFF000000) == 0) {
		cbit += 8;
		u <<= 8;
	}
	if ((u & 0xF0000000) == 0) {
		cbit += 4;
		u <<= 4;
	}
	if ((u & 0xC0000000) == 0) {
		cbit += 2;
		u <<= 2;
	}
	if ((u & 0x80000000) == 0)
		cbit += 1;
	return cbit;
}

int cu_non_zero(struct bn* n, int* outLen)
{
	int cu = 0;
	bool r = false;
	for (int maxn = BN_ARRAY_SIZE - 1; maxn >= 0; --maxn)
	{
		if (n->array[maxn] != 0)
		{
			if (!r)
			{
				*outLen = maxn;
				r = true;
			}
			cu++;
		}
	}
	return cu;
}

void bignum_add_int(struct bn* a, DTYPE b, struct bn* c, int validALen)
{
	int ai = validALen;
	if (ai < 0)
		ai = _valid_(a);

	if (c != a)
		memcpy(c->array, a->array, BN_ARRAY_MEMORY_SIZE);

	if (ai <= 0) {
		if ((c->array[0] += b) >= b)
			return;
		c->array[1] = 1;
		return;
	}

	if (b == 0)
		return;

	DTYPE uNew = c->array[0] + b;
	if (uNew < b) {
		// Have carry.
		ApplyCarry(c, 1, ai);
	}
	c->array[0] = uNew;
}
void bignum_add(struct bn* a, struct bn* b, struct bn* c)
{
	require(a, "a is null");
	require(b, "b is null");
	require(c, "c is null");

	int ai = _valid_(a);
	if (ai == -1)
	{
		memcpy(c->array, b->array, BN_ARRAY_MEMORY_SIZE);
		return;
	}

	int bi = _valid_(b);
	if (bi == -1)
	{
		memcpy(c->array, a->array, BN_ARRAY_MEMORY_SIZE);
		return;
	}

	if (bi == 0)
	{
		bignum_add_int(a, b->array[0], c, ai);
		return;
	}

	if (ai == 0)
	{
		bignum_add_int(b, a->array[0], c, bi);
		return;
	}

	if (a != c)
		memcpy(c->array, a->array, BN_ARRAY_MEMORY_SIZE);

	int cuAdd = bi + 1;
	if (ai < bi) {
		cuAdd = ai + 1;
		memcpy(c->array + cuAdd, b->array + cuAdd, WORD_SIZE * (bi - ai));
		ai = bi;
	}

	// Add, tracking carry.
	DTYPE uCarry = 0;
	for (int iu = 0; iu < cuAdd; iu++) {
		uCarry = AddCarry(&c->array[iu], b->array[iu], uCarry);
	}

	// Deal with extra carry.
	if (uCarry != 0)
		ApplyCarry(c, cuAdd, ai);

}

void bignum_sub_int(struct bn* a, DTYPE b, struct bn* c, int validALen, int* sign)
{
	if (b == 0) {
		if (a != c)
			memcpy(c->array, a->array, BN_ARRAY_MEMORY_SIZE);
		return;
	}

	int ai = validALen;
	if (ai < 0)
		ai = _valid_(a);
	if (ai == -1)
	{
		memset(c->array, 0, BN_ARRAY_MEMORY_SIZE);
		c->array[0] = b;
		return;
	}

	if (ai == 0)
	{
		if (b <= a->array[0])
			a->array[0] -= b;
		else {
			a->array[0] = b - a->array[0];
			*sign = -*sign;
		}
		return;
	}

	if (c != a)
	{
		memcpy(c->array, a->array, BN_ARRAY_MEMORY_SIZE);
	}

	DTYPE uTmp = c->array[0];
	c->array[0] = uTmp - b;
	if (uTmp < b) {
		ApplyBorrow(c, 1, ai);
	}
}

void bignum_sub_rev(struct bn* a, struct bn* b, struct bn* c, int* sign, int aLen, int bLen)
{
	int ai = aLen;
	if (ai < 0)
		ai = _valid_(a);

	int bi = bLen;
	if (bi < 0)
		bi = _valid_(b);

	if (a != c)
		memcpy(c->array, a->array, BN_ARRAY_MEMORY_SIZE);

	int cuSub = ai + 1;
	if (ai < bi) {
		memcpy(b->array + cuSub, c->array + cuSub, WORD_SIZE * (bi - ai));
		ai = bi;
	}

	DTYPE uBorrow = 0;
	for (int iu = 0; iu < cuSub; iu++) {
		uBorrow = SubRevBorrow(&c->array[iu], b->array[iu], uBorrow);
	}
	if (uBorrow != 0) {
		ApplyBorrow(c, cuSub, ai);
	}
}

void bignum_sub(struct bn* a, struct bn* b, struct bn* c, int* sign)
{
	require(a, "a is null");
	require(b, "b is null");
	require(c, "c is null");
	int bi = _valid_(b);
	if (bi == -1)
	{
		if (a != c)
			memcpy(c->array, a->array, BN_ARRAY_MEMORY_SIZE);
		return;
	}
	else if (bi == 0)
	{
		bignum_sub_int(a, b->array[0], c, -1, sign);
		return;
	}

	int ai = _valid_(a);
	if (ai == -1)
	{
		// 不支持0减大数
		memcpy(c->array, b->array, BN_ARRAY_MEMORY_SIZE);
		*sign = -*sign;
		return;
	}
	else if (ai == 0)
	{
		DTYPE u = a->array[0];
		bignum_sub_int(b, u, c, bi, sign);
		*sign = -*sign;
		return;
	}

	if (c != a)
	{
		memcpy(c->array, a->array, BN_ARRAY_MEMORY_SIZE);
	}

	if (ai < bi) {
		bignum_sub_rev(a, b, c, sign, ai, bi);
		*sign = -*sign;
		return;
	}

	int cuSub = bi + 1;
	if (ai == bi) {
		// Determine which is larger.
		int maxLen = ai;
		ai = GetDiffLength(a->array, b->array, ai + 1) - 1;
		if (ai < 0) {
			ai = 0;
			memset(c->array, 0, BN_ARRAY_MEMORY_SIZE);
			return;
		}

		memset(c->array + (ai+1), 0, BN_ARRAY_MEMORY_SIZE - sizeof(DTYPE) * (ai + 1));

		DTYPE u1 = c->array[ai];
		DTYPE u2 = b->array[ai];
		if (ai == 0) {
			
			if (u1 < u2) {
				c->array[0] = u2 - u1;
				*sign = -*sign;
			}
			else
				c->array[0] = u1 - u2;
			return;
		}

		if (u1 < u2) {
			bi = ai;
			bignum_sub_rev(a, b, c, sign, ai, bi);
			bi = cuSub - 1;
			*sign = -*sign;
			return;
		}
		cuSub = ai + 1;
	}

	// Subtract, tracking borrow.
	DTYPE uBorrow = 0;
	for (int iu = 0; iu < cuSub; iu++) {
		uBorrow = SubBorrow(&c->array[iu], b->array[iu], uBorrow);
	}
	if (uBorrow != 0) {
		ApplyBorrow(c, cuSub, ai);
	}
}

void bignum_mul_int(struct bn* a, DTYPE b, struct bn* c, int validALen)
{
	if (b == 0) {
		memset(c->array, 0, BN_ARRAY_MEMORY_SIZE);
		return;
	}
	if (b == 1)
	{
		if (a != c)
			memcpy(c->array, a->array, BN_ARRAY_MEMORY_SIZE);
		return;
	}

	int ai = validALen;
	if (ai < 0)
		ai = _valid_(a);
	if (ai == -1)
	{
		memset(c->array, 0, BN_ARRAY_MEMORY_SIZE);
		return;
	}

	if (ai == 0)
	{
		DTYPE_TMP val = (DTYPE_TMP)a->array[0] * b;
		DTYPE hi = GetHi(val);
		memset(c->array, 0, BN_ARRAY_MEMORY_SIZE);
		if (hi != 0)
		{
			c->array[1] = hi;
		}
		c->array[0] = (DTYPE)val;
		return;
	}

	if (c != a)
	{
		memcpy(c->array, a->array, BN_ARRAY_MEMORY_SIZE);
	}

	DTYPE uCarry = 0;
	for (int iu = 0; iu <= ai; iu++)
		uCarry = MulCarry(&c->array[iu], b, uCarry);

	if (uCarry != 0) {
		c->array[ai + 1] = uCarry;
	}

}

void bignum_mul(struct bn* a, struct bn* b, struct bn* c)
{
	require(a, "a is null");
	require(b, "b is null");
	require(c, "c is null");

	int ai = _valid_(a);
	if (ai == -1)
	{
		memset(c->array, 0, BN_ARRAY_MEMORY_SIZE);
		return;
	}

	int bi = _valid_(b);
	if (bi == -1)
	{
		memset(c->array, 0, BN_ARRAY_MEMORY_SIZE);
		return;
	}

	if (bi == 0)
	{
		bignum_mul_int(a, b->array[0], c, ai);
	}
	else if (ai == 0)
	{
		bignum_mul_int(b, a->array[0], c, bi);
	}
	else
	{
		// 
		if (c != a)
		{
			memcpy(c->array, a->array, BN_ARRAY_MEMORY_SIZE);
		}

		int cuBase = ai + 1;
		for (int iu = cuBase; --iu >= 0; ) {
			DTYPE uMul = c->array[iu];
			c->array[iu] = 0;
			DTYPE uCarry = 0;
			for (int iuSrc = 0; iuSrc <= bi; iuSrc++)
				uCarry = AddMulCarry(&c->array[iu + iuSrc], b->array[iuSrc], uMul, uCarry);
			if (uCarry != 0) {
				for (int iuDst = iu + bi + 1; uCarry != 0 && iuDst <= ai; iuDst++)
					uCarry = AddCarry(&c->array[iuDst], 0, uCarry);
				if (uCarry != 0) {
					c->array[cuBase] = uCarry;
				}
			}
		}
	}

}


DTYPE DivMod(Bn* a, DTYPE uDen, int aLen) {
	if (uDen == 1)
		return 0;
	int ai = aLen;
	if (ai < 0)
		ai = _valid_(a);

	if (ai == 0) {
		DTYPE uTmp = a->array[0];
		a->array[0] = uTmp / uDen;
		return uTmp % uDen;
	}

	DTYPE_TMP uu = 0;
	for (int iv = ai; iv >= 0; iv--) {
		uu = _MakeUlong((DTYPE)uu, a->array[iv]);
		a->array[iv] = (DTYPE)(uu / uDen);
		uu %= uDen;
	}
	return (DTYPE)uu;
}

void ModDivCore(Bn* a, int aLen, Bn* b, int bLen, Bn* c, bool fQuo, Bn* quo) {
	int ai = aLen;
	if (ai < 0)
		ai = _valid_(a);

	int bi = bLen;
	if (bi < 0)
		bi = _valid_(b);

	if (ai < bi)
	{
		memset(c->array, 0, BN_ARRAY_MEMORY_SIZE);
		if (fQuo)
			memset(quo->array, 0, BN_ARRAY_MEMORY_SIZE);
		return;
	}

	if (c != a)
		memcpy(c->array, a->array, BN_ARRAY_MEMORY_SIZE);

	int cuDen = bi + 1;
	int cuDiff = ai - bi;

	// Determine whether the result will have cuDiff "digits" or cuDiff+1 "digits".
	int cuQuo = cuDiff;
	for (int iu = ai; ; iu--) {
		if (iu < cuDiff) {
			cuQuo++;
			break;
		}
		if (b->array[iu - cuDiff] != c->array[iu]) {
			if (b->array[iu - cuDiff] < c->array[iu])
				cuQuo++;
			break;
		}
	}

	if (cuQuo == 0)
		return;

	if (fQuo)
		memset(quo->array, 0, BN_ARRAY_MEMORY_SIZE);

	// Get the uint to use for the trial divisions. We normalize so the high bit is set.
	DTYPE uDen = b->array[cuDen - 1];
	DTYPE uDenNext = b->array[cuDen - 2];
	int cbitShiftLeft = CbitHighZero(uDen);
	int cbitShiftRight = kcbitUint - cbitShiftLeft;
	if (cbitShiftLeft > 0) {
		uDen = (uDen << cbitShiftLeft) | (uDenNext >> cbitShiftRight);
		uDenNext <<= cbitShiftLeft;
		if (cuDen > 2)
			uDenNext |= b->array[cuDen - 3] >> cbitShiftRight;
	}
	//Contract.Assert((uDen & 0x80000000) != 0);

	// Allocate and initialize working space.
	//Contract.Assert(cuQuo + cuDen == regNum._iuLast + 1 || cuQuo + cuDen == regNum._iuLast + 2);
	for (int iu = cuQuo; --iu >= 0; ) {
		// Get the high (normalized) bits of regNum.
		DTYPE uNumHi = (iu + cuDen <= ai) ? c->array[iu + cuDen] : 0;
		//Contract.Assert(uNumHi <= regDen._rgu[cuDen - 1]);

		DTYPE_TMP uuNum = _MakeUlong(uNumHi, c->array[iu + cuDen - 1]);
		DTYPE uNumNext = c->array[iu + cuDen - 2];
		if (cbitShiftLeft > 0) {
			uuNum = (uuNum << cbitShiftLeft) | (uNumNext >> cbitShiftRight);
			uNumNext <<= cbitShiftLeft;
			if (iu + cuDen >= 3)
				uNumNext |= c->array[iu + cuDen - 3] >> cbitShiftRight;
		}

		// Divide to get the quotient digit.
		DTYPE_TMP uuQuo = uuNum / uDen;
		DTYPE_TMP uuRem = (DTYPE)(uuNum % uDen);
		//Contract.Assert(uuQuo <= (DTYPE_TMP)MAX_VAL + 2);
		if (uuQuo > MAX_VAL) {
			uuRem += uDen * (uuQuo - MAX_VAL);
			uuQuo = MAX_VAL;
		}
		while (uuRem <= MAX_VAL && uuQuo * uDenNext > _MakeUlong((DTYPE)uuRem, uNumNext)) {
			uuQuo--;
			uuRem += uDen;
		}

		// Multiply and subtract. Note that uuQuo may be 1 too large. If we have a borrow
		// at the end, we'll add the denominator back on and decrement uuQuo.
		if (uuQuo > 0) {
			DTYPE_TMP uuBorrow = 0;
			for (int iu2 = 0; iu2 < cuDen; iu2++) {
				uuBorrow += b->array[iu2] * uuQuo;
				DTYPE uSub = (DTYPE)uuBorrow;
				uuBorrow >>= kcbitUint;
				if (c->array[iu + iu2] < uSub)
					uuBorrow++;
				c->array[iu + iu2] -= uSub;
			}

			//Contract.Assert(uNumHi == uuBorrow || uNumHi == uuBorrow - 1);
			if (uNumHi < uuBorrow) {
				// Add, tracking carry.
				DTYPE uCarry = 0;
				for (int iu2 = 0; iu2 < cuDen; iu2++) {
					uCarry = AddCarry(&c->array[iu + iu2], b->array[iu2], uCarry);
					//Contract.Assert(uCarry <= 1);
				}
				//Contract.Assert(uCarry == 1);
				uuQuo--;
			}
			//ai = iu + cuDen - 1;
		}

		if (fQuo) {
			if (cuQuo == 1)
				quo->array[0] = (DTYPE)uuQuo;
			else
				quo->array[iu] = (DTYPE)uuQuo;
		}


	}
	memset(c->array + cuDen, 0, BN_ARRAY_MEMORY_SIZE - sizeof(DTYPE) * cuDen);
	//Contract.Assert(cuDen > 1 && regNum._iuLast > 0);
	//regNum._iuLast = cuDen - 1;
}

void bignum_div(struct bn* a, struct bn* b, struct bn* c)
{
	require(a, "a is null");
	require(b, "b is null");
	require(c, "c is null");

	int ai = _valid_(a);
	if (ai == -1)
	{
		memset(c->array, 0, BN_ARRAY_MEMORY_SIZE);
		return;
	}

	int bi = _valid_(b);
	if (bi == -1)
	{
		memset(c->array, 0, BN_ARRAY_MEMORY_SIZE);
		return;
	}

	if (bi == 0) {
		if (c != a)
		{
			memcpy(c->array, a->array, BN_ARRAY_MEMORY_SIZE);
		}
		DivMod(c, b->array[0], ai);
		return;
	}
	if (ai == 0) {
		memset(c->array, 0, BN_ARRAY_MEMORY_SIZE);
		return;
	}

	Bn quo;
	ModDivCore(a, ai, b, bi, c, true, &quo);
	*c = quo;
}


void bignum_lshift(struct bn* a, struct bn* b, int nbits)
{
	require(a, "a is null");
	require(b, "b is null");
	require(nbits >= 0, "no negative shifts");

	bignum_assign(b, a);
	/* Handle shift in multiples of word-size */
	const int nbits_pr_word = (WORD_SIZE * 8);
	int nwords = nbits / nbits_pr_word;
	if (nwords != 0)
	{
		_lshift_word(b, nwords);
		nbits -= (nwords * nbits_pr_word);
	}

	if (nbits != 0)
	{
		int i;
		for (i = (BN_ARRAY_SIZE - 1); i > 0; --i)
		{
			b->array[i] = (b->array[i] << nbits) | (b->array[i - 1] >> ((8 * WORD_SIZE) - nbits));
		}
		b->array[i] <<= nbits;
	}
}


void bignum_rshift(struct bn* a, struct bn* b, int nbits)
{
	require(a, "a is null");
	require(b, "b is null");
	require(nbits >= 0, "no negative shifts");

	bignum_assign(b, a);
	/* Handle shift in multiples of word-size */
	const int nbits_pr_word = (WORD_SIZE * 8);
	int nwords = nbits / nbits_pr_word;
	if (nwords != 0)
	{
		_rshift_word(b, nwords);
		nbits -= (nwords * nbits_pr_word);
	}

	if (nbits != 0)
	{
		int i;
		for (i = 0; i < (BN_ARRAY_SIZE - 1); ++i)
		{
			b->array[i] = (b->array[i] >> nbits) | (b->array[i + 1] << ((8 * WORD_SIZE) - nbits));
		}
		b->array[i] >>= nbits;
	}

}

DTYPE  Mod(struct bn* a, int aLen, DTYPE b)
{
	if (b == 1)
		return 0;

	int ai = aLen;
	if (ai < 0)
		ai = _valid_(a);

	if (ai == 0)
		return a->array[0] % b;

	DTYPE_TMP uu = 0;
	for (int iv = ai; iv >= 0; iv--) {
		uu = _MakeUlong((DTYPE)uu, a->array[iv]);
		uu %= b;
	}
	return (DTYPE)uu;
}

void bignum_mod(struct bn* a, struct bn* b, struct bn* c)
{
	/*
	  Take divmod and throw away div part
	*/
	require(a, "a is null");
	require(b, "b is null");
	require(c, "c is null");

	int ai = _valid_(a);
	if (ai == -1)
	{
		memset(c->array, 0, BN_ARRAY_MEMORY_SIZE);
		return;
	}

	int bi = _valid_(b);
	if (bi == -1)
	{
		memcpy(c->array, b->array, BN_ARRAY_MEMORY_SIZE);
		return;
	}

	if (bi == 0) {
		if (c != a)
			memcpy(c->array, a->array, BN_ARRAY_MEMORY_SIZE);
		c->array[0] = Mod(c, ai, b->array[0]);
		return;
	}

	if (ai == 0)
		return;

	ModDivCore(a, ai, b, bi, c, false, NULL);
}

void bignum_divmod(struct bn* a, struct bn* b, struct bn* c, struct bn* d)
{
	ModDivCore(a, _valid_(a), b, _valid_(b), d, false, c);
}


void bignum_and(struct bn* a, struct bn* b, struct bn* c)
{
	require(a, "a is null");
	require(b, "b is null");
	require(c, "c is null");

	int i;
	for (i = 0; i < BN_ARRAY_SIZE; ++i)
	{
		c->array[i] = (a->array[i] & b->array[i]);
	}
}


void bignum_or(struct bn* a, struct bn* b, struct bn* c)
{
	require(a, "a is null");
	require(b, "b is null");
	require(c, "c is null");

	int i;
	for (i = 0; i < BN_ARRAY_SIZE; ++i)
	{
		c->array[i] = (a->array[i] | b->array[i]);
	}
}


void bignum_xor(struct bn* a, struct bn* b, struct bn* c)
{
	require(a, "a is null");
	require(b, "b is null");
	require(c, "c is null");

	int i;
	for (i = 0; i < BN_ARRAY_SIZE; ++i)
	{
		c->array[i] = (a->array[i] ^ b->array[i]);
	}
}


int bignum_cmp(struct bn* a, struct bn* b)
{
	require(a, "a is null");
	require(b, "b is null");

	int i = BN_ARRAY_SIZE;
	do
	{
		i -= 1; /* Decrement first, to start with last array element */
		if (a->array[i] > b->array[i])
		{
			return LARGER;
		}
		else if (a->array[i] < b->array[i])
		{
			return SMALLER;
		}
	} while (i != 0);

	return EQUAL;
}


int bignum_is_zero(struct bn* n)
{
	require(n, "n is null");

	int i;
	for (i = 0; i < BN_ARRAY_SIZE; ++i)
	{
		if (n->array[i])
		{
			return 0;
		}
	}

	return 1;
}


void bignum_pow(struct bn* a, struct bn* b, struct bn* c)
{
	require(a, "a is null");
	require(b, "b is null");
	require(c, "c is null");

	struct bn tmp;

	bignum_init(c);

	if (bignum_cmp(b, c) == EQUAL)
	{
		/* Return 1 when exponent is 0 -- n^0 = 1 */
		bignum_inc(c);
	}
	else
	{
		struct bn bcopy;
		bignum_assign(&bcopy, b);

		/* Copy a -> tmp */
		bignum_assign(&tmp, a);

		bignum_dec(&bcopy);

		/* Begin summing products: */
		while (!bignum_is_zero(&bcopy))
		{

			/* c = tmp * tmp */
			bignum_mul(&tmp, a, c);
			/* Decrement b by one */
			bignum_dec(&bcopy);

			bignum_assign(&tmp, c);
		}

		/* c = tmp */
		bignum_assign(c, &tmp);
	}
}

void bignum_isqrt(struct bn* a, struct bn* b)
{
	/*require(a, "a is null");
	require(b, "b is null");

	struct bn low, high, mid, tmp;

	bignum_init(&low);
	bignum_assign(&high, a);
	bignum_rshift(&high, &mid, 1);
	bignum_inc(&mid);

	while (bignum_cmp(&high, &low) > 0)
	{
		bignum_mul(&mid, &mid, &tmp);
		if (bignum_cmp(&tmp, a) > 0)
		{
			bignum_assign(&high, &mid);
			bignum_dec(&high);
		}
		else
		{
			bignum_assign(&low, &mid);
		}
		bignum_sub(&high, &low, &mid);
		_rshift_one_bit(&mid);
		bignum_add(&low, &mid, &mid);
		bignum_inc(&mid);
	}
	bignum_assign(b, &low);*/
}


void bignum_assign(struct bn* dst, struct bn* src)
{
	require(dst, "dst is null");
	require(src, "src is null");

	int i;
	for (i = 0; i < BN_ARRAY_SIZE; ++i)
	{
		dst->array[i] = src->array[i];
	}
}


/* Private / Static functions. */
static void _rshift_word(struct bn* a, int nwords)
{
	/* Naive method: */
	require(a, "a is null");
	require(nwords >= 0, "no negative shifts");

	int i;
	if (nwords >= BN_ARRAY_SIZE)
	{
		memset(a->array, 0, BN_ARRAY_SIZE * sizeof(DTYPE));
		return;
	}

	for (i = 0; i < BN_ARRAY_SIZE - nwords; ++i)
	{
		a->array[i] = a->array[i + nwords];
	}
	for (; i < BN_ARRAY_SIZE; ++i)
	{
		memset(a->array + (i * sizeof(DTYPE)), 0, BN_ARRAY_SIZE - i);
		//a->array[i] = 0;
	}
}


static void _lshift_word(struct bn* a, int nwords)
{
	require(a, "a is null");
	require(nwords >= 0, "no negative shifts");

	int i;
	/* Shift whole words */
	for (i = (BN_ARRAY_SIZE - 1); i >= nwords; --i)
	{
		a->array[i] = a->array[i - nwords];
	}
	memset(a->array, 0, sizeof(DTYPE) * (i + 1));
	/* Zero pad shifted words. */
	/*for (; i >= 0; --i)
	{
		a->array[i] = 0;
	}*/
}


static void _lshift_one_bit(struct bn* a)
{
	require(a, "a is null");

	int i;
	for (i = (BN_ARRAY_SIZE - 1); i > 0; --i)
	{
		a->array[i] = (a->array[i] << 1) | (a->array[i - 1] >> ((8 * WORD_SIZE) - 1));
	}
	a->array[0] <<= 1;
}


static void _rshift_one_bit(struct bn* a)
{
	require(a, "a is null");

	int i;
	for (i = 0; i < (BN_ARRAY_SIZE - 1); ++i)
	{
		a->array[i] = (a->array[i] >> 1) | (a->array[i + 1] << ((8 * WORD_SIZE) - 1));
	}
	a->array[BN_ARRAY_SIZE - 1] >>= 1;
}

int deepGetI(const char* str, int num, int count)
{
	if (count > 0)
	{
		return deepGetI(str, num * 10 + str[kcchBase - count] - '0', --count);
	}
	return num;
}

void bignum_from_string(struct bn* n, const char* str)
{
	require(n, "n is null");
	require(str, "str is null");
	int len = strlen(str);
	bignum_init(n);
	struct bn tmpN;
	DTYPE_TMP tempI = 0;
	for (int i = 0; i < len; ++i)
	{
		int c = str[i];
		if (c == '.')
			return;
		/*if (i + kcchBase > len)
		{
			int num1 = deepGetI(str + kcchBase * i, 0, len - i);
		}
		else
		{
			int num1 = deepGetI(str + kcchBase * i, 0, kcchBase);
		}*/

		/*if (tempI < 0xFFFF)
		{
			int num = str[i] - '0';
			tempI = tempI * 10 + num;
		}
		else
		{
			bignum_mul(n, &tmpTen, n);
			bignum_from_int(&tmpN, num);
			bignum_add(n, &tmpN, n);
		}*/
		int num = c - '0';
		bignum_mul_int(n, 10, n, -1);
		bignum_add_int(n, num, n, -1);
	}
}

void bignum_to_string(struct bn* n, char* rgch, int len)
{
	// 判断n是否为0
	bool isZero = true;
	for (int i = 0; i < BN_ARRAY_SIZE; ++i) {
		if (n->array[i] > 0)
		{
			isZero = false;
			break;
		}
	}
	if (isZero)
	{
		rgch[0] = '0';
		rgch[1] = 0;
		return;
	}

	// First convert to base 10^9.
	int cuSrc = BN_ARRAY_SIZE;
	int cuMax = cuSrc * 10 / 9 + 2;
	int tt = sizeof(DTYPE) * cuMax;
	DTYPE* rguDst = malloc(sizeof(DTYPE) * cuMax);
	int cuDst = 0;

	for (int iuSrc = cuSrc; --iuSrc >= 0; ) {
		DTYPE uCarry = n->array[iuSrc];
		for (int iuDst = 0; iuDst < cuDst; iuDst++) {
			DTYPE_TMP uuRes = _MakeUlong(rguDst[iuDst], uCarry);
			rguDst[iuDst] = (DTYPE)(uuRes % kuBase);
			uCarry = (DTYPE)(uuRes / kuBase);
		}
		if (uCarry != 0) {
			rguDst[cuDst++] = uCarry % kuBase;
			uCarry /= kuBase;
			if (uCarry != 0)
				rguDst[cuDst++] = uCarry;
		}
	}

	int cchMax = cuDst * kcchBase;
	int rgchBufSize = cchMax + 1;
	if (len < rgchBufSize)
	{
		free(rguDst);
		return;
	}
	//char* rgch = __vcrt_malloc_normal(rgchBufSize);

	int ichDst = cchMax;

	for (int iuDst = 0; iuDst < cuDst - 1; iuDst++) {
		DTYPE uDig = rguDst[iuDst];
		for (int cch = kcchBase; --cch >= 0; ) {
			rgch[--ichDst] = (char)('0' + uDig % 10);
			uDig /= 10;
		}
	}
	for (DTYPE uDig = rguDst[cuDst - 1]; uDig != 0; ) {
		rgch[--ichDst] = (char)('0' + uDig % 10);
		uDig /= 10;
	}
	memcpy(rgch, rgch + ichDst, cchMax - ichDst);
	rgch[cchMax - ichDst] = 0;
	free(rguDst);
}

/* endian options */
#define BIG	0
#define LITTLE	1

DTYPE getIntegerByte(const char* buff, int size, int endian)
{
	int i;
	DTYPE l = 0;
	if (endian == BIG) {
		for (i = 0; i < size; i++) {
			l <<= 8;
			l |= (DTYPE)(unsigned char)buff[i];
		}
	}
	else {
		for (i = size - 1; i >= 0; i--) {
			l <<= 8;
			l |= (DTYPE)(unsigned char)buff[i];
		}
	}
	return l;
}


void setIntegerByte(char* buff, DTYPE value, int endian)
{
	int size = sizeof(DTYPE);
	if (endian == LITTLE) {
		int i;
		for (i = 0; i < size; i++) {
			buff[i] = (value & 0xff);
			value >>= 8;
		}
	}
	else {
		int i;
		for (i = size - 1; i >= 0; i--) {
			buff[i] = (value & 0xff);
			value >>= 8;
		}
	}
}

void bignum_from_byteArray(struct bn* n, const char* buff, int len, int endian)
{
	require(n, "n is null");
	bignum_init(n);
	int size = sizeof(DTYPE);
	int i = 0;
	int offset = 0;
	int j = 0;
	while (i < len)
	{
		int m = size;
		if (i + size > len)
		{
			m = len - i;
		}
		n->array[j++] = getIntegerByte(buff + offset, m, endian);
		offset += m;
		i += size;
	}
}

unsigned char* bignum_to_byteArray(struct bn* n, int* count, int endian)
{
	require(n, "n is null");
	int size = sizeof(DTYPE);
	int maxn = BN_ARRAY_SIZE - 1;
	for (; maxn >= 0; --maxn)
	{
		if (n->array[maxn] != 0)
			break;
	}

	// 对0值特殊处理
	if (maxn <= -1)
	{
		unsigned char* buff = malloc(1);
		*count = 1;
		buff[0] = 0;
		return buff;
	}

	maxn++;
	int ccount = size * maxn;
	unsigned char* buff = malloc(ccount);
	for (int i = 0; i < maxn; ++i)
	{
		setIntegerByte(buff + size * i, n->array[i], endian);
	}
	*count = 1;
	for (int j = ccount - 1; j >= 0; --j)
	{
		if (buff[j] > 0)
		{
			*count = j + 1;
			break;
		}
	}
	return buff;
}
