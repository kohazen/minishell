CC = gcc
CFLAGS = -Wall -g

all: myshell

myshell: myshell.c
    $(CC) $(CFLAGS) -o myshell myshell.c

clean:
    rm -f myshell
