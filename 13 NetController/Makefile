CPP=g++
CFLAGS= -g -std=gnu++11 -lsfml-graphics -lsfml-window -lsfml-system -lsfml-network -lpthread

%.o: %.cpp
	$(CPP) -c -o $@ $< $(CFLAGS)

main: main.o evp_server.o FastNoise/FastNoise.o evp_draw.o
	$(CPP) -o $@ $^ -std=gnu++11 $(CFLAGS)

.PHONY: clean

clean:
	rm -f main.o main evp_server.o evp_draw.o
