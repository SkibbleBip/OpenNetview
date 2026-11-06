
#include <SDL_ttf.h>
#include <string>


class TextLabel {

private:
    SDL_Texture *texture = nullptr;
    int width = 0;
    int height = 0;

public:

        TextLabel(void) : texture(nullptr), width(0), height(0) {}
    int setText(SDL_Renderer *renderer,
                 TTF_Font *font,
                 const std::string &text,
                 SDL_Color color);

    void draw(SDL_Renderer *renderer, int x, int y) const;

    void destroy();

    ~TextLabel(void) {
        destroy();
    }
};
