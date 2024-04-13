server: server.c
	gcc -o server server.c -pthread

run: server
	./server

clean:
	rm -f server

debug:
	gcc -g -o server server.c -lpthread
	gdb ./server