#ifndef VK_GRAPHICS_BASIC_IMAGES_H
#define VK_GRAPHICS_BASIC_IMAGES_H

unsigned char* loadImageLDR(const char* a_filename, int &w, int &h, int &channels);

void freeImageMemLDR(unsigned char* pixels);

#endif// VK_GRAPHICS_BASIC_IMAGES_H
