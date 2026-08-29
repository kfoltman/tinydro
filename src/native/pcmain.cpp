#include <algorithm>
#include <functional>
#include <assert.h>
#include <time.h>
#include <unistd.h>
#include <SDL/SDL.h>
#include "bmpfont.h"
#include "app.h"
#include <iostream>

App app;
SDL_Surface *pc_display;
int zoom = 1;

void Display::fill(int line, int xs, int xe, Pixel colour)
{
    for (int y = 0; y < zoom; ++y) {
        uint16_t *dst = (uint16_t *)(((uint8_t *)pc_display->pixels) + pc_display->pitch * (zoom * line + y));
        for (int x = xs * zoom; x < xe * zoom; ++x)
            dst[x] = colour;
    }
}

void Display::copy(int line, int xs, int xe, const Pixel *data)
{
    for (int y = 0; y < zoom; ++y) {
        uint16_t *dst = (uint16_t *)(((uint8_t *)pc_display->pixels) + pc_display->pitch * (zoom * line + y));
        // Don't care about efficiency here
        for (int x = xs * zoom; x < xe * zoom; ++x)
            dst[x] = data[x / zoom - xs];
    }
}

void RealTimeClock::init()
{
}

RealTimeClock::Time RealTimeClock::getTime()
{
    struct timespec ts;
    struct tm local;
    clock_gettime(CLOCK_REALTIME, &ts);
    localtime_r(&ts.tv_sec, &local);
    return Time(local.tm_year, local.tm_mon, local.tm_mday, local.tm_hour, local.tm_min, local.tm_sec, ts.tv_nsec / 1000000);
}

void stressTest()
{
    for (int t = 0; t < 10000; ++t) {
        for (int i = 0; i < 4; ++i) {
            app.eeprom.write32(i * 4, app.eeprom.read32(i * 4) + 0x01020304);
        }
        app.eeprom.flush();
    }
    int check[64];
    for (int i = 0; i < 64; ++i)
        check[i] = app.eeprom.read32(i * 4);
    app.eeprom.init();
    for (int i = 0; i < 64; ++i)
        assert(check[i] == app.eeprom.read32(i * 4));
}

Display display;

int main(int argc, char *argv[])
{
    if (SDL_Init(SDL_INIT_EVERYTHING) < 0)
    {
        printf("SDL Init failed\n");
        exit(1);
    }

    pc_display = SDL_SetVideoMode(zoom * 480, zoom * 320, 16, 0);
    bool quit = false;
    app.init();
    // stressTest();
    while(!app.touchscreen.quit) {
        if (!SDL_LockSurface(pc_display)) {
            app.render(display);
            SDL_UnlockSurface(pc_display);
        }
        SDL_Flip(pc_display);
        app.loop();
        usleep(10 * 1000);

        globalTime++;
    }
    SDL_Quit();
    return 0;
}
