CC= gcc
LIBS= 
PARAMS= -Wall -Wunused -Wundef -Wpointer-arith -O2 $(DEBUG_PARAMS) $(LIBS)

build:
	$(CC) $(PARAMS) ./src/*.c -c
	mv ./*.o ./objs
	$(CC) $(PARAMS) ./objs/*.o -o ./opentube-server
