CC = gcc
CFLAGS = -std=gnu11 -O2 -g -Wall -Wextra -Wformat=2 $(shell pkg-config glib-2.0 openssl --cflags) $(shell ncursesw5-config --cflags)
#CFLAGS = -std=gnu11 -O2 -g $(shell pkg-config glib-2.0 openssl --cflags) $(shell ncursesw5-config --cflags)

all: chatd chat

chat: chat.o getpasswd.o
	$(CC) $(CFLAGS) -o $@ $^ -lreadline $(shell pkg-config glib-2.0 openssl --libs)

chatd: chatd.o
	$(CC) $(CFLAGS) -o $@ $^ $(shell pkg-config glib-2.0 openssl --libs) -lm

getpassword.o: getpassword.c
	$(CC) $(CFLAGS) -o $@ $^ $(shell pkg-config glib-2.0 openssl --libs) -lm

nc: nc.o
	$(CC) $(CFLAGS) -o $@ $^ $(shell pkg-config glib-2.0 openssl --libs) $(shell ncursesw5-config --libs) -lm

clean:
	rm -f *.o *~

distclean: clean
	rm -f chatd chat
