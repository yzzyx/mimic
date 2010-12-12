CC?=gcc
CFLAGS+=-Wall -g
LDFLAGS+=-lncursesw -lexpat

OBJECTS=main.o directory.o list.o logo.o visual-list.o common.o config.o shortcut.o utf8.o

mimic: $(OBJECTS)
	$(CC) -o mimic $(LDFLAGS) $(OBJECTS)

test: test.o common.o
	$(CC) -o test test.o common.o
	

clean:
	rm $(OBJECTS) mimic
