CPP=g++
CFLAGS= -std=gnu++11 -lsfml-graphics -lsfml-window -lsfml-system -lsfml-network

%.o: %.cpp
	$(CPP) -c -o $@ $< $(CFLAGS)

main: main.o
	$(CPP) -o $@ $^ -std=gnu++11 $(CFLAGS)

.PHONY: clean

clean:
	rm -f main.o main
