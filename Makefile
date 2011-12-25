all:
	gcc -o ptmax ptmax.c

install:
	install -Ts ptmax /usr/bin/ptmax	

uninstall:
	rm -f /usr/bin/ptmax

