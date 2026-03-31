CFLAGS = -std=c++17 -O2 -I./include
LDFLAGS = -lglfw -lGL -ldl

OpenGLTest: main.cpp glad.c ParticleSystem.cpp TextureLoad.cpp
	g++ $(CFLAGS) -o OpenGLTest main.cpp glad.c ParticleSystem.cpp TextureLoad.cpp $(LDFLAGS)

.PHONY: test clean

test: OpenGLTest
	./OpenGLTest

clean:
	rm -f OpenGLTest