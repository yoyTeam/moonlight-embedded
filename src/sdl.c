/*
 * This file is part of Moonlight Embedded.
 *
 * Copyright (C) 2015 Iwan Timmer
 *
 * Moonlight is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * Moonlight is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Moonlight; if not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_SDL

#include "sdl.h"
#include "input/sdl.h"

#include "SDL_image.h"

#include <Limelight.h>

//Screen dimension constants
#define SCREEN_WIDTH 1080
#define SCREEN_HEIGHT 1920
#define CENTER_OFFSET 45

static bool done;
static int fullscreen_flags;

static SDL_Window *window;
static SDL_Renderer *renderer;
static SDL_Texture *bmp;

SDL_mutex *mutex;

int *movX;
int *movY;
SDL_Texture *vignetteTexture;

int sdlCurrentFrame, sdlNextFrame;

void sdl_init(int width, int height, bool fullscreen) {
  sdlCurrentFrame = sdlNextFrame = 0;

  if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
    fprintf(stderr, "Could not initialize SDL - %s\n", SDL_GetError());
    exit(1);
  }

  fullscreen_flags = fullscreen?SDL_WINDOW_FULLSCREEN:0;
  window = SDL_CreateWindow("Moonlight", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, SDL_WINDOW_OPENGL | fullscreen_flags);
  if(!window) {
    fprintf(stderr, "SDL: could not create window - exiting\n");
    exit(1);
  }

  SDL_SetRelativeMouseMode(SDL_TRUE);
  renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  if (!renderer) {
    printf("SDL_CreateRenderer failed: %s\n", SDL_GetError());
    exit(1);
  }

  bmp = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_YV12, SDL_TEXTUREACCESS_STREAMING, width, height);
  if (!bmp) {
    fprintf(stderr, "SDL: could not create texture - exiting\n");
    exit(1);
  }

  mutex = SDL_CreateMutex();
  if (!mutex) {
    fprintf(stderr, "Couldn't create mutex\n");
    exit(1);
  }

  SDL_Surface *vignetteImage = IMG_Load( "vignette.png" );
  vignetteTexture = SDL_CreateTextureFromSurface( renderer, vignetteImage );
}

void sdl_loop() {
  SDL_Event event;
  while(!done && SDL_WaitEvent(&event)) {
    switch (sdlinput_handle_event(&event)) {
    case SDL_QUIT_APPLICATION:
      done = true;
      break;
    case SDL_TOGGLE_FULLSCREEN:
      fullscreen_flags ^= SDL_WINDOW_FULLSCREEN;
      SDL_SetWindowFullscreen(window, fullscreen_flags);
    case SDL_MOUSE_GRAB:
      SDL_SetRelativeMouseMode(SDL_TRUE);
      break;
    case SDL_MOUSE_UNGRAB:
      SDL_SetRelativeMouseMode(SDL_FALSE);
      break;
    default:
      if (event.type == SDL_QUIT)
        done = true;
      else if (event.type == SDL_USEREVENT) {
        if (event.user.code == SDL_CODE_FRAME) {
          if (++sdlCurrentFrame <= sdlNextFrame - SDL_BUFFER_FRAMES) {
            //Skip frame
          } else if (SDL_LockMutex(mutex) == 0) {
              float scaleFactor = 2.0;

              int maxOffset = 250;

              int offsetX = *movX;
              offsetX *= scaleFactor;
              if( offsetX > maxOffset ) offsetX = maxOffset;
              else if( offsetX < -maxOffset ) offsetX = -maxOffset;

//Screen dimension constants
#define SCREEN_WIDTH 1080
#define SCREEN_HEIGHT 1920
#define CENTER_OFFSET 45
              int offsetY = *movY;
              offsetY *= -scaleFactor;
              if( offsetY > maxOffset ) offsetY = maxOffset;
              else if( offsetY < -maxOffset ) offsetY = -maxOffset;

            Uint8** data = ((Uint8**) event.user.data1);
            int* linesize = ((int*) event.user.data2);
            SDL_UpdateYUVTexture(bmp, NULL, data[0], linesize[0], data[1], linesize[1], data[2], linesize[2]);
            SDL_UnlockMutex(mutex);
            SDL_RenderClear(renderer);

            SDL_Rect rect = {offsetY+60, offsetX-60, SCREEN_HEIGHT*0.5, SCREEN_WIDTH}; // the rect is where you wants the texture to be drawn (screen coordinate).
                            //SDL_Rect crop = {AA, BB, CC, DD}; // the crop is what part of the image we want to display.

            float angle = 90.0f; // set the angle.
            //SDL_Point center = {0,0}; // the center where the texture will be rotated.
            SDL_RendererFlip flip = SDL_FLIP_NONE; // the flip of the texture.

            SDL_Rect topViewport;
            topViewport.x = 0;
            topViewport.y = CENTER_OFFSET;
            topViewport.w = SCREEN_WIDTH;
            topViewport.h = SCREEN_HEIGHT/2;
            SDL_RenderSetViewport( renderer, &topViewport );
            //gBGTexture.render( scrollingOffset, scrollingOffset, &clip, scrollingOffset*0.05, &center );
            SDL_RenderCopyEx(renderer, bmp, NULL , &rect, angle, NULL, flip);
            SDL_RenderCopy(renderer, vignetteTexture, NULL , NULL);


            SDL_Rect botViewport;
            botViewport.x = 0;
            botViewport.y = SCREEN_HEIGHT/2 + CENTER_OFFSET;
            botViewport.w = SCREEN_WIDTH;
            botViewport.h = SCREEN_HEIGHT/2;
            SDL_RenderSetViewport( renderer, &botViewport );
            //gBGTexture.render( scrollingOffset, scrollingOffset, &clip, scrollingOffset*0.05, &center );
            SDL_RenderCopyEx(renderer, bmp, NULL , &rect, angle, NULL, flip);
            SDL_RenderCopy(renderer, vignetteTexture, NULL , NULL);

            //SDL_RenderCopy(renderer, bmp, NULL, NULL);
            SDL_RenderPresent(renderer);
          } else
            fprintf(stderr, "Couldn't lock mutex\n");
        }
      }
    }
  }

  SDL_DestroyWindow(window);
  SDL_Quit();
}

#endif /* HAVE_SDL */
