all: 
	gcc -o ptmax ptmax.c
	strip ptmax

install: 
	install  ptmax -D $(DESTDIR)/usr/bin/ptmax

uninstall:
	rm -f /usr/bin/ptmax

clean:
	rm -f ptmax

deb-pkg:
	dpkg-buildpackage -b
