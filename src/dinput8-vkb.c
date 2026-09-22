/* VKBDevCfg/Wine workaround: omit phantom HID padding buttons beyond 128.
 * Wraps a private copy of Wine's dinput8.dll; affects this application only.
 */
#define COBJMACROS
#define DIRECTINPUT_VERSION 0x0800
#include <windows.h>
#include <dinput.h>
#include <stddef.h>
#include <string.h>

static HMODULE real_module;
typedef HRESULT (WINAPI *Factory)(HINSTANCE,DWORD,REFIID,void**,IUnknown*);
static Factory real_factory;
typedef struct { void *v[32]; void **original; BOOL vkb; } DeviceTable;
typedef struct { void *v[13]; void **original; } InputTable;
typedef HRESULT (WINAPI *CapsFn)(void*,DIDEVCAPS*);
typedef HRESULT (WINAPI *EnumFn)(void*,LPDIENUMDEVICEOBJECTSCALLBACKW,void*,DWORD);
typedef HRESULT (WINAPI *InfoFn)(void*,DIDEVICEINSTANCEW*);
typedef ULONG (WINAPI *ReleaseFn)(void*);
typedef HRESULT (WINAPI *CreateFn)(void*,REFGUID,void**,IUnknown*);
static ULONG WINAPI device_release(void *self) {
    DeviceTable *t=*(DeviceTable**)self;
    ULONG refs=((ReleaseFn)t->original[2])(self);
    if(!refs) HeapFree(GetProcessHeap(),0,t);
    return refs;
}
static HRESULT WINAPI device_caps(void *self,DIDEVCAPS *caps) {
    DeviceTable *t=*(DeviceTable**)self;
    HRESULT hr=((CapsFn)t->original[3])(self,caps);
    if(SUCCEEDED(hr)&&t->vkb&&caps->dwButtons>128) caps->dwButtons=128;
    return hr;
}
typedef struct {LPDIENUMDEVICEOBJECTSCALLBACKW callback; void *context;} EnumContext;
static BOOL CALLBACK enum_filter(const DIDEVICEOBJECTINSTANCEW *obj,void *context) {
    EnumContext *e=context;
    if((obj->dwType&DIDFT_BUTTON)&&DIDFT_GETINSTANCE(obj->dwType)>=128) return DIENUM_CONTINUE;
    return e->callback(obj,e->context);
}
static HRESULT WINAPI device_enum(void *self,LPDIENUMDEVICEOBJECTSCALLBACKW cb,void *context,DWORD flags) {
    DeviceTable *t=*(DeviceTable**)self;
    if(!t->vkb)return ((EnumFn)t->original[4])(self,cb,context,flags);
    EnumContext e={cb,context};
    return ((EnumFn)t->original[4])(self,enum_filter,&e,flags);
}
static HRESULT WINAPI input_create(void *self,REFGUID guid,void **out,IUnknown *outer) {
    InputTable *input=*(InputTable**)self;
    HRESULT hr=((CreateFn)input->original[3])(self,guid,out,outer);
    if(FAILED(hr)||!out||!*out)return hr;
    DeviceTable *t=HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(*t));
    if(!t)return hr;
    t->original=*(void***)*out;
    memcpy(t->v,t->original,sizeof(t->v));
    /* A/W structures have identical GUID fields. Use a sufficiently large buffer. */
    DIDEVICEINSTANCEW info={0};info.dwSize=sizeof(info);
    HRESULT ih=((InfoFn)t->original[15])(*out,&info);
    /* GetDeviceInfo(A) expects the ANSI size when the caller requested ANSI. */
    if(FAILED(ih)){info.dwSize=sizeof(DIDEVICEINSTANCEA);ih=((InfoFn)t->original[15])(*out,&info);}
    t->vkb=SUCCEEDED(ih)&&LOWORD(info.guidProduct.Data1)==0x231d;
    t->v[2]=device_release;t->v[3]=device_caps;t->v[4]=device_enum;
    *(void***)*out=t->v;
    return hr;
}
static ULONG WINAPI input_release(void *self) {
    InputTable *t=*(InputTable**)self;
    ULONG refs=((ReleaseFn)t->original[2])(self);
    if(!refs)HeapFree(GetProcessHeap(),0,t);
    return refs;
}
HRESULT WINAPI DirectInput8Create(HINSTANCE instance,DWORD version,REFIID iid,void **out,IUnknown *outer) {
    if(!real_factory){
        real_module=LoadLibraryA("vkb-wine-dinput8.dll");
        if(!real_module)return HRESULT_FROM_WIN32(GetLastError());
        real_factory=(Factory)GetProcAddress(real_module,"DirectInput8Create");
        if(!real_factory)return E_FAIL;
    }
    HRESULT hr=real_factory(instance,version,iid,out,outer);
    if(FAILED(hr)||!out||!*out)return hr;
    InputTable *t=HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(*t));
    if(!t)return hr;
    t->original=*(void***)*out;memcpy(t->v,t->original,sizeof(t->v));
    t->v[2]=input_release;t->v[3]=input_create;*(void***)*out=t->v;
    return hr;
}
BOOL WINAPI DllMain(HINSTANCE h,DWORD reason,void *reserved){if(reason==DLL_PROCESS_ATTACH)DisableThreadLibraryCalls(h);return TRUE;}
