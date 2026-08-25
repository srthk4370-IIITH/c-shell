CC = gcc
CFLAGS = -Wall -Wextra -Iinclude

OBJECTS = src/main.o src/lexer.o src/parser.o src/echo.o src/cmd.o src/hop.o src/lower.o src/reveal.o src/locate.o src/peek.o src/grp.o

c-shell: $(OBJECTS)
	$(CC) $(OBJECTS) -o a

clean:
	rm -f $(OBJECTS) a