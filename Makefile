all: build
rebuild: clean build
build:
	$(MAKE) -f ./objs/Makefile
clean:
	rm -f ./objs/*.o ./opentube-server
