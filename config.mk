# version
VERSION = 0.1
TARGET = snack
SRCDIR = src
OBJDIR = build

# Customize below to fit your system

# paths
PREFIX = /usr/local
MANPREFIX = ${PREFIX}/share/man

# includes and libs (ncurses)
INCS = -I. -I/usr/include
LIBS = -L/usr/lib -lc -lncurses

# flags
CPPFLAGS = -DVERSION=\"${VERSION}\" -DSIGWINCH=28
CFLAGS = -g -std=c11 -x c -Wall -Wextra -pedantic -O0 ${INCS} ${CPPFLAGS}
LDFLAGS = ${LIBS}

# compiler and linker
CC = clang
