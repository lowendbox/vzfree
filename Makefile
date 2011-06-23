CFLAGS=-O2 -s
PREFIX=/usr/local
TARGET=vzfree

all:	${TARGET}

clean:
	rm -f ${TARGET}

vzfree:	vzfree.c

install: vzfree
	install -s ${TARGET} ${PREFIX}/bin/
