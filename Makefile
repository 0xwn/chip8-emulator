all:
	g++ -I src/include -L src/lib -o debug main.cpp -lSDL2main -lSDL2
