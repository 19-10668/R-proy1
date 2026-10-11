
all: *.cpp
	g++ server.cpp -o server
	g++ client.cpp -o client 



clean:
	rm -f server
	rm -f server
