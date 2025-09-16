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
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include "tolua.h"
#include <inttypes.h>
#include <errno.h>
#include <ctype.h>
#include <stdlib.h>

static bool _isbn(lua_State* L, int pos)
{
	if (lua_getmetatable(L, pos))
	{
		lua_getref(L, LUA_RIDX_BIGNUMBER);
		int equal = lua_rawequal(L, -1, -2);
		lua_pop(L, 2);
		return equal;
	}

	return false;
}

LUALIB_API void tolua_pushbn(lua_State* L, Bn* n)
{
	Bn* p = (Bn*)lua_newuserdata(L, sizeof(Bn));
	*p = *n;
	lua_getref(L, LUA_RIDX_BIGNUMBER);
	lua_setmetatable(L, -2);
}

//转换一个字符串为 Bn
static void _strToBn(lua_State* L, int pos, Bn* a)
{
	const char* str = lua_tostring(L, pos);
	bignum_from_string(a, str);
}

LUALIB_API Bn tolua_tobn(lua_State* L, int pos)
{
	Bn n;
	int type = lua_type(L, pos);

	switch (type)
	{
	case LUA_TNUMBER:
		bignum_from_int(&n, (DTYPE_TMP)lua_tonumber(L, pos));
		break;
	case LUA_TSTRING:
		_strToBn(L, pos, &n);
		break;
	case LUA_TUSERDATA:
		if (_isbn(L, pos))
		{
			n = *(Bn*)lua_touserdata(L, pos);
		}
		break;
	default:
		break;
	}

	return n;
}

static Bn tolua_checkbn(lua_State* L, int pos)
{
	Bn n;
	int type = lua_type(L, pos);

	switch (type)
	{
	case LUA_TNUMBER:
		bignum_from_int(&n, (DTYPE_TMP)lua_tonumber(L, pos));
		break;
	case LUA_TSTRING:
		_strToBn(L, pos, &n);
		break;
	case LUA_TUSERDATA:
		if (_isbn(L, pos))
		{
			n = *(Bn*)lua_touserdata(L, pos);
		}
		break;
	default:
		bignum_init(&n);
		break;
	}
	return n;
}

static int _bnadd(lua_State* L)
{
	Bn lhs = tolua_checkbn(L, 1);
	Bn rhs = tolua_checkbn(L, 2);
	bignum_add(&lhs, &rhs, &lhs);
	tolua_pushbn(L, &lhs);
	return 1;
}

static int _bnsub(lua_State* L)
{
	Bn lhs = tolua_checkbn(L, 1);
	Bn rhs = tolua_checkbn(L, 2);
	int sign = 1;
	bignum_sub(&lhs, &rhs, &lhs, &sign);
	tolua_pushbn(L, &lhs);
	//lua_pushnumber(L, sign);
	return 1;
}

static int _bnmul(lua_State* L)
{
	Bn lhs = tolua_checkbn(L, 1);
	Bn rhs = tolua_checkbn(L, 2);
	bignum_mul(&lhs, &rhs, &lhs);
	tolua_pushbn(L, &lhs);
	return 1;
}

static int _bndiv(lua_State* L)
{
	Bn lhs = tolua_checkbn(L, 1);
	Bn rhs = tolua_checkbn(L, 2);
	bignum_div(&lhs, &rhs, &lhs);
	tolua_pushbn(L, &lhs);
	return 1;
}

static int _bnmod(lua_State* L)
{
	Bn lhs = tolua_checkbn(L, 1);
	Bn rhs = tolua_checkbn(L, 2);
	bignum_mod(&lhs, &rhs, &lhs);
	tolua_pushbn(L, &lhs);
	return 1;
}

static int _bnunm(lua_State* L)
{
	Bn lhs = tolua_checkbn(L, 1);
	tolua_pushbn(L, &lhs);
	return 1;
}

static int _bnpow(lua_State* L)
{
	Bn lhs = tolua_checkbn(L, 1);
	Bn rhs = tolua_checkbn(L, 2);
	bignum_pow(&lhs, &rhs, &lhs);
	tolua_pushbn(L, &lhs);
	return 1;
}

static int _bneq(lua_State* L)
{
	Bn lhs = *(Bn*)lua_touserdata(L, 1);
	Bn rhs = *(Bn*)lua_touserdata(L, 2);
	lua_pushboolean(L, bignum_cmp(&lhs, &rhs) == 0);
	return 1;
}

static int _bnequals(lua_State* L)
{
	Bn lhs = tolua_checkbn(L, 1);
	Bn rhs = tolua_tobn(L, 2);
	lua_pushboolean(L, bignum_cmp(&lhs, &rhs) == 0);
	return 1;
}

static int _bnlt(lua_State* L)
{
	Bn lhs = tolua_checkbn(L, 1);
	Bn rhs = tolua_checkbn(L, 2);
	lua_pushboolean(L, bignum_cmp(&lhs, &rhs) < 0);
	return 1;
}

static int _bnle(lua_State* L)
{
	Bn lhs = tolua_checkbn(L, 1);
	Bn rhs = tolua_checkbn(L, 2);
	lua_pushboolean(L, bignum_cmp(&lhs, &rhs) <= 0);
	return 1;
}

static int _bntostring(lua_State* L)
{
	if (!_isbn(L, 1))
	{
		return luaL_typerror(L, 1, "bignumber");
	}

	Bn n = tolua_tobn(L, 1);
	char temp[2048];
	bignum_to_string(&n, temp, 2048);
	lua_pushstring(L, temp);
	return 1;
}

static int _bnfrombytes(lua_State* L)
{
	Bn n;
	int type = lua_type(L, 1);
	switch (type)
	{
	case LUA_TSTRING:
	{
		const char* str = lua_tostring(L, 1);
		int len = lua_tonumber(L, 2);
		bignum_from_byteArray(&n, str, len, 1);
		break;
	}
	default:
		break;
	}
	tolua_pushbn(L, &n);
	return 1;
}

static int _bntobytes(lua_State* L)
{
	if (_isbn(L, 1))
	{
		Bn* n = (Bn*)lua_touserdata(L, 1);
		int count;
		unsigned char* b = bignum_to_byteArray(n, &count, 1);
		lua_pushlstring(L, b, count);
		lua_pushnumber(L, count);
		free(b);
	}
	else
		return 0;
	return 2;
}

static int _bnfromdouble(lua_State* L)
{
	Bn n;
	int type = lua_type(L, 1);
	if (type == LUA_TNUMBER)
	{
		double d = (double)lua_tonumber(L, 1);
		bignum_from_double(&n, d);
	}
	tolua_pushbn(L, &n);
	return 1;
}

static int tolua_newbn(lua_State* L)
{
	Bn n;
	int type = lua_type(L, 1);

	if (type == LUA_TSTRING)
	{
		_strToBn(L, 1, &n);
	}
	else if (type == LUA_TNUMBER)
	{
		DTYPE_TMP n1 = (DTYPE_TMP)lua_tonumber(L, 1);
		bignum_from_int(&n, n1);
	}
	tolua_pushbn(L, &n);
	return 1;
}

static int _bneqnumber(lua_State* L)
{
	Bn lhs = tolua_checkbn(L, 1);
	Bn rhs = tolua_tobn(L, 2);
	lua_pushnumber(L, bignum_cmp(&lhs, &rhs));
	return 1;
}


static int _bntonum2(lua_State* L)
{
	if (!_isbn(L, 1))
	{
		return luaL_typerror(L, 1, "bn");
	}

	Bn n = tolua_tobn(L, 1);
	lua_pushnumber(L, n.array[0]);
	lua_pushnumber(L, n.array[1]);
	return 2;
}

static int _bntoint(lua_State* L)
{
	if (!_isbn(L, 1))
	{
		return luaL_typerror(L, 1, "bn");
	}

	Bn n = tolua_tobn(L, 1);
	lua_pushnumber(L, n.array[0]);
	return 1;
}

void tolua_openbignumber(lua_State* L)
{
	lua_newtable(L);
	lua_pushvalue(L, -1);
	lua_setglobal(L, "bignumber");

	lua_getref(L, LUA_RIDX_LOADED);
	lua_pushstring(L, "bignumber");
	lua_pushvalue(L, -3);
	lua_rawset(L, -3);
	lua_pop(L, 1);

	lua_pushstring(L, "__add"),
		lua_pushcfunction(L, _bnadd);
	lua_rawset(L, -3);

	lua_pushstring(L, "__sub"),
		lua_pushcfunction(L, _bnsub);
	lua_rawset(L, -3);

	lua_pushstring(L, "__mul"),
		lua_pushcfunction(L, _bnmul);
	lua_rawset(L, -3);

	lua_pushstring(L, "__div"),
		lua_pushcfunction(L, _bndiv);
	lua_rawset(L, -3);

	lua_pushstring(L, "__mod"),
		lua_pushcfunction(L, _bnmod);
	lua_rawset(L, -3);

	lua_pushstring(L, "__unm"),
		lua_pushcfunction(L, _bnunm);
	lua_rawset(L, -3);

	lua_pushstring(L, "__pow"),
		lua_pushcfunction(L, _bnpow);
	lua_rawset(L, -3);

	lua_pushstring(L, "__tostring");
	lua_pushcfunction(L, _bntostring);
	lua_rawset(L, -3);

	lua_pushstring(L, "tostring");
	lua_pushcfunction(L, _bntostring);
	lua_rawset(L, -3);

	lua_pushstring(L, "__eq");
	lua_pushcfunction(L, _bneq);
	lua_rawset(L, -3);

	lua_pushstring(L, "__lt");
	lua_pushcfunction(L, _bnlt);
	lua_rawset(L, -3);

	lua_pushstring(L, "__le");
	lua_pushcfunction(L, _bnle);
	lua_rawset(L, -3);

	lua_pushstring(L, ".name");
	lua_pushstring(L, "bignumber");
	lua_rawset(L, -3);

	lua_pushstring(L, "new");
	lua_pushcfunction(L, tolua_newbn);
	lua_rawset(L, -3);

	lua_pushstring(L, "equals");
	lua_pushcfunction(L, _bnequals);
	lua_rawset(L, -3);

	lua_pushstring(L, "eqn");
	lua_pushcfunction(L, _bneqnumber);
	lua_rawset(L, -3);

	lua_pushstring(L, "tobytes");
	lua_pushcfunction(L, _bntobytes);
	lua_rawset(L, -3);

	lua_pushstring(L, "frombytes");
	lua_pushcfunction(L, _bnfrombytes);
	lua_rawset(L, -3);

	lua_pushstring(L, "fromdouble");
	lua_pushcfunction(L, _bnfromdouble);
	lua_rawset(L, -3);

	lua_pushstring(L, "__index");
	lua_pushvalue(L, -2);
	lua_rawset(L, -3);

	lua_pushstring(L, "tonum2");
	lua_pushcfunction(L, _bntonum2);
	lua_rawset(L, -3);

	lua_pushstring(L, "toint");
	lua_pushcfunction(L, _bntoint);
	lua_rawset(L, -3);

	lua_rawseti(L, LUA_REGISTRYINDEX, LUA_RIDX_BIGNUMBER);
}