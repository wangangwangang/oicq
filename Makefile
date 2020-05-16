all:ser cli

ser:server.c
	gcc server.c -o ser  -g -lmysqlclient  -lpthread

cli:client.c
	gcc client.c -o cli   -g   -lpthread
clean:
	rm  ser   cli
