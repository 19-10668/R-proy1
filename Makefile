CXX ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -Wpedantic
.PHONY: all clean test
all: server client
server: src/server.cpp src/common.hpp
	$(CXX) $(CXXFLAGS) src/server.cpp -o servidor
client: src/client.cpp src/common.hpp
	$(CXX) $(CXXFLAGS) src/client.cpp -o cliente
clean:
	rm -f servidor cliente
