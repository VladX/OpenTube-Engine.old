all: build
rebuild: clean build
build:
	$(MAKE) --directory=./objs --makefile=./Makefile
clean:
	rm -f ./objs/*.o ./opentube-server
