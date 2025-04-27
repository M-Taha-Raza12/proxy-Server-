CC = gcc
CFLAGS = -Wall -g -pthread
TARGET = proxy

all: $(TARGET)

$(TARGET): proxy_parse.o proxy_server_with_cache.o
	$(CC) $(CFLAGS) -o $(TARGET) proxy_parse.o proxy_server_with_cache.o -lpthread

proxy_parse.o: proxy_parse.c proxy_parse.h
	$(CC) $(CFLAGS) -c proxy_parse.c

proxy_server_with_cache.o: proxy_server_with_cache.c proxy_parse.h
	$(CC) $(CFLAGS) -c proxy_server_with_cache.c

clean:
	rm -f *.o $(TARGET)
