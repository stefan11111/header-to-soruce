.POSIX:

all:
	${CC} ${CFLAGS} -std=c99 -Wall -Wextra -pedantic header-to-source.c -o header-to-source
	${CC} ${CFLAGS} -std=c99 -Wall -Wextra -pedantic remove-comments.c -o remove-comments

clean:
	rm -f header-to-source remove-comments

install: header-to-source remove-comments
	mkdir -p ${DESTDIR}/usr/bin
	cp -f header-to-source ${DESTDIR}/usr/bin/header-to-source
	cp -f remove-comments ${DESTDIR}/usr/bin/remove-comments

uninstall:
	rm -f ${DESTDIR}/usr/bin/header-to-source ${DESTDIR}/usr/bin/remove-comments

.PHONY: all clean install uninstall

