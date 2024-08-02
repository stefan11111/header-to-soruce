.POSIX:

all:
	${CC} ${CFLAGS} -std=c99 -Wall -Wextra -pedantic header-to-source.c -o header-to-source

clean:
	rm -f header-to-source

install: header-to-source
	mkdir -p ${DESTDIR}/usr/bin
	cp -f header-to-source ${DESTDIR}/usr/bin/header-to-source

uninstall:
	rm -f ${DESTDIR}/usr/bin/header-to-source

.PHONY: all clean install uninstall

