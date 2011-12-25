all: 
	gcc -o ptmax ptmax.c

install: 
	install -sd ptmax $(DESTDIR)/usr/bin

uninstall:
	rm -f /usr/bin/ptmax

clean:
	rm -f ptmax
