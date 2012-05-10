
CC=g++

CFLAGS=-Wall -Werror -O0 -g $(shell pkg-config --cflags libmapper-0)
CXXFLAGS=-Wall -Werror -O0 -g $(shell pkg-config --cflags libmapper-0) -I../../qualia/src
LDLIBS=$(shell pkg-config --libs libmapper-0) -L../../qualia/build -lqualia

all: qualiaAgent

qualiaAgent: qualiaAgent.o AutoConnect.o InfluenceEnvironment.o
