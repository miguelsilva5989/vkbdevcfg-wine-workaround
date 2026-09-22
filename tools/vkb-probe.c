#define COBJMACROS
#define DIRECTINPUT_VERSION 0x0800
#include <windows.h>
#include <dinput.h>
#include <stdio.h>
static IDirectInput8A *di;
static BOOL CALLBACK objects(const DIDEVICEOBJECTINSTANCEA *o,void *ctx){if(o->dwType&DIDFT_BUTTON)printf("button instance=%lu offset=%lu usage=%04x:%04x name=%s\n",DIDFT_GETINSTANCE(o->dwType),o->dwOfs,o->wUsagePage,o->wUsage,o->tszName);return DIENUM_CONTINUE;}
static BOOL CALLBACK devices(const DIDEVICEINSTANCEA *i,void *ctx){IDirectInputDevice8A *d;DIDEVCAPS c={sizeof(c)};HRESULT hr=IDirectInput8_CreateDevice(di,&i->guidInstance,&d,NULL);printf("Device %s create=%08lx\n",i->tszProductName,hr);if(SUCCEEDED(hr)){hr=IDirectInputDevice8_GetCapabilities(d,&c);printf("Caps hr=%08lx axes=%lu buttons=%lu POVs=%lu\n",hr,c.dwAxes,c.dwButtons,c.dwPOVs);IDirectInputDevice8_EnumObjects(d,objects,NULL,DIDFT_BUTTON);IDirectInputDevice8_Release(d);}return DIENUM_CONTINUE;}
int main(){HRESULT hr=DirectInput8Create(GetModuleHandle(NULL),0x800,&IID_IDirectInput8A,(void**)&di,NULL);printf("Factory hr=%08lx\n",hr);if(FAILED(hr))return 1;IDirectInput8_EnumDevices(di,DI8DEVCLASS_GAMECTRL,devices,NULL,DIEDFL_ATTACHEDONLY);IDirectInput8_Release(di);return 0;}
