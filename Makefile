PYTHON= python

all: build
full-rebuild: gen-callbacks.c rebuild
rebuild: clean build
build:
	$(MAKE) --directory=./objs --makefile=./Makefile
clean:
	rm -f ./objs/*.o ./opentube-server
gen-callbacks.c:
	$(PYTHON) ./src/web/callbacks/generate-callbacks.c.py ./src/web
