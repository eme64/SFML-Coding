CPP=g++ -std=c++11
CFLAGS= -lsfml-graphics -lsfml-window -lsfml-system -lsfml-audio

%.o: %.cpp
	$(CPP) -c $<

main: main.o
	$(CPP) -o $@ $^ $(CFLAGS)

.PHONY: clean

clean:
	rm -f main.o main

exec:
	./main
