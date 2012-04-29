
CFLAGS=-Wall -Werror -O0 -g $(shell pkg-config --cflags libmapper-0)
LDLIBS=-lglut -lGLU -lGLEW $(shell pkg-config --libs libmapper-0)

all: influence agentConnect

influence: influence.o influence_opengl.o influence_opengl.h

influence.o: influence.c influence_opengl.h
influence_opengl.o: influence_opengl.c influence_opengl.h
