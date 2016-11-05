CC = gcc
CFLAGS = -std=gnu11 -O2 -g -Wall -Wextra -Wformat=2 $(shell pkg-config glib-2.0 openssl --cflags) $(shell ncursesw5-config --cflags)
OBJFLAGS = -c

all: chatd nc

chat: chat.o getpasswd.o
	$(CC) $(CFLAGS) -o $@ $^ -lreadline $(shell pkg-config glib-2.0 openssl --libs)

chatd: chatd.o log.o client_connection.o util.o chatroom.o
	$(CC) $(CFLAGS) -o $@ $^ $(shell pkg-config glib-2.0 openssl --libs) -lm

getpassword.o: getpassword.c
	$(CC) $(CFLAGS) -o $@ $^ $(shell pkg-config glib-2.0 openssl --libs) -lm

line_buffer.o: line_buffer.c
	$(CC) $(OBJFLAGS) $(CFLAGS) -o $@ $^ $(shell pkg-config glib-2.0 openssl --libs) -lm

log.o: log.c
	$(CC) $(OBJFLAGS) $(CFLAGS) -o $@ $^ $(shell pkg-config glib-2.0 openssl --libs) -lm

client_connection.o: client_connection.c
	$(CC) $(OBJFLAGS) $(CFLAGS) -o $@ $^ $(shell pkg-config glib-2.0 openssl --libs) -lm

chatroom.o: chatroom.c
	$(CC) $(OBJFLAGS) $(CFLAGS) -o $@ $^ $(shell pkg-config glib-2.o openssl --libs) -lm

util.o: util.c
	$(CC) $(OBJFLAGS) $(CFLAGS) -o $@ $^ $(shell pkg-config glib-2.0 openssl --libs) -lm

nc: nc.o chat_ui.o log.o line_buffer.o util.o
	$(CC) $(CFLAGS) -o $@ $^ $(shell pkg-config glib-2.0 openssl --libs) $(shell ncursesw5-config --libs) -lm

chat_ui.o: chat_ui.c
	$(CC) $(OBJFLAGS) $(CFLAGS) -o $@ $^

clean:
	rm -f *.o *~

distclean: clean
	rm -f chatd chat
