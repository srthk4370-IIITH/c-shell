CC = gcc
CFLAGS = -Wall -Wextra -Iinclude

OBJECTS = main.o lexer.o parser.o echo.o cmd.o

c-shell: $(OBJECTS)
	$(CC) $(OBJECTS) -o c-shell

clean:
	rm -f $(OBJECTS) c-shell