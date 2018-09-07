all: g940

FLAGS=$(CXXFLAGS) -march=native -O3
LIBS=-lusb-1.0

g940.o: g940.h g940.c
	g++ $(FLAGS) -std=c++0x -c g940.c
	
g940: g940.o
	g++ -o g940 -std=c++0x g940.o $(LIBS)

clean:
	rm -f g940 g940.o
