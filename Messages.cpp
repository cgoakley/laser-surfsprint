#define WINDOBJ_MAIN

#include <WindObj/Private.h>

#include <malloc.h>
#include <stdio.h>

_Screen Screen;

MessageProcessor::MessageProcessor() {}

MessageProcessor::MessageProcessor(HWND hWnd)
{
  m_hWnd = hWnd;
  m_next = g_MP;
  g_MP = this;
}

void MessageProcessor::UnhookMP()
{
  MessageProcessor *MP, *nextMP, *prevMP = NULL;
  
  for (MP = g_MP; MP; prevMP = MP, MP = nextMP)
  {
    nextMP = MP->m_next;
    if (MP != this) continue;
    if (prevMP) prevMP->m_next = nextMP;
    else g_MP = nextMP;
    break;
  }
}

MessageProcessor::~MessageProcessor()
{
  UnhookMP();
}

WorkspaceMP::WorkspaceMP(HWND hWnd) : MessageProcessor(hWnd) {}

BOOL WorkspaceMP::SpecialProcessing(MSG &msg)
{
  return TranslateMDISysAccel(m_hWnd, &msg);
}

DialogMP::DialogMP(HWND hWndDlg) : MessageProcessor(hWndDlg) {}

BOOL DialogMP::SpecialProcessing(MSG &msg)
{
  return IsDialogMessage(m_hWnd, &msg);
}

//AcceleratorMP *g_currentAMP = NULL;

AcceleratorMP::AcceleratorMP(HWND hWnd, ACCEL *acc, int nAccs)
{
  if (nAccs) m_hAccel = CreateAcceleratorTable(acc, nAccs);
  else m_hAccel = NULL;
//  if (m_replacedAMP = g_currentAMP) g_currentAMP->UnhookMP();
//  g_currentAMP = this;
  m_hWnd = hWnd;
  m_next = g_MP;
  g_MP = this;
}

static int EnumAccels(Menu *menu, ACCEL **acc);

AcceleratorMP::AcceleratorMP(HWND hWnd, Menu *menu)
{
  ACCEL *acc0, *acc;

  int n = EnumAccels(menu, NULL);
  acc0 = acc = (ACCEL *)_alloca(n * sizeof(ACCEL));
  EnumAccels(menu, &acc);
  this->AcceleratorMP::AcceleratorMP(hWnd, acc0, n);
}

static ACCEL ParseMenuAccelString(Menu *menu);

static int EnumAccels(Menu *menu, ACCEL **acc)
{
  ACCEL a;
  int n = (a = ParseMenuAccelString(menu)).cmd != 0;
  if (acc && n) {**acc = a; (*acc)++;}
  for (Menu *m = menu->Sub(); m; m = m->Next())
    n += EnumAccels(m, acc);
  return n;
}

static ACCEL ParseMenuAccelString(Menu *menu)
{
  ACCEL a = {0, 0, 0};
  int i;
  MenuData *md = menu->m_menuData;
  if (!(md->type & (MF_POPUP | MF_BITMAP | MF_OWNERDRAW | MF_SEPARATOR)) && 
    (i = strcspn(md->data, "\t\a") + 1) < (int)strlen(md->data))
  {
    a = ParseAccelString(md->data + i);
    a.cmd = md->id;
  }
  return a;
}

AcceleratorMP::~AcceleratorMP()
{
//  AcceleratorMP *amp, *prevAMP = NULL;

  DestroyAcceleratorTable(m_hAccel);
/*  for (amp = g_currentAMP; amp; prevAMP = amp, amp = amp->m_replacedAMP)
  {
    if (amp != this) continue;
    if (prevAMP) prevAMP->m_replacedAMP = amp->m_replacedAMP;
    else
    {
      if (m_replacedAMP) m_replacedAMP->m_next = g_MP;
      g_MP = g_currentAMP = m_replacedAMP;
    }
    break;
  } */
}

bool IsDescendedFrom(HWND hWnd, HWND hWndAncestor)
{
  return hWnd == hWndAncestor? true: hWnd == NULL? false: IsDescendedFrom(GetWindow(hWnd, GW_OWNER), hWndAncestor);
}

BOOL AcceleratorMP::SpecialProcessing(MSG &msg)
{
  return IsDescendedFrom(msg.hwnd, m_hWnd)? TranslateAccelerator(m_hWnd, m_hAccel, &msg): FALSE;
}

void AcceleratorMP::Modify(Window *w, char *keyStroke, KeyAction keyAction)
{
  WindowData *wd = w->m_windowData;
  Menu *menu = wd->menu;
  if (keyStroke)
  {
    if (!keyAction) // remove accelerator
    {
      for (int i = 0; i < wd->nSpecialKeys; i++)
      {
        if (_stricmp(keyStroke, wd->specialKey[i].keyStroke)) continue;
        delete [] wd->specialKey[i].keyStroke;
        memmove(wd->specialKey + i, wd->specialKey + i + 1, (--wd->nSpecialKeys - i) * sizeof(SpecialKey));
        break;
      }
      if (wd->nSpecialKeys == 0)
      {
        delete [] wd->specialKey;
        wd->specialKey = NULL;
      }
    }
    else // add accelerator
    {
      SpecialKey *newSks = new SpecialKey[wd->nSpecialKeys + 1];
      memcpy(newSks, wd->specialKey, wd->nSpecialKeys * sizeof(SpecialKey));
      delete [] wd->specialKey;
      wd->specialKey = newSks;
      newSks[wd->nSpecialKeys].keyAction = keyAction;
      strcpy(newSks[wd->nSpecialKeys++].keyStroke = new char[strlen(keyStroke) + 1], keyStroke);
    }
  }
  int nAccels = EnumAccels(menu, NULL) + wd->nSpecialKeys;
  ACCEL *accels = new ACCEL[nAccels], *pAccels = accels;
  if (m_hAccel) DestroyAcceleratorTable(m_hAccel);
  EnumAccels(menu, &pAccels);
  for (int i = 0; i < wd->nSpecialKeys; i++)
  {
    *pAccels = ParseAccelString(wd->specialKey[i].keyStroke);
    (pAccels++)->cmd = 40000 + i;
  }
  m_hAccel = CreateAcceleratorTable(accels, nAccels);
  delete [] accels;
}

HWND g_hWndNuu = NULL;

_Screen::_Screen()
{
  left = GetSystemMetrics(SM_XVIRTUALSCREEN);
  top = GetSystemMetrics(SM_YVIRTUALSCREEN);
  width = GetSystemMetrics(SM_CXVIRTUALSCREEN);
  height = GetSystemMetrics(SM_CYVIRTUALSCREEN);
  p_backgroundBrush = CreateSolidBrush(GetSysColor(COLOR_WINDOW));
}

_Screen::~_Screen() {DeleteObject(p_backgroundBrush);}

HINSTANCE g_hInst, g_hInstPrev;
MessageProcessor *g_MP = NULL;
BOOL g_oleInit = FALSE;
DWORD g_ddeInst = 0;

WO_EXPORT void InitAppParams(WindowVisibility &wv, BOOL &activated, int cmdShow, 
                                         HINSTANCE hInst, HINSTANCE hInstPrev)
{
  g_hInst = hInst;
  g_hInstPrev = hInstPrev;

  switch(cmdShow)
  {
  case SW_HIDE:            activated = FALSE; wv = WV_hidden;    break;

  case SW_SHOWMINNOACTIVE:
  case SW_MINIMIZE:        activated = FALSE; wv = WV_minimized; break;
  case SW_SHOWMINIMIZED:   activated = TRUE;  wv = WV_minimized; break;

  case SW_SHOWMAXIMIZED:   activated = TRUE;  wv = WV_maximized; break;

  case SW_SHOWNA:
  case SW_SHOWNOACTIVATE:  activated = FALSE; wv = WV_normal;    break;
  default:                 activated = TRUE;  wv = WV_normal;    break;
  }
}

WO_EXPORT int WINAPI WindObjMain()
{
  MSG msg;
  MessageProcessor *MP;

  while (GetMessage(&msg, NULL, 0, 0))
  {
    for (MP = g_MP; MP; MP = MP->m_next)
      if (MP->SpecialProcessing(msg)) goto ML1;
    TranslateMessage(&msg);
    DispatchMessage(&msg);
ML1:;
  }
  if (g_oleInit) OleUninitialize();
  if (g_ddeInst) DdeUninitialize(g_ddeInst);
  RestoreMemFuncs();  // restore mem allocators to DLL so that static objects are destroyed properly
  return msg.wParam;
}

void QuitApplication(int exitCode)
{
  if (g_floatingLabel) delete g_floatingLabel;
  PostQuitMessage(exitCode);
}

void ErrorMessageBox(const char *formatString, ...)
{
  va_list args;
  va_start(args, formatString);
  char errorText[256];
  _vsnprintf(errorText, sizeof(errorText), formatString, args);
  MessageBox(NULL, errorText, "", MB_OK | MB_ICONEXCLAMATION);
}

void StdErrorMessageBox()
{
  char *emsg = StdErrorString();
  if (emsg)
    MessageBox(NULL, emsg, "", MB_OK | MB_ICONEXCLAMATION);
}

// A representative subset of the available error codes ...

static char *s_stdErrStr[] = 
{
  "Invalid function",
  "The system cannot find the file specified",
  "The system cannot find the path specified",
  "The system cannot open the file",
  "Access is denied",
  "The handle is invalid",
  "The storage control blocks were destroyed",
  "Not enough storage is available to process this command",
  "The storage control block address is invalid",
  "The environment is incorrect",
  "An attempt was made to load a program with an incorrect format",
  "The access code is invalid",
  "The data is invalid",
  "Not enough storage is available to complete this operation",
  "The system cannot find the drive specified",
  "The directory cannot be removed",
  "The system cannot move the file to a different disk drive",
  "There are no more files",
  "The media is write protected",
  "The system cannot find the device specified",
  "The device is not ready",
  "The device does not recognize the command",
  "Data error (cyclic redundancy check)",
  "The program issued a command but the command length is incorrect",
  "The drive cannot locate a specific area or track on the disk",
  "The specified disk or diskette cannot be accessed",
  "The drive cannot find the sector requested",
  "The printer is out of paper",
  "The system cannot write to the specified device",
  "The system cannot read from the specified device",
  "A device attached to the system is not functioning",
  "The process cannot access the file because it is being used by another process",
  "The process cannot access the file because another process has locked a portion of the file",
  "The wrong diskette is in the drive",
  "Insert %2 (Volume Serial Number: %3) into drive %1",
  NULL,
  "Too many files opened for sharing",
  NULL,
  NULL,
  "Reached end of file",
  "The disk is full"
};

#define s_nStdErrStrs (sizeof(s_stdErrStr) / sizeof(char *))

char *StdErrorString()
{
  static char buf[40];

  WORD e = (WORD)GetLastError();
  if (!e) return NULL;
  if (e <= s_nStdErrStrs && s_stdErrStr[e - 1])
    return s_stdErrStr[e - 1];
  sprintf(buf, "System error %d", e);
  return buf;
}

bool CtrlKeyDown()
{
  return !!(GetKeyState(VK_CONTROL) & 0x8000);
}

WO_EXPORT bool IsKeyDown(VirtualKeyCode vkc)
{
  return !!(GetKeyState(vkc) & 0x8000);
}

Clipboard::Clipboard(bool &success)
{
  success = OpenClipboard(NULL) != 0;
}

void Clipboard::Clear()
{
  EmptyClipboard();
}

Clipboard::~Clipboard()
{
  CloseClipboard();
}

void Clipboard::operator=(const Bitmap *bm)
{
  if (bm->m_bitmapData->hPalette)
  {
    SetClipboardData(CF_DIB, PackedDIBFromBitmap(bm->m_bitmapData->hBitmap, bm->m_bitmapData->hPalette));
    SetClipboardData(CF_PALETTE, ClonePalette(bm->m_bitmapData->hPalette));
  }
  else SetClipboardData(CF_BITMAP, CloneBitmap(bm->m_bitmapData->hBitmap));
}

Bitmap *Clipboard::GetBitmap()
{
  HGLOBAL hDib;
  Bitmap *bm = new Bitmap;
  if (hDib = GetClipboardData(CF_DIB))
  {
    bm->m_bitmapData->hBitmap = BitmapFromPackedDIB(&bm->m_bitmapData->hPalette, hDib); 
  }
  else if (bm->m_bitmapData->hBitmap = (HBITMAP)GetClipboardData(CF_BITMAP))
  {
    bm->m_bitmapData->hBitmap = CloneBitmap(bm->m_bitmapData->hBitmap);
    bm->m_bitmapData->hPalette = NULL;
  }
  else
  {
    delete bm;
    return NULL;
  }
  return bm;
}

void Clipboard::operator=(const char *text)
{
  HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, text? strlen(text) + 1: 1);
  strcpy((char *)GlobalLock(hMem), text? text: "");
  GlobalUnlock(hMem);
  if (!SetClipboardData(CF_TEXT, hMem))
    throw WindObjError();
}

char *Clipboard::GetText()
{
  HGLOBAL hMem;
  if (!(hMem = GetClipboardData(CF_TEXT))) return NULL;
  char *ptr = (char *)GlobalLock(hMem);
  char *ptr1 = new char[ptr? strlen(ptr) + 1: 1];
  strcpy(ptr1, ptr);
  GlobalUnlock(hMem);
  return ptr1;
}

Point PointerPosition()
{
  Point pt;
  GetCursorPos((LPPOINT)&pt);
  return pt;
}

void PointerPosition(const Point &pt)
{
  SetCursorPos(pt.x, pt.y);
}
