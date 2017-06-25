all: sample2D

sample2D: piston.cpp glad.c
	g++ -o sample2D piston.cpp glad.c -lGL -lglfw -ldl -pthread -lao -lSOIL -lmpg123

clean:
	rm sample2D
