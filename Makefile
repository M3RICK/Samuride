CC = g++
CFLAGS = -Wall -Wextra -g -std=c++20 -I./src
LDFLAGS = -pthread

# Common sources
COMMON_SRCS = src/common/debug.cpp src/common/protocol.cpp src/common/map.cpp

# Server sources
SERVER_SRCS = src/server/main.cpp src/server/server.cpp src/server/logic.cpp src/server/player.cpp

# Client sources
CLIENT_SRCS = src/client/main.cpp src/client/client.cpp src/client/render.cpp src/client/inputs.cpp src/client/state.cpp
CLIENT_LDFLAGS = -lsfml-graphics -lsfml-window -lsfml-system -lsfml-audio

# Object files
COMMON_OBJS = $(COMMON_SRCS:.cpp=.o)
SERVER_OBJS = $(SERVER_SRCS:.cpp=.o)
CLIENT_OBJS = $(CLIENT_SRCS:.cpp=.o)

# Executables
SERVER_BIN = jetpack_server
CLIENT_BIN = jetpack_client

# Rules
all: server client

server: $(SERVER_OBJS) $(COMMON_OBJS)
	$(CC) $(CFLAGS) -o $(SERVER_BIN) $^ $(LDFLAGS)

client: $(CLIENT_OBJS) $(COMMON_OBJS)
	$(CC) $(CFLAGS) -o $(CLIENT_BIN) $^ $(LDFLAGS) $(CLIENT_LDFLAGS)

%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(COMMON_OBJS) $(SERVER_OBJS) $(CLIENT_OBJS)

fclean: clean
	rm -f $(SERVER_BIN) $(CLIENT_BIN)

re: fclean all

test_map: src/common/debug.o src/common/map.o src/common/test_map.cpp
	$(CC) $(CFLAGS) -o test_map src/common/test_map.cpp src/common/debug.o src/common/map.o

.PHONY: all server client clean fclean re test_map
