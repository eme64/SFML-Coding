CPP=g++
CFLAGS= -lsfml-graphics -lsfml-window -lsfml-system

%.o: %.cpp
	$(CPP) -c $< 

main: main.o
	$(CPP) -o $@ $^ $(CFLAGS)
	
.PHONY: clean

clean:
	rm -f main.o main
