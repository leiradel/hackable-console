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
	-Isrc -Isrc/deps/imgui -Isrc/deps/imgui/backends -Isrc/deps/ImGuiAl/term -Isrc/deps/ImGuiAl/fonts \
	-Isrc/deps/ImGuiAl/button -Isrc/deps/ImGuiAl/sparkline -Isrc/deps/IconFontCppHeaders \
	-Isrc/deps/ImGui-Addons/addons/imguifilesystem -Isrc/dynlib -Iinclude \
	-Isrc/fnkdat -Isrc/speex -Isrc/deps/lrcpp/include -Isrc/deps/lua -Isrc/deps/luafilesystem/src \
	-Isrc/deps/imgui_club/imgui_memory_editor -Isrc/deps/chips/util
DEFINES=-DIMGUI_DISABLE_WIN32_DEFAULT_IME_FUNCS -D"IM_ASSERT(x)=do{(void)(x);}while(0)"
DEFINES+=-DOUTSIDE_SPEEX -DRANDOM_PREFIX=speex -DEXPORT= -D_USE_SSE2 -DFIXED_POINT
DEFINES+=-DPACKAGE=\"hackable-console\" -DDEBUG_FSM
CFLAGS+=$(INCLUDES) $(DEFINES) `sdl2-config --cflags`
CXXFLAGS=$(CFLAGS) -std=c++11
LDFLAGS=
LIBS+=`sdl2-config --libs`

# hackable-console
HC_OBJS=\
	src/main.o src/Application.o src/LifeCycle.o src/Fifo.o src/LuaUtil.o \
	src/Audio.o src/Config.o src/Control.o src/Logger.o src/Memory.o src/Video.o \
	src/Led.o src/Input.o src/Perf.o src/Desktop.o src/Timer.o src/Devices.o \
	src/dynlib/dynlib.o src/fnkdat/fnkdat.o src/speex/resample.o src/Debugger.o \
	src/Cpu.o src/Disasm.o src/Handle.o src/cpus/Z80.o

# lrcpp
LRCPP_OBJS=\
	src/deps/lrcpp/src/Frontend.o src/deps/lrcpp/src/Core.o src/deps/lrcpp/src/Components.o \
	src/deps/lrcpp/src/CoreFsm.o

# imgui
IMGUI_OBJS=\
	src/deps/imgui/imgui.o src/deps/imgui/imgui_draw.o \
	src/deps/imgui/imgui_tables.o src/deps/imgui/imgui_widgets.o src/deps/imgui/imgui_demo.o \
	src/deps/imgui/backends/imgui_impl_sdl.o src/deps/imgui/backends/imgui_impl_opengl2.o

# imgui extras
IMGUIEXTRA_OBJS=\
	src/deps/ImGuiAl/term/imguial_term.o \
	src/deps/ImGui-Addons/addons/imguifilesystem/imguifilesystem.o

# lua
LUA_OBJS=\
	src/deps/lua/lapi.o src/deps/lua/lcode.o src/deps/lua/lctype.o src/deps/lua/ldebug.o \
	src/deps/lua/ldo.o src/deps/lua/ldump.o src/deps/lua/lfunc.o src/deps/lua/lgc.o src/deps/lua/llex.o \
	src/deps/lua/lmem.o src/deps/lua/lobject.o src/deps/lua/lopcodes.o src/deps/lua/lparser.o \
	src/deps/lua/lstate.o src/deps/lua/lstring.o src/deps/lua/ltable.o src/deps/lua/ltm.o \
	src/deps/lua/lundump.o src/deps/lua/lvm.o src/deps/lua/lzio.o src/deps/lua/lauxlib.o \
	src/deps/lua/lbaselib.o src/deps/lua/lcorolib.o src/deps/lua/ldblib.o \
	src/deps/lua/liolib.o src/deps/lua/lmathlib.o src/deps/lua/loslib.o src/deps/lua/lstrlib.o \
	src/deps/lua/ltablib.o src/deps/lua/lutf8lib.o src/deps/lua/loadlib.o src/deps/lua/linit.o \
	src/deps/luafilesystem/src/lfs.o

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -Wall -Wpedantic -Werror -c $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) -Wall -Wpedantic -Werror -c $< -o $@

src/deps/lua/%.o: src/deps/lua/%.c
	$(CC) $(CFLAGS) -c $< -o $@

src/deps/ImGui-Addons/addons/imguifilesystem/%.o: src/deps/ImGui-Addons/addons/imguifilesystem/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

all: hackcon

hackcon: $(HC_OBJS) $(LRCPP_OBJS) $(IMGUI_OBJS) $(IMGUIEXTRA_OBJS) $(LUA_OBJS)
	$(CXX) $(LDFLAGS) -o $@ $+ $(LIBS)

src/gamecontrollerdb.h: src/deps/SDL_GameControllerDB/gamecontrollerdb.txt
	xxd -i $< | sed "s/unsigned char/static char const/" \
		| sed "s/unsigned int/static size_t const/" \
		| sed "s/src_deps_SDL_GameControllerDB_gamecontrollerdb_txt/gamecontrollerdb/" > $@

src/main.cpp: src/gamecontrollerdb.h

clean:
	rm -f hackcon $(HC_OBJS)

realclean:
	rm -f hackcon $(HC_OBJS) $(LRCPP_OBJS) $(IMGUI_OBJS) $(IMGUIEXTRA_OBJS) $(LUA_OBJS) src/gamecontrollerdb.h

.PHONY: clean
