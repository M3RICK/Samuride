CC = g++
CFLAGS = -Wall -Wextra -g -O2 -std=c++20 -I./src -MMD -MP
LDFLAGS = -pthread
EXE =

# TARGET=windows with a MinGW compiler (CC=x86_64-w64-mingw32-g++): .exe files, Winsock,
# no console window for the game, and the C++ runtime linked in
ifeq ($(TARGET),windows)
EXE = .exe
LDFLAGS += -static-libgcc -static-libstdc++ -Wl,-Bstatic -lpthread -Wl,-Bdynamic
WINDOWS_LIBS = -lws2_32
CLIENT_WINDOWS = -mwindows
endif

# macOS: libc++ needs 13.3 or newer for std::format
ifeq ($(shell uname -s),Darwin)
MACOS = 1
CFLAGS += -mmacosx-version-min=13.3
endif

# Set SFML_DIR if SFML 3 is not installed system-wide.
# RPATH is where the binaries look for it at run time: '$$ORIGIN/lib' for a relocatable Linux
# release, '@executable_path/lib' on macOS.
ifdef SFML_DIR
RPATH ?= $(SFML_DIR)/lib
CFLAGS += -I$(SFML_DIR)/include
LDFLAGS += -L$(SFML_DIR)/lib
ifdef MACOS
LDFLAGS += -Wl,-rpath,'$(RPATH)'
else ifneq ($(TARGET),windows)
LDFLAGS += -Wl,--disable-new-dtags,-rpath,'$(RPATH)'
endif
endif

# Common sources
COMMON_SRCS = src/common/debug.cpp src/common/protocol.cpp src/common/map.cpp src/common/socket.cpp src/common/platform.cpp

# Server sources; the game links the server core too, to host solo and multiplayer runs itself
SERVER_CORE_SRCS = src/server/server.cpp src/server/logic.cpp src/server/player.cpp src/server/generator.cpp \
                   src/server/combat.cpp src/server/titans.cpp src/server/shift.cpp
SERVER_SRCS = src/server/main.cpp $(SERVER_CORE_SRCS)

# Client sources
CLIENT_SRCS = src/client/main.cpp src/client/client.cpp src/client/inputs.cpp src/client/state.cpp \
              src/client/menu.cpp src/client/characters.cpp src/client/audio.cpp src/client/settings.cpp \
              $(wildcard src/client/render/*.cpp)
CLIENT_LDFLAGS = -lsfml-graphics -lsfml-window -lsfml-system -lsfml-audio

# Object files
COMMON_OBJS = $(COMMON_SRCS:.cpp=.o)
SERVER_OBJS = $(SERVER_SRCS:.cpp=.o)
SERVER_CORE_OBJS = $(SERVER_CORE_SRCS:.cpp=.o)
CLIENT_OBJS = $(CLIENT_SRCS:.cpp=.o)
ALL_OBJS = $(COMMON_OBJS) $(SERVER_OBJS) $(CLIENT_OBJS) src/common/tests.o

# Executables
SERVER_BIN = jetpack_server$(EXE)
CLIENT_BIN = jetpack_client$(EXE)

# Rules
all: server client

server: $(SERVER_BIN)

client: $(CLIENT_BIN)

$(SERVER_BIN): $(SERVER_OBJS) $(COMMON_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(WINDOWS_LIBS)

$(CLIENT_BIN): $(CLIENT_OBJS) $(SERVER_CORE_OBJS) $(COMMON_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(CLIENT_LDFLAGS) $(WINDOWS_LIBS) $(CLIENT_WINDOWS)

%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

tests: src/common/tests.o src/server/generator.o $(COMMON_OBJS)
	$(CC) $(CFLAGS) -o run_tests $^ $(LDFLAGS)
	./run_tests

clean:
	rm -f $(ALL_OBJS) $(ALL_OBJS:.o=.d)

fclean: clean
	rm -f $(SERVER_BIN) $(CLIENT_BIN) run_tests

re: fclean all

-include $(ALL_OBJS:.o=.d)

.PHONY: all server client clean fclean re tests
