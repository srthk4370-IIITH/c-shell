CC = gcc
CFLAGS = -Wall -Wextra -Iinclude

OBJECTS = main.o lexer.o parser.o echo.o cmd.o hop.o lower.o reveal.o locate.o peek.o grp.o

c-shell: $(OBJECTS)
	$(CC) $(OBJECTS) -o a

clean:
	rm -f $(OBJECTS) a