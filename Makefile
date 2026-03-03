CC = gcc
CFLAGS = -Wall -Wextra -std=c99
TARGET = server
SRC = server.c

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)
