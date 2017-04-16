#ifdef __cplusplus
extern "C" {
#endif
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
#ifdef __cplusplus
}
#endif
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if LUA_VERSION_NUM == 501
  #define LUA_OK 0
#endif

/* Copied from lua.c */

static lua_State *globalL = NULL;

static void lstop (lua_State *L, lua_Debug *ar) {
  (void)ar;  /* unused arg. */
  lua_sethook(L, NULL, 0, 0);  /* reset hook */
  luaL_error(L, "interrupted!");
}

static void laction (int i) {
  signal(i, SIG_DFL); /* if another SIGINT happens, terminate process */
  lua_sethook(globalL, lstop, LUA_MASKCALL | LUA_MASKRET | LUA_MASKCOUNT, 1);
}

static void createargtable (lua_State *L, char **argv, int argc, int script) {
  int i, narg;
  if (script == argc) script = 0;  /* no script name? */
  narg = argc - (script + 1);  /* number of positive indices */
  lua_createtable(L, narg, script + 1);
  for (i = 0; i < argc; i++) {
    lua_pushstring(L, argv[i]);
    lua_rawseti(L, -2, i - script);
  }
  lua_setglobal(L, "arg");
}

static int msghandler (lua_State *L) {
  const char *msg = lua_tostring(L, 1);
  if (msg == NULL) {  /* is error object not a string? */
    if (luaL_callmeta(L, 1, "__tostring") &&  /* does it have a metamethod */
        lua_type(L, -1) == LUA_TSTRING)  /* that produces a string? */
      return 1;  /* that is the message */
    else
      msg = lua_pushfstring(L, "(error object is a %s value)",
                               luaL_typename(L, 1));
  }
  /* Call debug.traceback() instead of luaL_traceback() for Lua 5.1 compatibility. */
  lua_getglobal(L, "debug");
  lua_getfield(L, -1, "traceback");
  /* debug */
  lua_remove(L, -2);
  lua_pushstring(L, msg);
  /* original msg */
  lua_remove(L, -3);
  lua_pushinteger(L, 2);  /* skip this function and traceback */
  lua_call(L, 2, 1); /* call debug.traceback */
  return 1;  /* return the traceback */
}

static int docall (lua_State *L, int narg, int nres) {
  int status;
  int base = lua_gettop(L) - narg;  /* function index */
  lua_pushcfunction(L, msghandler);  /* push message handler */
  lua_insert(L, base);  /* put it under function and args */
  globalL = L;  /* to be available to 'laction' */
  signal(SIGINT, laction);  /* set C-signal handler */
  status = lua_pcall(L, narg, nres, base);
  signal(SIGINT, SIG_DFL); /* reset C-signal handler */
  lua_remove(L, base);  /* remove message handler from the stack */
  return status;
}

int main(int argc, char *argv[])
{
  lua_State *L = luaL_newstate();
  luaL_openlibs(L);
  createargtable(L, argv, argc, 0);

  static const unsigned char lua_loader_program[] = {
    0x6c, 0x6f, 0x63, 0x61, 0x6c, 0x20, 0x61, 0x72, 0x67, 0x73, 0x20, 0x3d, 0x20, 0x7b, 0x2e, 0x2e, 0x2e, 0x7d, 0x0a, 0x6c, 0x6f, 0x63, 0x61, 0x6c, 0x20, 0x6c, 0x75, 0x61, 0x5f, 0x62, 0x75, 0x6e, 0x64, 0x6c, 0x65, 0x20, 0x3d, 0x20, 0x61, 0x72, 0x67, 0x73, 0x5b, 0x31, 0x5d, 0x0a, 0x0a, 0x6c, 0x6f, 0x63, 0x61, 0x6c, 0x20, 0x66, 0x75, 0x6e, 0x63, 0x74, 0x69, 0x6f, 0x6e, 0x20, 0x6c, 0x6f, 0x61, 0x64, 0x5f, 0x73, 0x74, 0x72, 0x69, 0x6e, 0x67, 0x28, 0x73, 0x74, 0x72, 0x2c, 0x20, 0x6e, 0x61, 0x6d, 0x65, 0x29, 0x0a, 0x20, 0x20, 0x69, 0x66, 0x20, 0x5f, 0x56, 0x45, 0x52, 0x53, 0x49, 0x4f, 0x4e, 0x20, 0x3d, 0x3d, 0x20, 0x22, 0x4c, 0x75, 0x61, 0x20, 0x35, 0x2e, 0x31, 0x22, 0x20, 0x74, 0x68, 0x65, 0x6e, 0x0a, 0x20, 0x20, 0x20, 0x20, 0x72, 0x65, 0x74, 0x75, 0x72, 0x6e, 0x20, 0x6c, 0x6f, 0x61, 0x64, 0x73, 0x74, 0x72, 0x69, 0x6e, 0x67, 0x28, 0x73, 0x74, 0x72, 0x2c, 0x20, 0x6e, 0x61, 0x6d, 0x65, 0x29, 0x0a, 0x20, 0x20, 0x65, 0x6c, 0x73, 0x65, 0x0a, 0x20, 0x20, 0x20, 0x20, 0x72, 0x65, 0x74, 0x75, 0x72, 0x6e, 0x20, 0x6c, 0x6f, 0x61, 0x64, 0x28, 0x73, 0x74, 0x72, 0x2c, 0x20, 0x6e, 0x61, 0x6d, 0x65, 0x29, 0x0a, 0x20, 0x20, 0x65, 0x6e, 0x64, 0x0a, 0x65, 0x6e, 0x64, 0x0a, 0x0a, 0x6c, 0x6f, 0x63, 0x61, 0x6c, 0x20, 0x66, 0x75, 0x6e, 0x63, 0x74, 0x69, 0x6f, 0x6e, 0x20, 0x6c, 0x75, 0x61, 0x5f, 0x6c, 0x6f, 0x61, 0x64, 0x65, 0x72, 0x28, 0x6e, 0x61, 0x6d, 0x65, 0x29, 0x0a, 0x20, 0x20, 0x6c, 0x6f, 0x63, 0x61, 0x6c, 0x20, 0x6d, 0x6f, 0x64, 0x20, 0x3d, 0x20, 0x6c, 0x75, 0x61, 0x5f, 0x62, 0x75, 0x6e, 0x64, 0x6c, 0x65, 0x5b, 0x6e, 0x61, 0x6d, 0x65, 0x5d, 0x20, 0x6f, 0x72, 0x20, 0x6c, 0x75, 0x61, 0x5f, 0x62, 0x75, 0x6e, 0x64, 0x6c, 0x65, 0x5b, 0x6e, 0x61, 0x6d, 0x65, 0x20, 0x2e, 0x2e, 0x20, 0x22, 0x2e, 0x69, 0x6e, 0x69, 0x74, 0x22, 0x5d, 0x0a, 0x20, 0x20, 0x69, 0x66, 0x20, 0x6d, 0x6f, 0x64, 0x20, 0x74, 0x68, 0x65, 0x6e, 0x0a, 0x20, 0x20, 0x20, 0x20, 0x69, 0x66, 0x20, 0x74, 0x79, 0x70, 0x65, 0x28, 0x6d, 0x6f, 0x64, 0x29, 0x20, 0x3d, 0x3d, 0x20, 0x22, 0x73, 0x74, 0x72, 0x69, 0x6e, 0x67, 0x22, 0x20, 0x74, 0x68, 0x65, 0x6e, 0x0a, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x6c, 0x6f, 0x63, 0x61, 0x6c, 0x20, 0x63, 0x68, 0x75, 0x6e, 0x6b, 0x2c, 0x20, 0x65, 0x72, 0x72, 0x73, 0x74, 0x72, 0x20, 0x3d, 0x20, 0x6c, 0x6f, 0x61, 0x64, 0x5f, 0x73, 0x74, 0x72, 0x69, 0x6e, 0x67, 0x28, 0x6d, 0x6f, 0x64, 0x2c, 0x20, 0x6e, 0x61, 0x6d, 0x65, 0x29, 0x0a, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x69, 0x66, 0x20, 0x63, 0x68, 0x75, 0x6e, 0x6b, 0x20, 0x74, 0x68, 0x65, 0x6e, 0x0a, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x72, 0x65, 0x74, 0x75, 0x72, 0x6e, 0x20, 0x63, 0x68, 0x75, 0x6e, 0x6b, 0x0a, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x65, 0x6c, 0x73, 0x65, 0x0a, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x65, 0x72, 0x72, 0x6f, 0x72, 0x28, 0x0a, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x28, 0x22, 0x65, 0x72, 0x72, 0x6f, 0x72, 0x20, 0x6c, 0x6f, 0x61, 0x64, 0x69, 0x6e, 0x67, 0x20, 0x6d, 0x6f, 0x64, 0x75, 0x6c, 0x65, 0x20, 0x27, 0x25, 0x73, 0x27, 0x20, 0x66, 0x72, 0x6f, 0x6d, 0x20, 0x6c, 0x75, 0x61, 0x73, 0x74, 0x61, 0x74, 0x69, 0x63, 0x20, 0x62, 0x75, 0x6e, 0x64, 0x6c, 0x65, 0x3a, 0x5c, 0x6e, 0x5c, 0x74, 0x25, 0x73, 0x22, 0x29, 0x3a, 0x66, 0x6f, 0x72, 0x6d, 0x61, 0x74, 0x28, 0x6e, 0x61, 0x6d, 0x65, 0x2c, 0x20, 0x65, 0x72, 0x72, 0x73, 0x74, 0x72, 0x29, 0x2c, 0x0a, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x30, 0x0a, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x29, 0x0a, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x65, 0x6e, 0x64, 0x0a, 0x20, 0x20, 0x20, 0x20, 0x65, 0x6c, 0x73, 0x65, 0x69, 0x66, 0x20, 0x74, 0x79, 0x70, 0x65, 0x28, 0x6d, 0x6f, 0x64, 0x29, 0x20, 0x3d, 0x3d, 0x20, 0x22, 0x66, 0x75, 0x6e, 0x63, 0x74, 0x69, 0x6f, 0x6e, 0x22, 0x20, 0x74, 0x68, 0x65, 0x6e, 0x0a, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x72, 0x65, 0x74, 0x75, 0x72, 0x6e, 0x20, 0x6d, 0x6f, 0x64, 0x0a, 0x20, 0x20, 0x20, 0x20, 0x65, 0x6e, 0x64, 0x0a, 0x20, 0x20, 0x65, 0x6c, 0x73, 0x65, 0x0a, 0x20, 0x20, 0x20, 0x20, 0x72, 0x65, 0x74, 0x75, 0x72, 0x6e, 0x20, 0x28, 0x22, 0x5c, 0x6e, 0x5c, 0x74, 0x6e, 0x6f, 0x20, 0x6d, 0x6f, 0x64, 0x75, 0x6c, 0x65, 0x20, 0x27, 0x25, 0x73, 0x27, 0x20, 0x69, 0x6e, 0x20, 0x6c, 0x75, 0x61, 0x73, 0x74, 0x61, 0x74, 0x69, 0x63, 0x20, 0x62, 0x75, 0x6e, 0x64, 0x6c, 0x65, 0x22, 0x29, 0x3a, 0x66, 0x6f, 0x72, 0x6d, 0x61, 0x74, 0x28, 0x6e, 0x61, 0x6d, 0x65, 0x29, 0x0a, 0x20, 0x20, 0x65, 0x6e, 0x64, 0x0a, 0x65, 0x6e, 0x64, 0x0a, 0x74, 0x61, 0x62, 0x6c, 0x65, 0x2e, 0x69, 0x6e, 0x73, 0x65, 0x72, 0x74, 0x28, 0x70, 0x61, 0x63, 0x6b, 0x61, 0x67, 0x65, 0x2e, 0x6c, 0x6f, 0x61, 0x64, 0x65, 0x72, 0x73, 0x20, 0x6f, 0x72, 0x20, 0x70, 0x61, 0x63, 0x6b, 0x61, 0x67, 0x65, 0x2e, 0x73, 0x65, 0x61, 0x72, 0x63, 0x68, 0x65, 0x72, 0x73, 0x2c, 0x20, 0x32, 0x2c, 0x20, 0x6c, 0x75, 0x61, 0x5f, 0x6c, 0x6f, 0x61, 0x64, 0x65, 0x72, 0x29, 0x0a, 0x6c, 0x6f, 0x63, 0x61, 0x6c, 0x20, 0x66, 0x75, 0x6e, 0x63, 0x20, 0x3d, 0x20, 0x6c, 0x75, 0x61, 0x5f, 0x6c, 0x6f, 0x61, 0x64, 0x65, 0x72, 0x28, 0x22, 0x61, 0x6e, 0x73, 0x69, 0x63, 0x6f, 0x6c, 0x6f, 0x72, 0x73, 0x22, 0x29, 0x0a, 0x69, 0x66, 0x20, 0x74, 0x79, 0x70, 0x65, 0x28, 0x66, 0x75, 0x6e, 0x63, 0x29, 0x20, 0x3d, 0x3d, 0x20, 0x22, 0x66, 0x75, 0x6e, 0x63, 0x74, 0x69, 0x6f, 0x6e, 0x22, 0x20, 0x74, 0x68, 0x65, 0x6e, 0x0a, 0x20, 0x20, 0x2d, 0x2d, 0x20, 0x52, 0x75, 0x6e, 0x20, 0x74, 0x68, 0x65, 0x20, 0x6d, 0x61, 0x69, 0x6e, 0x20, 0x4c, 0x75, 0x61, 0x20, 0x70, 0x72, 0x6f, 0x67, 0x72, 0x61, 0x6d, 0x2e, 0x0a, 0x20, 0x20, 0x66, 0x75, 0x6e, 0x63, 0x28, 0x29, 0x0a, 0x65, 0x6c, 0x73, 0x65, 0x0a, 0x20, 0x20, 0x65, 0x72, 0x72, 0x6f, 0x72, 0x28, 0x66, 0x75, 0x6e, 0x63, 0x2c, 0x20, 0x30, 0x29, 0x0a, 0x65, 0x6e, 0x64, 0x0a, 
  };
  /*printf("%.*s", (int)sizeof(lua_loader_program), lua_loader_program);*/
  if
  (
    luaL_loadbuffer(L, (const char*)lua_loader_program, sizeof(lua_loader_program), "ansicolors") 
    != LUA_OK
  )
  {
    fprintf(stderr, "luaL_loadbuffer: %s\n", lua_tostring(L, -1));
    lua_close(L);
    return 1;
  }
  
  /* lua_bundle */
  lua_newtable(L);
  static const unsigned char lua_require_1[] = {
    0x2d, 0x2d, 0x20, 0x61, 0x6e, 0x73, 0x69, 0x63, 0x6f, 0x6c, 0x6f, 0x72, 0x73, 0x2e, 0x6c, 0x75, 0x61, 0x20, 0x76, 0x31, 0x2e, 0x30, 0x2e, 0x32, 0x20, 0x28, 0x32, 0x30, 0x31, 0x32, 0x2d, 0x30, 0x38, 0x29, 0x0a, 0x0a, 0x2d, 0x2d, 0x20, 0x43, 0x6f, 0x70, 0x79, 0x72, 0x69, 0x67, 0x68, 0x74, 0x20, 0x28, 0x63, 0x29, 0x20, 0x32, 0x30, 0x30, 0x39, 0x20, 0x52, 0x6f, 0x62, 0x20, 0x48, 0x6f, 0x65, 0x6c, 0x7a, 0x20, 0x3c, 0x72, 0x6f, 0x62, 0x40, 0x68, 0x6f, 0x65, 0x6c, 0x7a, 0x72, 0x6f, 0x2e, 0x6e, 0x65, 0x74, 0x3e, 0x0a, 0x2d, 0x2d, 0x20, 0x43, 0x6f, 0x70, 0x79, 0x72, 0x69, 0x67, 0x68, 0x74, 0x20, 0x28, 0x63, 0x29, 0x20, 0x32, 0x30, 0x31, 0x31, 0x20, 0x45, 0x6e, 0x72, 0x69, 0x71, 0x75, 0x65, 0x20, 0x47, 0x61, 0x72, 0x63, 0xc3, 0xad, 0x61, 0x20, 0x43, 0x6f, 0x74, 0x61, 0x20, 0x3c, 0x65, 0x6e, 0x72, 0x69, 0x71, 0x75, 0x65, 0x2e, 0x67, 0x61, 0x72, 0x63, 0x69, 0x61, 0x2e, 0x63, 0x6f, 0x74, 0x61, 0x40, 0x67, 0x6d, 0x61, 0x69, 0x6c, 0x2e, 0x63, 0x6f, 0x6d, 0x3e, 0x0a, 0x2d, 0x2d, 0x0a, 0x2d, 0x2d, 0x20, 0x50, 0x65, 0x72, 0x6d, 0x69, 0x73, 0x73, 0x69, 0x6f, 0x6e, 0x20, 0x69, 0x73, 0x20, 0x68, 0x65, 0x72, 0x65, 0x62, 0x79, 0x20, 0x67, 0x72, 0x61, 0x6e, 0x74, 0x65, 0x64, 0x2c, 0x20, 0x66, 0x72, 0x65, 0x65, 0x20, 0x6f, 0x66, 0x20, 0x63, 0x68, 0x61, 0x72, 0x67, 0x65, 0x2c, 0x20, 0x74, 0x6f, 0x20, 0x61, 0x6e, 0x79, 0x20, 0x70, 0x65, 0x72, 0x73, 0x6f, 0x6e, 0x20, 0x6f, 0x62, 0x74, 0x61, 0x69, 0x6e, 0x69, 0x6e, 0x67, 0x20, 0x61, 0x20, 0x63, 0x6f, 0x70, 0x79, 0x0a, 0x2d, 0x2d, 0x20, 0x6f, 0x66, 0x20, 0x74, 0x68, 0x69, 0x73, 0x20, 0x73, 0x6f, 0x66, 0x74, 0x77, 0x61, 0x72, 0x65, 0x20, 0x61, 0x6e, 0x64, 0x20, 0x61, 0x73, 0x73, 0x6f, 0x63, 0x69, 0x61, 0x74, 0x65, 0x64, 0x20, 0x64, 0x6f, 0x63, 0x75, 0x6d, 0x65, 0x6e, 0x74, 0x61, 0x74, 0x69, 0x6f, 0x6e, 0x20, 0x66, 0x69, 0x6c, 0x65, 0x73, 0x20, 0x28, 0x74, 0x68, 0x65, 0x20, 0x22, 0x53, 0x6f, 0x66, 0x74, 0x77, 0x61, 0x72, 0x65, 0x22, 0x29, 0x2c, 0x20, 0x74, 0x6f, 0x20, 0x64, 0x65, 0x61, 0x6c, 0x0a, 0x2d, 0x2d, 0x20, 0x69, 0x6e, 0x20, 0x74, 0x68, 0x65, 0x20, 0x53, 0x6f, 0x66, 0x74, 0x77, 0x61, 0x72, 0x65, 0x20, 0x77, 0x69, 0x74, 0x68, 0x6f, 0x75, 0x74, 0x20, 0x72, 0x65, 0x73, 0x74, 0x72, 0x69, 0x63, 0x74, 0x69, 0x6f, 0x6e, 0x2c, 0x20, 0x69, 0x6e, 0x63, 0x6c, 0x75, 0x64, 0x69, 0x6e, 0x67, 0x20, 0x77, 0x69, 0x74, 0x68, 0x6f, 0x75, 0x74, 0x20, 0x6c, 0x69, 0x6d, 0x69, 0x74, 0x61, 0x74, 0x69, 0x6f, 0x6e, 0x20, 0x74, 0x68, 0x65, 0x20, 0x72, 0x69, 0x67, 0x68, 0x74, 0x73, 0x0a, 0x2d, 0x2d, 0x20, 0x74, 0x6f, 0x20, 0x75, 0x73, 0x65, 0x2c, 0x20, 0x63, 0x6f, 0x70, 0x79, 0x2c, 0x20, 0x6d, 0x6f, 0x64, 0x69, 0x66, 0x79, 0x2c, 0x20, 0x6d, 0x65, 0x72, 0x67, 0x65, 0x2c, 0x20, 0x70, 0x75, 0x62, 0x6c, 0x69, 0x73, 0x68, 0x2c, 0x20, 0x64, 0x69, 0x73, 0x74, 0x72, 0x69, 0x62, 0x75, 0x74, 0x65, 0x2c, 0x20, 0x73, 0x75, 0x62, 0x6c, 0x69, 0x63, 0x65, 0x6e, 0x73, 0x65, 0x2c, 0x20, 0x61, 0x6e, 0x64, 0x2f, 0x6f, 0x72, 0x20, 0x73, 0x65, 0x6c, 0x6c, 0x0a, 0x2d, 0x2d, 0x20, 0x63, 0x6f, 0x70, 0x69, 0x65, 0x73, 0x20, 0x6f, 0x66, 0x20, 0x74, 0x68, 0x65, 0x20, 0x53, 0x6f, 0x66, 0x74, 0x77, 0x61, 0x72, 0x65, 0x2c, 0x20, 0x61, 0x6e, 0x64, 0x20, 0x74, 0x6f, 0x20, 0x70, 0x65, 0x72, 0x6d, 0x69, 0x74, 0x20, 0x70, 0x65, 0x72, 0x73, 0x6f, 0x6e, 0x73, 0x20, 0x74, 0x6f, 0x20, 0x77, 0x68, 0x6f, 0x6d, 0x20, 0x74, 0x68, 0x65, 0x20, 0x53, 0x6f, 0x66, 0x74, 0x77, 0x61, 0x72, 0x65, 0x20, 0x69, 0x73, 0x0a, 0x2d, 0x2d, 0x20, 0x66, 0x75, 0x72, 0x6e, 0x69, 0x73, 0x68, 0x65, 0x64, 0x20, 0x74, 0x6f, 0x20, 0x64, 0x6f, 0x20, 0x73, 0x6f, 0x2c, 0x20, 0x73, 0x75, 0x62, 0x6a, 0x65, 0x63, 0x74, 0x20, 0x74, 0x6f, 0x20, 0x74, 0x68, 0x65, 0x20, 0x66, 0x6f, 0x6c, 0x6c, 0x6f, 0x77, 0x69, 0x6e, 0x67, 0x20, 0x63, 0x6f, 0x6e, 0x64, 0x69, 0x74, 0x69, 0x6f, 0x6e, 0x73, 0x3a, 0x0a, 0x2d, 0x2d, 0x0a, 0x2d, 0x2d, 0x20, 0x54, 0x68, 0x65, 0x20, 0x61, 0x62, 0x6f, 0x76, 0x65, 0x20, 0x63, 0x6f, 0x70, 0x79, 0x72, 0x69, 0x67, 0x68, 0x74, 0x20, 0x6e, 0x6f, 0x74, 0x69, 0x63, 0x65, 0x20, 0x61, 0x6e, 0x64, 0x20, 0x74, 0x68, 0x69, 0x73, 0x20, 0x70, 0x65, 0x72, 0x6d, 0x69, 0x73, 0x73, 0x69, 0x6f, 0x6e, 0x20, 0x6e, 0x6f, 0x74, 0x69, 0x63, 0x65, 0x20, 0x73, 0x68, 0x61, 0x6c, 0x6c, 0x20, 0x62, 0x65, 0x20, 0x69, 0x6e, 0x63, 0x6c, 0x75, 0x64, 0x65, 0x64, 0x20, 0x69, 0x6e, 0x0a, 0x2d, 0x2d, 0x20, 0x61, 0x6c, 0x6c, 0x20, 0x63, 0x6f, 0x70, 0x69, 0x65, 0x73, 0x20, 0x6f, 0x72, 0x20, 0x73, 0x75, 0x62, 0x73, 0x74, 0x61, 0x6e, 0x74, 0x69, 0x61, 0x6c, 0x20, 0x70, 0x6f, 0x72, 0x74, 0x69, 0x6f, 0x6e, 0x73, 0x20, 0x6f, 0x66, 0x20, 0x74, 0x68, 0x65, 0x20, 0x53, 0x6f, 0x66, 0x74, 0x77, 0x61, 0x72, 0x65, 0x2e, 0x0a, 0x2d, 0x2d, 0x0a, 0x2d, 0x2d, 0x20, 0x54, 0x48, 0x45, 0x20, 0x53, 0x4f, 0x46, 0x54, 0x57, 0x41, 0x52, 0x45, 0x20, 0x49, 0x53, 0x20, 0x50, 0x52, 0x4f, 0x56, 0x49, 0x44, 0x45, 0x44, 0x20, 0x22, 0x41, 0x53, 0x20, 0x49, 0x53, 0x22, 0x2c, 0x20, 0x57, 0x49, 0x54, 0x48, 0x4f, 0x55, 0x54, 0x20, 0x57, 0x41, 0x52, 0x52, 0x41, 0x4e, 0x54, 0x59, 0x20, 0x4f, 0x46, 0x20, 0x41, 0x4e, 0x59, 0x20, 0x4b, 0x49, 0x4e, 0x44, 0x2c, 0x20, 0x45, 0x58, 0x50, 0x52, 0x45, 0x53, 0x53, 0x20, 0x4f, 0x52, 0x0a, 0x2d, 0x2d, 0x20, 0x49, 0x4d, 0x50, 0x4c, 0x49, 0x45, 0x44, 0x2c, 0x20, 0x49, 0x4e, 0x43, 0x4c, 0x55, 0x44, 0x49, 0x4e, 0x47, 0x20, 0x42, 0x55, 0x54, 0x20, 0x4e, 0x4f, 0x54, 0x20, 0x4c, 0x49, 0x4d, 0x49, 0x54, 0x45, 0x44, 0x20, 0x54, 0x4f, 0x20, 0x54, 0x48, 0x45, 0x20, 0x57, 0x41, 0x52, 0x52, 0x41, 0x4e, 0x54, 0x49, 0x45, 0x53, 0x20, 0x4f, 0x46, 0x20, 0x4d, 0x45, 0x52, 0x43, 0x48, 0x41, 0x4e, 0x54, 0x41, 0x42, 0x49, 0x4c, 0x49, 0x54, 0x59, 0x2c, 0x0a, 0x2d, 0x2d, 0x20, 0x46, 0x49, 0x54, 0x4e, 0x45, 0x53, 0x53, 0x20, 0x46, 0x4f, 0x52, 0x20, 0x41, 0x20, 0x50, 0x41, 0x52, 0x54, 0x49, 0x43, 0x55, 0x4c, 0x41, 0x52, 0x20, 0x50, 0x55, 0x52, 0x50, 0x4f, 0x53, 0x45, 0x20, 0x41, 0x4e, 0x44, 0x20, 0x4e, 0x4f, 0x4e, 0x49, 0x4e, 0x46, 0x52, 0x49, 0x4e, 0x47, 0x45, 0x4d, 0x45, 0x4e, 0x54, 0x2e, 0x20, 0x49, 0x4e, 0x20, 0x4e, 0x4f, 0x20, 0x45, 0x56, 0x45, 0x4e, 0x54, 0x20, 0x53, 0x48, 0x41, 0x4c, 0x4c, 0x20, 0x54, 0x48, 0x45, 0x0a, 0x2d, 0x2d, 0x20, 0x41, 0x55, 0x54, 0x48, 0x4f, 0x52, 0x53, 0x20, 0x4f, 0x52, 0x20, 0x43, 0x4f, 0x50, 0x59, 0x52, 0x49, 0x47, 0x48, 0x54, 0x20, 0x48, 0x4f, 0x4c, 0x44, 0x45, 0x52, 0x53, 0x20, 0x42, 0x45, 0x20, 0x4c, 0x49, 0x41, 0x42, 0x4c, 0x45, 0x20, 0x46, 0x4f, 0x52, 0x20, 0x41, 0x4e, 0x59, 0x20, 0x43, 0x4c, 0x41, 0x49, 0x4d, 0x2c, 0x20, 0x44, 0x41, 0x4d, 0x41, 0x47, 0x45, 0x53, 0x20, 0x4f, 0x52, 0x20, 0x4f, 0x54, 0x48, 0x45, 0x52, 0x0a, 0x2d, 0x2d, 0x20, 0x4c, 0x49, 0x41, 0x42, 0x49, 0x4c, 0x49, 0x54, 0x59, 0x2c, 0x20, 0x57, 0x48, 0x45, 0x54, 0x48, 0x45, 0x52, 0x20, 0x49, 0x4e, 0x20, 0x41, 0x4e, 0x20, 0x41, 0x43, 0x54, 0x49, 0x4f, 0x4e, 0x20, 0x4f, 0x46, 0x20, 0x43, 0x4f, 0x4e, 0x54, 0x52, 0x41, 0x43, 0x54, 0x2c, 0x20, 0x54, 0x4f, 0x52, 0x54, 0x20, 0x4f, 0x52, 0x20, 0x4f, 0x54, 0x48, 0x45, 0x52, 0x57, 0x49, 0x53, 0x45, 0x2c, 0x20, 0x41, 0x52, 0x49, 0x53, 0x49, 0x4e, 0x47, 0x20, 0x46, 0x52, 0x4f, 0x4d, 0x2c, 0x0a, 0x2d, 0x2d, 0x20, 0x4f, 0x55, 0x54, 0x20, 0x4f, 0x46, 0x20, 0x4f, 0x52, 0x20, 0x49, 0x4e, 0x20, 0x43, 0x4f, 0x4e, 0x4e, 0x45, 0x43, 0x54, 0x49, 0x4f, 0x4e, 0x20, 0x57, 0x49, 0x54, 0x48, 0x20, 0x54, 0x48, 0x45, 0x20, 0x53, 0x4f, 0x46, 0x54, 0x57, 0x41, 0x52, 0x45, 0x20, 0x4f, 0x52, 0x20, 0x54, 0x48, 0x45, 0x20, 0x55, 0x53, 0x45, 0x20, 0x4f, 0x52, 0x20, 0x4f, 0x54, 0x48, 0x45, 0x52, 0x20, 0x44, 0x45, 0x41, 0x4c, 0x49, 0x4e, 0x47, 0x53, 0x20, 0x49, 0x4e, 0x0a, 0x2d, 0x2d, 0x20, 0x54, 0x48, 0x45, 0x20, 0x53, 0x4f, 0x46, 0x54, 0x57, 0x41, 0x52, 0x45, 0x2e, 0x0a, 0x0a, 0x0a, 0x2d, 0x2d, 0x20, 0x73, 0x75, 0x70, 0x70, 0x6f, 0x72, 0x74, 0x20, 0x64, 0x65, 0x74, 0x65, 0x63, 0x74, 0x69, 0x6f, 0x6e, 0x0a, 0x6c, 0x6f, 0x63, 0x61, 0x6c, 0x20, 0x66, 0x75, 0x6e, 0x63, 0x74, 0x69, 0x6f, 0x6e, 0x20, 0x69, 0x73, 0x57, 0x69, 0x6e, 0x64, 0x6f, 0x77, 0x73, 0x28, 0x29, 0x0a, 0x20, 0x20, 0x72, 0x65, 0x74, 0x75, 0x72, 0x6e, 0x20, 0x74, 0x79, 0x70, 0x65, 0x28, 0x70, 0x61, 0x63, 0x6b, 0x61, 0x67, 0x65, 0x29, 0x20, 0x3d, 0x3d, 0x20, 0x27, 0x74, 0x61, 0x62, 0x6c, 0x65, 0x27, 0x20, 0x61, 0x6e, 0x64, 0x20, 0x74, 0x79, 0x70, 0x65, 0x28, 0x70, 0x61, 0x63, 0x6b, 0x61, 0x67, 0x65, 0x2e, 0x63, 0x6f, 0x6e, 0x66, 0x69, 0x67, 0x29, 0x20, 0x3d, 0x3d, 0x20, 0x27, 0x73, 0x74, 0x72, 0x69, 0x6e, 0x67, 0x27, 0x20, 0x61, 0x6e, 0x64, 0x20, 0x70, 0x61, 0x63, 0x6b, 0x61, 0x67, 0x65, 0x2e, 0x63, 0x6f, 0x6e, 0x66, 0x69, 0x67, 0x3a, 0x73, 0x75, 0x62, 0x28, 0x31, 0x2c, 0x31, 0x29, 0x20, 0x3d, 0x3d, 0x20, 0x27, 0x5c, 0x5c, 0x27, 0x0a, 0x65, 0x6e, 0x64, 0x0a, 0x0a, 0x6c, 0x6f, 0x63, 0x61, 0x6c, 0x20, 0x73, 0x75, 0x70, 0x70, 0x6f, 0x72, 0x74, 0x65, 0x64, 0x20, 0x3d, 0x20, 0x6e, 0x6f, 0x74, 0x20, 0x69, 0x73, 0x57, 0x69, 0x6e, 0x64, 0x6f, 0x77, 0x73, 0x28, 0x29, 0x0a, 0x69, 0x66, 0x20, 0x69, 0x73, 0x57, 0x69, 0x6e, 0x64, 0x6f, 0x77, 0x73, 0x28, 0x29, 0x20, 0x74, 0x68, 0x65, 0x6e, 0x20, 0x73, 0x75, 0x70, 0x70, 0x6f, 0x72, 0x74, 0x65, 0x64, 0x20, 0x3d, 0x20, 0x6f, 0x73, 0x2e, 0x67, 0x65, 0x74, 0x65, 0x6e, 0x76, 0x28, 0x22, 0x41, 0x4e, 0x53, 0x49, 0x43, 0x4f, 0x4e, 0x22, 0x29, 0x20, 0x65, 0x6e, 0x64, 0x0a, 0x0a, 0x6c, 0x6f, 0x63, 0x61, 0x6c, 0x20, 0x6b, 0x65, 0x79, 0x73, 0x20, 0x3d, 0x20, 0x7b, 0x0a, 0x20, 0x20, 0x2d, 0x2d, 0x20, 0x72, 0x65, 0x73, 0x65, 0x74, 0x0a, 0x20, 0x20, 0x72, 0x65, 0x73, 0x65, 0x74, 0x20, 0x3d, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x30, 0x2c, 0x0a, 0x0a, 0x20, 0x20, 0x2d, 0x2d, 0x20, 0x6d, 0x69, 0x73, 0x63, 0x0a, 0x20, 0x20, 0x62, 0x72, 0x69, 0x67, 0x68, 0x74, 0x20, 0x20, 0x20, 0x20, 0x20, 0x3d, 0x20, 0x31, 0x2c, 0x0a, 0x20, 0x20, 0x64, 0x69, 0x6d, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x3d, 0x20, 0x32, 0x2c, 0x0a, 0x20, 0x20, 0x75, 0x6e, 0x64, 0x65, 0x72, 0x6c, 0x69, 0x6e, 0x65, 0x20, 0x20, 0x3d, 0x20, 0x34, 0x2c, 0x0a, 0x20, 0x20, 0x62, 0x6c, 0x69, 0x6e, 0x6b, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x3d, 0x20, 0x35, 0x2c, 0x0a, 0x20, 0x20, 0x72, 0x65, 0x76, 0x65, 0x72, 0x73, 0x65, 0x20, 0x20, 0x20, 0x20, 0x3d, 0x20, 0x37, 0x2c, 0x0a, 0x20, 0x20, 0x68, 0x69, 0x64, 0x64, 0x65, 0x6e, 0x20, 0x20, 0x20, 0x20, 0x20, 0x3d, 0x20, 0x38, 0x2c, 0x0a, 0x0a, 0x20, 0x20, 0x2d, 0x2d, 0x20, 0x66, 0x6f, 0x72, 0x65, 0x67, 0x72, 0x6f, 0x75, 0x6e, 0x64, 0x20, 0x63, 0x6f, 0x6c, 0x6f, 0x72, 0x73, 0x0a, 0x20, 0x20, 0x62, 0x6c, 0x61, 0x63, 0x6b, 0x20, 0x20, 0x20, 0x20, 0x20, 0x3d, 0x20, 0x33, 0x30, 0x2c, 0x0a, 0x20, 0x20, 0x72, 0x65, 0x64, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x3d, 0x20, 0x33, 0x31, 0x2c, 0x0a, 0x20, 0x20, 0x67, 0x72, 0x65, 0x65, 0x6e, 0x20, 0x20, 0x20, 0x20, 0x20, 0x3d, 0x20, 0x33, 0x32, 0x2c, 0x0a, 0x20, 0x20, 0x79, 0x65, 0x6c, 0x6c, 0x6f, 0x77, 0x20, 0x20, 0x20, 0x20, 0x3d, 0x20, 0x33, 0x33, 0x2c, 0x0a, 0x20, 0x20, 0x62, 0x6c, 0x75, 0x65, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x3d, 0x20, 0x33, 0x34, 0x2c, 0x0a, 0x20, 0x20, 0x6d, 0x61, 0x67, 0x65, 0x6e, 0x74, 0x61, 0x20, 0x20, 0x20, 0x3d, 0x20, 0x33, 0x35, 0x2c, 0x0a, 0x20, 0x20, 0x63, 0x79, 0x61, 0x6e, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x3d, 0x20, 0x33, 0x36, 0x2c, 0x0a, 0x20, 0x20, 0x77, 0x68, 0x69, 0x74, 0x65, 0x20, 0x20, 0x20, 0x20, 0x20, 0x3d, 0x20, 0x33, 0x37, 0x2c, 0x0a, 0x0a, 0x20, 0x20, 0x2d, 0x2d, 0x20, 0x62, 0x61, 0x63, 0x6b, 0x67, 0x72, 0x6f, 0x75, 0x6e, 0x64, 0x20, 0x63, 0x6f, 0x6c, 0x6f, 0x72, 0x73, 0x0a, 0x20, 0x20, 0x62, 0x6c, 0x61, 0x63, 0x6b, 0x62, 0x67, 0x20, 0x20, 0x20, 0x3d, 0x20, 0x34, 0x30, 0x2c, 0x0a, 0x20, 0x20, 0x72, 0x65, 0x64, 0x62, 0x67, 0x20, 0x20, 0x20, 0x20, 0x20, 0x3d, 0x20, 0x34, 0x31, 0x2c, 0x0a, 0x20, 0x20, 0x67, 0x72, 0x65, 0x65, 0x6e, 0x62, 0x67, 0x20, 0x20, 0x20, 0x3d, 0x20, 0x34, 0x32, 0x2c, 0x0a, 0x20, 0x20, 0x79, 0x65, 0x6c, 0x6c, 0x6f, 0x77, 0x62, 0x67, 0x20, 0x20, 0x3d, 0x20, 0x34, 0x33, 0x2c, 0x0a, 0x20, 0x20, 0x62, 0x6c, 0x75, 0x65, 0x62, 0x67, 0x20, 0x20, 0x20, 0x20, 0x3d, 0x20, 0x34, 0x34, 0x2c, 0x0a, 0x20, 0x20, 0x6d, 0x61, 0x67, 0x65, 0x6e, 0x74, 0x61, 0x62, 0x67, 0x20, 0x3d, 0x20, 0x34, 0x35, 0x2c, 0x0a, 0x20, 0x20, 0x63, 0x79, 0x61, 0x6e, 0x62, 0x67, 0x20, 0x20, 0x20, 0x20, 0x3d, 0x20, 0x34, 0x36, 0x2c, 0x0a, 0x20, 0x20, 0x77, 0x68, 0x69, 0x74, 0x65, 0x62, 0x67, 0x20, 0x20, 0x20, 0x3d, 0x20, 0x34, 0x37, 0x0a, 0x7d, 0x0a, 0x0a, 0x6c, 0x6f, 0x63, 0x61, 0x6c, 0x20, 0x65, 0x73, 0x63, 0x61, 0x70, 0x65, 0x53, 0x74, 0x72, 0x69, 0x6e, 0x67, 0x20, 0x3d, 0x20, 0x73, 0x74, 0x72, 0x69, 0x6e, 0x67, 0x2e, 0x63, 0x68, 0x61, 0x72, 0x28, 0x32, 0x37, 0x29, 0x20, 0x2e, 0x2e, 0x20, 0x27, 0x5b, 0x25, 0x64, 0x6d, 0x27, 0x0a, 0x6c, 0x6f, 0x63, 0x61, 0x6c, 0x20, 0x66, 0x75, 0x6e, 0x63, 0x74, 0x69, 0x6f, 0x6e, 0x20, 0x65, 0x73, 0x63, 0x61, 0x70, 0x65, 0x4e, 0x75, 0x6d, 0x62, 0x65, 0x72, 0x28, 0x6e, 0x75, 0x6d, 0x62, 0x65, 0x72, 0x29, 0x0a, 0x20, 0x20, 0x72, 0x65, 0x74, 0x75, 0x72, 0x6e, 0x20, 0x65, 0x73, 0x63, 0x61, 0x70, 0x65, 0x53, 0x74, 0x72, 0x69, 0x6e, 0x67, 0x3a, 0x66, 0x6f, 0x72, 0x6d, 0x61, 0x74, 0x28, 0x6e, 0x75, 0x6d, 0x62, 0x65, 0x72, 0x29, 0x0a, 0x65, 0x6e, 0x64, 0x0a, 0x0a, 0x6c, 0x6f, 0x63, 0x61, 0x6c, 0x20, 0x66, 0x75, 0x6e, 0x63, 0x74, 0x69, 0x6f, 0x6e, 0x20, 0x65, 0x73, 0x63, 0x61, 0x70, 0x65, 0x4b, 0x65, 0x79, 0x73, 0x28, 0x73, 0x74, 0x72, 0x29, 0x0a, 0x0a, 0x20, 0x20, 0x69, 0x66, 0x20, 0x6e, 0x6f, 0x74, 0x20, 0x73, 0x75, 0x70, 0x70, 0x6f, 0x72, 0x74, 0x65, 0x64, 0x20, 0x74, 0x68, 0x65, 0x6e, 0x20, 0x72, 0x65, 0x74, 0x75, 0x72, 0x6e, 0x20, 0x22, 0x22, 0x20, 0x65, 0x6e, 0x64, 0x0a, 0x0a, 0x20, 0x20, 0x6c, 0x6f, 0x63, 0x61, 0x6c, 0x20, 0x62, 0x75, 0x66, 0x66, 0x65, 0x72, 0x20, 0x3d, 0x20, 0x7b, 0x7d, 0x0a, 0x20, 0x20, 0x6c, 0x6f, 0x63, 0x61, 0x6c, 0x20, 0x6e, 0x75, 0x6d, 0x62, 0x65, 0x72, 0x0a, 0x20, 0x20, 0x66, 0x6f, 0x72, 0x20, 0x77, 0x6f, 0x72, 0x64, 0x20, 0x69, 0x6e, 0x20, 0x73, 0x74, 0x72, 0x3a, 0x67, 0x6d, 0x61, 0x74, 0x63, 0x68, 0x28, 0x22, 0x25, 0x77, 0x2b, 0x22, 0x29, 0x20, 0x64, 0x6f, 0x0a, 0x20, 0x20, 0x20, 0x20, 0x6e, 0x75, 0x6d, 0x62, 0x65, 0x72, 0x20, 0x3d, 0x20, 0x6b, 0x65, 0x79, 0x73, 0x5b, 0x77, 0x6f, 0x72, 0x64, 0x5d, 0x0a, 0x20, 0x20, 0x20, 0x20, 0x61, 0x73, 0x73, 0x65, 0x72, 0x74, 0x28, 0x6e, 0x75, 0x6d, 0x62, 0x65, 0x72, 0x2c, 0x20, 0x22, 0x55, 0x6e, 0x6b, 0x6e, 0x6f, 0x77, 0x6e, 0x20, 0x6b, 0x65, 0x79, 0x3a, 0x20, 0x22, 0x20, 0x2e, 0x2e, 0x20, 0x77, 0x6f, 0x72, 0x64, 0x29, 0x0a, 0x20, 0x20, 0x20, 0x20, 0x74, 0x61, 0x62, 0x6c, 0x65, 0x2e, 0x69, 0x6e, 0x73, 0x65, 0x72, 0x74, 0x28, 0x62, 0x75, 0x66, 0x66, 0x65, 0x72, 0x2c, 0x20, 0x65, 0x73, 0x63, 0x61, 0x70, 0x65, 0x4e, 0x75, 0x6d, 0x62, 0x65, 0x72, 0x28, 0x6e, 0x75, 0x6d, 0x62, 0x65, 0x72, 0x29, 0x20, 0x29, 0x0a, 0x20, 0x20, 0x65, 0x6e, 0x64, 0x0a, 0x0a, 0x20, 0x20, 0x72, 0x65, 0x74, 0x75, 0x72, 0x6e, 0x20, 0x74, 0x61, 0x62, 0x6c, 0x65, 0x2e, 0x63, 0x6f, 0x6e, 0x63, 0x61, 0x74, 0x28, 0x62, 0x75, 0x66, 0x66, 0x65, 0x72, 0x29, 0x0a, 0x65, 0x6e, 0x64, 0x0a, 0x0a, 0x6c, 0x6f, 0x63, 0x61, 0x6c, 0x20, 0x66, 0x75, 0x6e, 0x63, 0x74, 0x69, 0x6f, 0x6e, 0x20, 0x72, 0x65, 0x70, 0x6c, 0x61, 0x63, 0x65, 0x43, 0x6f, 0x64, 0x65, 0x73, 0x28, 0x73, 0x74, 0x72, 0x29, 0x0a, 0x20, 0x20, 0x73, 0x74, 0x72, 0x20, 0x3d, 0x20, 0x73, 0x74, 0x72, 0x69, 0x6e, 0x67, 0x2e, 0x67, 0x73, 0x75, 0x62, 0x28, 0x73, 0x74, 0x72, 0x2c, 0x22, 0x28, 0x25, 0x25, 0x7b, 0x28, 0x2e, 0x2d, 0x29, 0x7d, 0x29, 0x22, 0x2c, 0x20, 0x66, 0x75, 0x6e, 0x63, 0x74, 0x69, 0x6f, 0x6e, 0x28, 0x5f, 0x2c, 0x20, 0x73, 0x74, 0x72, 0x29, 0x20, 0x72, 0x65, 0x74, 0x75, 0x72, 0x6e, 0x20, 0x65, 0x73, 0x63, 0x61, 0x70, 0x65, 0x4b, 0x65, 0x79, 0x73, 0x28, 0x73, 0x74, 0x72, 0x29, 0x20, 0x65, 0x6e, 0x64, 0x20, 0x29, 0x0a, 0x20, 0x20, 0x72, 0x65, 0x74, 0x75, 0x72, 0x6e, 0x20, 0x73, 0x74, 0x72, 0x0a, 0x65, 0x6e, 0x64, 0x0a, 0x0a, 0x2d, 0x2d, 0x20, 0x70, 0x75, 0x62, 0x6c, 0x69, 0x63, 0x0a, 0x0a, 0x6c, 0x6f, 0x63, 0x61, 0x6c, 0x20, 0x66, 0x75, 0x6e, 0x63, 0x74, 0x69, 0x6f, 0x6e, 0x20, 0x61, 0x6e, 0x73, 0x69, 0x63, 0x6f, 0x6c, 0x6f, 0x72, 0x73, 0x28, 0x20, 0x73, 0x74, 0x72, 0x20, 0x29, 0x0a, 0x20, 0x20, 0x73, 0x74, 0x72, 0x20, 0x3d, 0x20, 0x74, 0x6f, 0x73, 0x74, 0x72, 0x69, 0x6e, 0x67, 0x28, 0x73, 0x74, 0x72, 0x20, 0x6f, 0x72, 0x20, 0x27, 0x27, 0x29, 0x0a, 0x0a, 0x20, 0x20, 0x72, 0x65, 0x74, 0x75, 0x72, 0x6e, 0x20, 0x72, 0x65, 0x70, 0x6c, 0x61, 0x63, 0x65, 0x43, 0x6f, 0x64, 0x65, 0x73, 0x28, 0x27, 0x25, 0x7b, 0x72, 0x65, 0x73, 0x65, 0x74, 0x7d, 0x27, 0x20, 0x2e, 0x2e, 0x20, 0x73, 0x74, 0x72, 0x20, 0x2e, 0x2e, 0x20, 0x27, 0x25, 0x7b, 0x72, 0x65, 0x73, 0x65, 0x74, 0x7d, 0x27, 0x29, 0x0a, 0x65, 0x6e, 0x64, 0x0a, 0x0a, 0x0a, 0x72, 0x65, 0x74, 0x75, 0x72, 0x6e, 0x20, 0x73, 0x65, 0x74, 0x6d, 0x65, 0x74, 0x61, 0x74, 0x61, 0x62, 0x6c, 0x65, 0x28, 0x7b, 0x6e, 0x6f, 0x52, 0x65, 0x73, 0x65, 0x74, 0x20, 0x3d, 0x20, 0x72, 0x65, 0x70, 0x6c, 0x61, 0x63, 0x65, 0x43, 0x6f, 0x64, 0x65, 0x73, 0x7d, 0x2c, 0x20, 0x7b, 0x5f, 0x5f, 0x63, 0x61, 0x6c, 0x6c, 0x20, 0x3d, 0x20, 0x66, 0x75, 0x6e, 0x63, 0x74, 0x69, 0x6f, 0x6e, 0x20, 0x28, 0x5f, 0x2c, 0x20, 0x73, 0x74, 0x72, 0x29, 0x20, 0x72, 0x65, 0x74, 0x75, 0x72, 0x6e, 0x20, 0x61, 0x6e, 0x73, 0x69, 0x63, 0x6f, 0x6c, 0x6f, 0x72, 0x73, 0x20, 0x28, 0x73, 0x74, 0x72, 0x29, 0x20, 0x65, 0x6e, 0x64, 0x7d, 0x29, 
  };
  lua_pushlstring(L, (const char*)lua_require_1, sizeof(lua_require_1));
  lua_setfield(L, -2, "ansicolors");

  if (docall(L, 1, LUA_MULTRET))
  {
    const char *errmsg = lua_tostring(L, 1);
    if (errmsg)
    {
      fprintf(stderr, "%s\n", errmsg);
    }
    lua_close(L);
    return 1;
  }
  lua_close(L);
  return 0;
}
