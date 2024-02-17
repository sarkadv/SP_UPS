CC=g++ -std=c++11
CFLAGS=-Wall -pedantic
OBJS=state_machine.o graph.o game_structures.o commands.o parser.o config.o logger.o server.o
EXE=server

all: $(EXE) clean

$(EXE): $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@ -lm

.cpp.o:
	$(CC) $(CFLAGS) -c $^

clean:
	rm -f *.o

