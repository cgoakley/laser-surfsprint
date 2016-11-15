/* This source file should be incorporated into a project that uses WindObj in the NT
   environment. It provides the entry point & calls the InitApplication() function that 
   needs to be supplied by the client to initialize the application ... */

#include <WindObj/WindObj.h>

#include <windows.h>

void * __cdecl clientMalloc(size_t size)
{
  return malloc(size);
}

void * __cdecl clientRealloc(void *ptr, size_t size)
{
  return realloc(ptr, size);
}

void __cdecl clientFree(void *ptr)
{
  free(ptr);
}

typedef void * (__cdecl *MallocFunc)(size_t size);
typedef void * (__cdecl *ReallocFunc)(void *ptr, size_t size);
typedef void (__cdecl *FreeFunc)(void *ptr);

WO_EXPORT void WindObjSetMemFuncs(MallocFunc clientMalloc,
                                              ReallocFunc clientRealloc,
                                              FreeFunc clientFree);

// Exported by WindObj ...

WO_EXPORT void InitAppParams(WindowVisibility &, BOOL &activated, int cmdshow,
                                         HINSTANCE hinst, HINSTANCE hinstprev);

WO_EXPORT int WINAPI WindObjMain(void);

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hInstPrev, char *cmdLine, int cmdShow)
{
  WindowVisibility wv;
  BOOL activated;
  InitAppParams(wv, activated, cmdShow, hInst, hInstPrev);
  WindObjSetMemFuncs(clientMalloc, clientRealloc, clientFree);
  InitApplication(cmdLine, wv, activated == TRUE);
  return WindObjMain();
}
