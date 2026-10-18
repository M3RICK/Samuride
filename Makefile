CC = g++
CFLAGS = -Wall -Wextra -g -O2 -std=c++20 -I./src -MMD -MP
LDFLAGS = -pthread

# Set SFML_DIR if SFML 3 is not installed system-wide.
# RPATH is where the binaries look for it at run time ('$$ORIGIN/lib' for a relocatable release).
ifdef SFML_DIR
RPATH ?= $(SFML_DIR)/lib
CFLAGS += -I$(SFML_DIR)/include
LDFLAGS += -L$(SFML_DIR)/lib -Wl,--disable-new-dtags,-rpath,'$(RPATH)'
endif

# Common sources
COMMON_SRCS = src/common/debug.cpp src/common/protocol.cpp src/common/map.cpp src/common/socket.cpp

# Server sources
SERVER_SRCS = src/server/main.cpp src/server/server.cpp src/server/logic.cpp src/server/player.cpp src/server/generator.cpp

# Client sources
CLIENT_SRCS = src/client/main.cpp src/client/client.cpp src/client/render.cpp src/client/inputs.cpp src/client/state.cpp \
              src/client/render_menu.cpp src/client/render_world.cpp src/client/render_game.cpp \
              src/client/menu.cpp src/client/characters.cpp
CLIENT_LDFLAGS = -lsfml-graphics -lsfml-window -lsfml-system -lsfml-audio

# Object files
COMMON_OBJS = $(COMMON_SRCS:.cpp=.o)
SERVER_OBJS = $(SERVER_SRCS:.cpp=.o)
CLIENT_OBJS = $(CLIENT_SRCS:.cpp=.o)
ALL_OBJS = $(COMMON_OBJS) $(SERVER_OBJS) $(CLIENT_OBJS) src/common/tests.o

# Executables
SERVER_BIN = jetpack_server
CLIENT_BIN = jetpack_client

# Rules
all: server client

server: $(SERVER_BIN)

client: $(CLIENT_BIN)

$(SERVER_BIN): $(SERVER_OBJS) $(COMMON_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(CLIENT_BIN): $(CLIENT_OBJS) $(COMMON_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(CLIENT_LDFLAGS)

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
