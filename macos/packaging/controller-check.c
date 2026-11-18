#include <SDL3/SDL.h>
#include <stdio.h>
int main(int argc,char **argv){
 SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI,"1");SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_PS5,"1");SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_XBOX,"1");SDL_SetHint(SDL_HINT_JOYSTICK_MFI,"1");
 if(!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)){printf("Controller check failed: %s\n",SDL_GetError());return 1;}
 if(argc>1 && SDL_AddGamepadMappingsFromFile(argv[1])<0)fprintf(stderr,"Using built-in controller mappings.\n");
 for(int i=0;i<20;i++){SDL_PumpEvents();SDL_Delay(50);}
 int count=0;SDL_JoystickID *pads=SDL_GetGamepads(&count);
 if(!count)puts("No gamepad detected. Connect your DualSense or Xbox controller by USB, or pair it in macOS Bluetooth settings, then check again.");
 int opened=0;
 for(int i=0;pads&&i<count;i++){
  SDL_Gamepad *pad=SDL_OpenGamepad(pads[i]);if(!pad)continue;opened++;
  SDL_JoystickConnectionState connection=SDL_GetGamepadConnectionState(pad);
  const char *transport=connection==SDL_JOYSTICK_CONNECTION_WIRED?"USB":connection==SDL_JOYSTICK_CONNECTION_WIRELESS?"wireless":"connected";
  printf("Detected: %s (%s)\n",SDL_GetGamepadName(pad),transport);
  SDL_CloseGamepad(pad);
 }
 SDL_free(pads);SDL_Quit();return count>0&&!opened?1:0;
}
