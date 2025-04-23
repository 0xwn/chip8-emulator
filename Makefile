all:
	g++ -I src/include -L src/lib -o debug main.cpp -lmingw32 -lSDL2main -lSDL2