CC= gcc
LIBS= -pthread -lz
PARAMS= -Wall -Wunused -Wundef -Wpointer-arith -O2 $(DEBUG_PARAMS)

build:
	$(CC) $(PARAMS) ../src/*.c -c
	$(CC) $(PARAMS) -I ../src ../src/web/callbacks/callbacks.c -c
	$(CC) $(PARAMS) $(LIBS) ./*.o -o ../opentube-server
