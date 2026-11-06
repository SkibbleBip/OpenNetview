
#include "TextBox.h"

int TextLabel::setText(SDL_Renderer *renderer, TTF_Font *font,
                       const std::string &text, SDL_Color color) {
  if (texture) {
    SDL_DestroyTexture(texture);
    texture = nullptr;
  }

  SDL_Surface *surface = TTF_RenderUTF8_Blended(font, text.c_str(), color);

  if (!surface) {
    //std::cerr << "Text rendering failed: " << TTF_GetError() << std::endl;
    return -1;
  }

  texture = SDL_CreateTextureFromSurface(renderer, surface);

  width = surface->w;
  height = surface->h;

  SDL_FreeSurface(surface);
  return 0;
}

void TextLabel::draw(SDL_Renderer *renderer, int x, int y) const {
  if (!texture)
    return;

  SDL_Rect destination = {x, y, width, height};

  SDL_RenderCopy(renderer, texture, nullptr, &destination);
}

void TextLabel::destroy() {
  if (texture) {
    SDL_DestroyTexture(texture);
    texture = nullptr;
  }
}
