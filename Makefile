
CFLAGS=-Wall -Werror -O0 -g $(shell pkg-config --cflags libmapper-0)
LDLIBS=$(shell pkg-config --libs libmapper-0)
FRAMEWORKS=$(wildcard /System/Library/Frameworks)

ifeq ($(FRAMEWORKS),/System/Library/Frameworks)
CFLAGS += -I/System/Library/Frameworks/OpenGL.framework/Headers \
          -I/System/Library/Frameworks/GLUT.framework/Headers
LDLIBS += -framework OpenGL -framework GLUT
else
LDLIBS += -lglut -lGLU -lGLEW
endif

all: influence agentConnect

influence: influence.o influence_opengl.o influence_opengl.h

influence.o: influence.c influence_opengl.h
influence_opengl.o: influence_opengl.c influence_opengl.h
