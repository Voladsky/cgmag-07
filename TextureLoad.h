#ifndef TEXTURE_LOAD_H
#define TEXTURE_LOAD_H
#include <glad/glad.h>
#include <stb_image.h>
#include <iostream>
GLuint loadTexture(const char* path);
GLuint createWhiteTexture();
GLuint createWhiteCircleTexture(int size = 256);
#endif