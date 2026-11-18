#include <SDL3/SDL.h>
#include "bflib_joyst.h"
#include "bflib_inputctrl.h"
#include "front_input.h"
#include "bflib_mouse.h"
#include "bflib_keybrd.h"
#include "game_legacy.h"
#include "frontend.h"
#include "macos_controls.h"
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <math.h>
extern "C" {
void JEvent(const SDL_Event *);
struct Game game={};
int frontend_menu_state=FeSt_INITIAL;
unsigned char lbKeyOn[KC_LIST_END]={};
void poll_controller_mouse_clicks(void);
static int mouse_down_count,mouse_up_count;
TbControllerButtons get_game_key_controller_buttons(long key){return key==Gkey_LeftClick?CBtn_R2:key==Gkey_RightClick?CBtn_L2:0;}
void mouseControl(unsigned int action,struct TbPoint *point){(void)point;if(action==MActn_LBUTTONDOWN)mouse_down_count++;if(action==MActn_LBUTTONUP)mouse_up_count++;}
GameTurn get_gameturn(void){return 0;}
unsigned char last_used_input_device = ID_Keyboard_Mouse;
int LbSyncLog(const char *fmt,...){va_list a;va_start(a,fmt);int r=vfprintf(stderr,fmt,a);va_end(a);return r;}
int LbWarnLog(const char *fmt,...){(void)fmt;return 0;}
int LbErrorLog(const char *fmt,...){(void)fmt;return 0;}
char *prepare_file_path(short,const char *){return (char *)"/dev/null";}
}
int main(){
 SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS,"1");SDL_Init(SDL_INIT_VIDEO|SDL_INIT_GAMEPAD);init_controller_input();
 SDL_GamepadType type; (void)type;
 unsigned buttons=0,axes=0;unsigned long long mask=0;bool moved[6]={};Uint64 until=SDL_GetTicks()+20000;
 printf("Reading physical controller for 20 seconds...\n");fflush(stdout);
 while(SDL_GetTicks()<until){SDL_UpdateJoysticks();SDL_Event e;while(SDL_PollEvent(&e)){JEvent(&e);if(e.type==SDL_EVENT_GAMEPAD_BUTTON_DOWN&&controller_owns_event(e.gbutton.which)){buttons++;mask|=controller_button_state;}if(e.type==SDL_EVENT_GAMEPAD_AXIS_MOTION&&controller_owns_event(e.gaxis.which)&&abs(e.gaxis.value)>12000){axes++;if(e.gaxis.axis<6)moved[e.gaxis.axis]=true;}}SDL_Delay(10);}
 printf("Device: %s; model=%d; button presses=%u; significant axis events=%u; engine button mask=%llu\n",controller_device_name(),controller_device_model(),buttons,axes,mask);
 printf("Axes moved: LX=%d LY=%d RX=%d RY=%d LT=%d RT=%d\n",moved[0],moved[1],moved[2],moved[3],moved[4],moved[5]);SDL_Quit();return 0;}
