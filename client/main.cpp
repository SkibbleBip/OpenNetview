
#define SDL_MAIN_HANDLED

#include <SDL.h>

#include <cstring>
#include <iostream>
#include <string>
#include <vector>

#include "Common.h"
#include "TextBox.hpp"
#include "WebConnect.hpp"

const uint32_t MOVE_INTERVAL_MS = 25;
const int16_t MOVE_STEPS = 10;

const std::string fontPath =
#ifdef _WIN32
    "C:/Windows/Fonts/consola.ttf";
#else
    "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf";
#endif
/// @todo hardcode ttf into the binary

const SDL_Color c_white = {255, 255, 255, 255};
const SDL_Color c_green = {0, 255, 100, 255};
const SDL_Color c_red = {255, 0, 100, 255};
const SDL_Color c_grey = {180, 180, 180, 255};

std::string g_camera;

int main(int argc, char **argv) {

  if (argc < 2) {
    std::cout << "Usage: " << argv[0] << " [IP/Domain]" << std::endl;
    return 0;
  }

  g_camera = argv[1];

  SDL_SetMainReady();

  // Initialize SDL
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
    std::cerr << "SDL initialization failed: " << SDL_GetError() << std::endl;
    return 1;
  }

  if (TTF_Init() != 0) {
    std::cerr << "SDL_ttf initialization failed: " << TTF_GetError()
              << std::endl;
    SDL_Quit();
    return 1;
  }

  TTF_Font *font = TTF_OpenFont(fontPath.c_str(), 18);

  if (!font) {
    std::cerr << "Failed to load font: " << TTF_GetError() << std::endl;

    TTF_Quit();
    SDL_Quit();
    return 1;
  }

  // Create window
  SDL_Window *window =
      SDL_CreateWindow("OpenNetview Client", SDL_WINDOWPOS_CENTERED,
                       SDL_WINDOWPOS_CENTERED, 1280, 720, SDL_WINDOW_RESIZABLE);

  if (!window) {
    std::cerr << "Window creation failed: " << SDL_GetError() << std::endl;

    TTF_CloseFont(font);
    TTF_Quit();
    SDL_Quit();
    return 1;
  }

  // Create renderer
  SDL_Renderer *renderer = SDL_CreateRenderer(
      window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

  if (!renderer) {
    std::cerr << "Renderer creation failed: " << SDL_GetError() << std::endl;

    SDL_DestroyWindow(window);
    TTF_CloseFont(font);
    TTF_Quit();
    SDL_Quit();
    return 1;
  }

  SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

  // Static overlay labels
  TextLabel cameraInfo;
  TextLabel streamInfo;
  TextLabel controlsInfo;
  TextLabel statusInfo;
  TextLabel keyInfo;

  WebConnect web_connection(g_camera);
  int errorReturned = web_connection.getError();
  std::string connectionStatus;
  if (errorReturned != 0) {
    connectionStatus =
        "Connection Failure: " + std::string(strerror(errorReturned));
    std::cerr << connectionStatus << std::endl;
  } else {
    connectionStatus = "Connected";
  }

  cameraInfo.setText(renderer, font, "Camera: " + g_camera, c_white);

  streamInfo.setText(renderer, font,
                     ("RTP / H264: " + std::to_string(H264_PORT) +
                      " | Commands: " + std::to_string(COMMAND_PORT)),
                     c_white);

  controlsInfo.setText(renderer, font, "WASD / ▲ ▼◄ ► | ESC: Quit", c_white);

  statusInfo.setText(renderer, font, connectionStatus,
                     errorReturned != 0 ? c_red : c_green);

  keyInfo.setText(renderer, font, "[None]", c_grey);

  bool running = true;

  uint32_t lastTime = SDL_GetTicks();
  uint32_t keyDelay = 0;

  // Main event/render loop
  while (running) {

    SDL_Event event;

    // Process events
    while (SDL_PollEvent(&event)) {

      switch (event.type) {

      case SDL_QUIT:
        running = false;
        break;

      case SDL_KEYDOWN: {

        if (event.key.repeat != 0)
          break;
        // dont trigger on repeat

        SDL_Keycode key = event.key.keysym.sym;

        if (key == SDLK_ESCAPE) {
          running = false;
          break;
        }

        std::string keyName = SDL_GetKeyName(key);

        switch (event.key.keysym.scancode) {
        case SDL_SCANCODE_LEFT:
        case SDL_SCANCODE_RIGHT:
        case SDL_SCANCODE_UP:
        case SDL_SCANCODE_DOWN:
        case SDL_SCANCODE_A:
        case SDL_SCANCODE_S:
        case SDL_SCANCODE_D:
        case SDL_SCANCODE_W:
          keyInfo.setText(renderer, font, keyName, c_grey);
          break;
        case SDL_SCANCODE_HOME: {
          // custom for home key - sends "goto 0 0" command
          //  I think "reset" is broken because then it rejects any commands
          //  after
          keyInfo.setText(renderer, font, keyName, c_grey);
          web_connection.sendCommand(COMMAND_TYPE::HOME, 0, 0);
          break;
        }
        default:
          break;
        } // KEY SCANCODE

        break;
      }
      case SDL_KEYUP: {
        if (event.key.keysym.scancode != SDL_SCANCODE_HOME)
          // only clear if we didnt release on HOME
          keyInfo.setText(renderer, font, "[None]", c_grey);

        break;
      }
      } // switch

    } // while poll

    uint32_t currentTime = SDL_GetTicks();
    float deltaTime = (currentTime - lastTime) / 1000.0f;
    lastTime = currentTime;

    // 3. Poll Real-Time Keyboard State for Movement
    const uint8_t *keystate = SDL_GetKeyboardState(NULL);

    if (keyDelay < MOVE_INTERVAL_MS) {
      keyDelay += (deltaTime * 1000);
      // what on earth am I doing here
    } else {

      if (keystate[SDL_SCANCODE_LEFT] || keystate[SDL_SCANCODE_A]) {
        // std::cout << "left" << deltaTime << std::endl;
        (void)deltaTime;
        web_connection.sendCommand(COMMAND_TYPE::MOVE, MOVE_STEPS, 0);
      }
      if (keystate[SDL_SCANCODE_RIGHT] || keystate[SDL_SCANCODE_D]) {
        // std::cout << "right" << std::endl;
        (void)deltaTime;
        web_connection.sendCommand(COMMAND_TYPE::MOVE, -MOVE_STEPS, 0);
      }
      if (keystate[SDL_SCANCODE_UP] || keystate[SDL_SCANCODE_W]) {
        // std::cout << "up" << std::endl;
        (void)deltaTime;
        web_connection.sendCommand(COMMAND_TYPE::MOVE, 0, -MOVE_STEPS);
      }
      if (keystate[SDL_SCANCODE_DOWN] || keystate[SDL_SCANCODE_S]) {
        // std::cout << "down" << std::endl;
        (void)deltaTime;
        web_connection.sendCommand(COMMAND_TYPE::MOVE, 0, MOVE_STEPS);
      }
      keyDelay = 0;
    }

    // Get window dimensions
    int windowWidth;
    int windowHeight;

    SDL_GetWindowSize(window, &windowWidth, &windowHeight);

    // Clear window
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

    SDL_RenderClear(renderer);

    // ------------------------------------------------
    // Video area
    // ------------------------------------------------
    // Eventually replaced with FFmpeg video texture.

    SDL_SetRenderDrawColor(renderer, 15, 15, 15, 255);

    SDL_Rect videoArea = {0, 0, windowWidth, windowHeight};

    SDL_RenderFillRect(renderer, &videoArea);

    // ------------------------------------------------
    // Top overlay
    // ------------------------------------------------

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 160);

    SDL_Rect topOverlay = {0, 0, windowWidth, 75};

    SDL_RenderFillRect(renderer, &topOverlay);

    cameraInfo.draw(renderer, 15, 10);
    streamInfo.draw(renderer, 15, 38);

    // ------------------------------------------------
    // Bottom overlay
    // ------------------------------------------------

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 180);

    SDL_Rect bottomOverlay = {0, windowHeight - 75, windowWidth, 75};

    SDL_RenderFillRect(renderer, &bottomOverlay);

    controlsInfo.draw(renderer, 15, windowHeight - 65);
    statusInfo.draw(renderer, 15, windowHeight - 35);
    keyInfo.draw(renderer, windowWidth - 180, windowHeight - 35);

    // Display everything
    SDL_RenderPresent(renderer);
  } // while running

  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);

  TTF_CloseFont(font);
  TTF_Quit();

  SDL_Quit();

  return 0;
}
