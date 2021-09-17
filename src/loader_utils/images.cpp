#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

unsigned char* loadImageLDR(const char* a_filename, int &w, int &h, int &channels)
{
  unsigned char* pixels = stbi_load(a_filename, &w, &h, &channels, STBI_rgb_alpha);

  if(w <= 0 || h <= 0 || !pixels)
  {
    return nullptr;
  }

  return pixels;
}

void freeImageMemLDR(unsigned char* pixels)
{
  stbi_image_free(pixels);
}