all: client server

client: client.cpp common.h
	g++ -std=c++11 client.cpp -o client -lrt -lpthread

server: server.cpp common.h
	g++ -std=c++11 server.cpp -o server -lrt -lpthread

clean:
	rm -f client server

