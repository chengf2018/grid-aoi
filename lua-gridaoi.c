#include "gridaoi/grid-aoi.h"
#include <lua.h>
#include <lauxlib.h>

int 
_aoi_new(lua_State *L) {
	int realwidth = luaL_checkinteger(L, 1);
	int realhigh = luaL_checkinteger(L, 2);
	int tile_len = luaL_checkinteger(L, 3);
	int view_range = luaL_checkinteger(L, 4);

	aoi_contextptr aoictx = aoi_new(realwidth, realhigh, tile_len, view_range);
	if (aoictx == NULL) {
		luaL_error(L,"[_aoi_new] realwidth:%d, realhigh:%d, tile_len:%d, view_range:%d", realwidth, realhigh, tile_len, view_range);
	}
	lua_pushlightuserdata(L, aoictx);
	return 1;
}

int 
_aoi_delete(lua_State *L) {
	aoi_contextptr aoictx = lua_touserdata(L, 1);
	aoi_delete(aoictx);
	return 0;
}

int 
_aoi_enter(lua_State *L) {
	aoi_contextptr aoictx = lua_touserdata(L, 1);
	int id = luaL_checkinteger(L, 2);
	float curx = luaL_checknumber(L, 3);
	float cury = luaL_checknumber(L, 4);
	int layer = luaL_checkinteger(L, 5);
	int viewLayer = luaL_checkinteger(L, 6);

	struct int_valuevec enter_self, enter_other;
	valuevec_init(&enter_self);
	valuevec_init(&enter_other);
	if (aoi_enter(aoictx, id, curx, cury, (short)layer,
		(short)viewLayer, &enter_self, &enter_other) != 0) {
		luaL_error(L, "[_aoi_enter] id:%d,x:%f,y:%f", id, curx, cury);
	}

	lua_createtable(L, valuevec_count(&enter_self), 0);
	for (size_t i=0; i<valuevec_count(&enter_self); i++) {
		lua_pushinteger(L, valuevec_get(&enter_self, i));
		lua_rawseti(L, -2, i+1);
	}

	lua_createtable(L, valuevec_count(&enter_other), 0);
	for (size_t i=0; i<valuevec_count(&enter_other); i++) {
		lua_pushinteger(L, valuevec_get(&enter_other, i));
		lua_rawseti(L, -2, i+1);
	}
	valuevec_free(&enter_self);
	valuevec_free(&enter_other);
	return 2;
}

int 
_aoi_leave(lua_State *L) {
	aoi_contextptr aoictx = lua_touserdata(L, 1);
	int id = lua_tointeger(L, 2);
	struct int_valuevec leave_other;
	valuevec_init(&leave_other);
	if (aoi_leave(aoictx, id, &leave_other) != 0) {
		luaL_error(L, "[_aoi_leave] id:%d", id);
	}
	lua_createtable(L, valuevec_count(&leave_other), 0);
	for (size_t i=0; i<valuevec_count(&leave_other); i++) {
		lua_pushinteger(L, valuevec_get(&leave_other, i));
		lua_rawseti(L, -2, i+1);
	}
	valuevec_free(&leave_other);
	return 1;
}

int 
_aoi_update(lua_State *L) {
	aoi_contextptr aoictx = lua_touserdata(L, 1);
	int id = lua_tointeger(L, 2);
	float curx = luaL_checknumber(L, 3);
	float cury = luaL_checknumber(L, 4);
	
	struct int_valuevec enter, leave;
	valuevec_init(&enter);
	valuevec_init(&leave);
	if (aoi_move(aoictx, id, curx, cury, &enter, &leave) != 0) {
		luaL_error(L, "[_aoi_update] id:%d", id);
	}

	lua_createtable(L, valuevec_count(&enter), 0);
	for (size_t i=0; i<valuevec_count(&enter); i++) {
		lua_pushinteger(L, valuevec_get(&enter, i));
		lua_rawseti(L, -2, i+1);
	}

	lua_createtable(L, valuevec_count(&leave), 0);
	for (size_t i=0; i<valuevec_count(&leave); i++) {
		lua_pushinteger(L, valuevec_get(&leave, i));
		lua_rawseti(L, -2, i+1);
	}
	valuevec_free(&enter);
	valuevec_free(&leave);
	return 2;
}

int 
_aoi_viewlist(lua_State *L) {
	aoi_contextptr aoictx = lua_touserdata(L, 1);
	int id = lua_tointeger(L, 2);
	struct int_valuevec viewed;
	valuevec_init(&viewed);
	if (aoi_viewlist(aoictx, id, &viewed) != 0) {
		luaL_error(L, "[_aoi_viewlist] id:%d", id);
	}
	lua_createtable(L, valuevec_count(&viewed), 0);
	for (size_t i=0; i<valuevec_count(&viewed); i++) {
		lua_pushinteger(L, valuevec_get(&viewed, i));
		lua_rawseti(L, -2, i+1);
	}
	valuevec_free(&viewed);
	return 1;
}

int 
luaopen_gridaoi_c(lua_State *L)
{
	luaL_checkversion(L);
	luaL_Reg l[] = {
		{ "aoi_new", _aoi_new},
		{ "aoi_delete", _aoi_delete},
		{ "aoi_enter", _aoi_enter},
		{ "aoi_leave", _aoi_leave},
		{ "aoi_update", _aoi_update},
		{ "aoi_viewlist", _aoi_viewlist},
		{ NULL, NULL },
	};

	lua_createtable(L, 0, (sizeof(l)) / sizeof(luaL_Reg) - 1);
	luaL_setfuncs(L, l, 0);
	return 1;
}
