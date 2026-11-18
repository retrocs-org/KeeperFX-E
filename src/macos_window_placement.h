#ifndef KFX_MACOS_WINDOW_PLACEMENT_H
#define KFX_MACOS_WINDOW_PLACEMENT_H
#include <SDL3/SDL_rect.h>
#include <algorithm>
// Fit the window onto the current monitor while keeping its aspect ratio.
inline SDL_Rect kfx_clamp_window(SDL_Rect window,SDL_Rect usable){
    if(usable.w<=0||usable.h<=0)return window;
    window.w=std::max(1,window.w);window.h=std::max(1,window.h);
    double scale=std::min(1.0,std::min(usable.w/(double)window.w,usable.h/(double)window.h));
    window.w=std::max(1,(int)(window.w*scale));window.h=std::max(1,(int)(window.h*scale));
    window.x=std::clamp(window.x,usable.x,usable.x+usable.w-window.w);window.y=std::clamp(window.y,usable.y,usable.y+usable.h-window.h);
    return window;
}
#endif
