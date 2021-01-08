# Platform setup
ifeq ($(shell uname -a),)
	LIBS=-lOpenGL32
else ifneq ($(findstring MINGW,$(shell uname -a)),)
	LIBS=-lOpenGL32
else
	LIBS=-lGL -ldl
endif

# Debug
ifeq ($(DEBUG), 1)
	CFLAGS=-O0 -g
else
	CFLAGS=-O2 -DNDEBUG
endif

# Toolset setup
CC=gcc
CXX=g++
INCLUDES=\
	-Isrc -Isrc/imgui -Isrc/imgui/backends -Isrc/ImGuiAl/term -Isrc/ImGuiAl/fonts \
	-Isrc/ImGuiAl/button -Isrc/IconFontCppHeaders -Isrc/ImGui-Addons/addons/imguifilesystem \
	-Isrc/fnkdat -Isrc/speex -Isrc/lrcpp/src -Isrc/lua -Isrc/luafilesystem/src \
	-Isrc/imgui_club/imgui_memory_editor
DEFINES=-DIMGUI_DISABLE_WIN32_DEFAULT_IME_FUNCS -D"IM_ASSERT(x)=do{(void)(x);}while(0)"
DEFINES+=-DOUTSIDE_SPEEX -DRANDOM_PREFIX=speex -DEXPORT= -D_USE_SSE2 -DFIXED_POINT
DEFINES+=-DPACKAGE=\"hackable-console\" -DDEBUG_FSM
CFLAGS+=$(INCLUDES) $(DEFINES) `sdl2-config --cflags`
CXXFLAGS=$(CFLAGS) -std=c++11
LDFLAGS=
LIBS+=`sdl2-config --libs`

# hackable-console
HC_OBJS=\
	src/main.o src/Application.o src/LifeCycle.o src/Fifo.o src/LuaBind.o src/LuaUtil.o \
	src/Audio.o src/Config.o src/Control.o src/Logger.o src/Memory.o src/Video.o \
	src/dynlib/dynlib.o src/fnkdat/fnkdat.o src/speex/resample.o 

# lrcpp
LRCPP_OBJS=\
	src/lrcpp/src/Frontend.o src/lrcpp/src/Core.o src/lrcpp/src/Components.o \
	src/lrcpp/src/CoreFsm.o

# imgui
IMGUI_OBJS=\
	src/imgui/imgui.o src/imgui/imgui_draw.o \
	src/imgui/imgui_tables.o src/imgui/imgui_widgets.o \
	src/imgui/backends/imgui_impl_sdl.o src/imgui/backends/imgui_impl_opengl2.o

# imgui extras
IMGUIEXTRA_OBJS=\
	src/ImGuiAl/term/imguial_term.o \
	src/ImGui-Addons/addons/imguifilesystem/imguifilesystem.o

# lua
LUA_OBJS=\
	src/lua/lapi.o src/lua/lcode.o src/lua/lctype.o src/lua/ldebug.o \
	src/lua/ldo.o src/lua/ldump.o src/lua/lfunc.o src/lua/lgc.o src/lua/llex.o \
	src/lua/lmem.o src/lua/lobject.o src/lua/lopcodes.o src/lua/lparser.o \
	src/lua/lstate.o src/lua/lstring.o src/lua/ltable.o src/lua/ltm.o \
	src/lua/lundump.o src/lua/lvm.o src/lua/lzio.o src/lua/lauxlib.o \
	src/lua/lbaselib.o src/lua/lcorolib.o src/lua/ldblib.o \
	src/lua/liolib.o src/lua/lmathlib.o src/lua/loslib.o src/lua/lstrlib.o \
	src/lua/ltablib.o src/lua/lutf8lib.o src/lua/loadlib.o src/lua/linit.o \
	src/luafilesystem/src/lfs.o

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -Wall -Wpedantic -Werror -c $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) -Wall -Wpedantic -Werror -c $< -o $@

src/lua/%.o: src/lua/%.c
	$(CC) $(CFLAGS) -c $< -o $@

src/ImGui-Addons/addons/imguifilesystem/%.o: src/ImGui-Addons/addons/imguifilesystem/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

all: hackcon

hackcon: $(HC_OBJS) $(LRCPP_OBJS) $(IMGUI_OBJS) $(IMGUIEXTRA_OBJS) $(LUA_OBJS)
	$(CXX) $(LDFLAGS) -o $@ $+ $(LIBS)

src/gamecontrollerdb.h: src/SDL_GameControllerDB/gamecontrollerdb.txt
	xxd -i $< | sed "s/unsigned char/static char const/" \
		| sed "s/unsigned int/static size_t const/" \
		| sed "s/src_SDL_GameControllerDB_gamecontrollerdb_txt/gamecontrollerdb/" > $@

src/main.cpp: src/gamecontrollerdb.h

clean:
	rm -f hackcon $(HC_OBJS)

realclean:
	rm -f hackcon $(HC_OBJS) $(LRCPP_OBJS) $(IMGUI_OBJS) $(IMGUIEXTRA_OBJS) $(LUA_OBJS) src/gamecontrollerdb.h

.PHONY: clean
