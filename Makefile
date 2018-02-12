CXX = g++ -std=c++14
CPPFLAGS = -g -O0 -Wall -I./include
GTKFLAGS = `pkg-config --cflags --libs gtkmm-3.0`
COMP = $(CXX) $(CPPFLAGS) $^ -o $@
SRCO = pendulum.o pendulum_parser.o trie.o location.o vimserver.o
SRCDIR = ./src
OBJ = $(patsubst %, src/%, $(SRCO))
SRC = $(patsubst %.o, ./src/%.cc, $(SRCO))

harmonogram : src/harmonogram.cc $(OBJ)
	$(COMP) $(GTKFLAGS)

servertest : src/servertest.cc $(OBJ)
	$(COMP)

dependencies : update $(OBJ)

update :
	touch $(SRC)

location : location.o
	$(COMP)

pendulum : pendulum.o
	$(COMP)

pendulum_parser : pendulum.o pendulum_parser.o trie.o location.o
	$(COMP)

trie : trie.o
	$(COMP)

vimserver : vimserver.o
	$(COMP)
