all:
	gcc ./src/main.c -o ./bin/out -Wall -lX11 

run:
	./bin/out