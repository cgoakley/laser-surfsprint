#define WINDOBJ_MAIN

#include <WindObj/Private.h>

#include <malloc.h>
#include <stdio.h>

static LRESULT CALLBACK WindowWndProc(HWND hWnd, UINT message,
    WPARAM wParam, LPARAM lParam);

Cursor g_arrowCursor(Cursor::arrow), g_waitCursor(Cursor::hourGlass);

bool g_isWaiting = false;

void SetWaitCursor(bool value)
{
  g_isWaiting = value;
  (value? &g_waitCursor: &g_arrowCursor)->Set();
}

static void WindowClassName(char *className, int classID)
{
  int i, n = GetModuleFileName(NULL, className, 256);
  for (i = n - 1; i > 0 && className[i] != '\\' && className[i] != ':'; i--)
  {
    if (className[i] == '.') className[n = i] = '\0';
  }
  memmove(className, className + i + 1, n - i);
  sprintf(className + n - i - 1, "WndClass%d", classID);
}

WindowClassData s_windowClassData = {0, 0, NULL, NULL};

Window::Window() : m_parent(NULL)
{
}

DWORD TranslateWindowStyle(int flags)
{
  DWORD dwStyle = WS_CAPTION;
  if (!(flags & WS_drawOnChildren)) dwStyle |=  WS_CLIPCHILDREN;
  if (flags & WS_sizeBorder) dwStyle |= WS_SIZEBOX;
  if (flags & (WS_sysMenu | WS_sysMenuWithExit)) dwStyle |= WS_SYSMENU;
  if (flags & WS_minButton) dwStyle |= WS_MINIMIZEBOX;
  if (flags & WS_maxButton) dwStyle |= WS_MAXIMIZEBOX;
  if (flags & WS_containedByParent) dwStyle |= WS_CHILD | WS_CLIPSIBLINGS;
  if ((flags & WS_displayMask) != WS_hidden) dwStyle |= WS_VISIBLE;
  return dwStyle;
}

// Window with title bar ...

void Window_Window(Window &thisWindow, Window *parent, const void *caption, bool captionIsWide, const Rect &rect,
  int flags, Menu *menu, const Icon *icon, Cursor *cursor)
{
  memset(thisWindow.m_windowData = new WindowData, 0, sizeof(WindowData));
  thisWindow.m_parent = parent;

  HMENU hMenu;
  if (thisWindow.m_windowData->menu = menu)
  {
    menu->Activate();
    hMenu = (HMENU)menu->m_menuData->id;
    MENUINFO menuInfo = {sizeof(MENUINFO), MIM_MENUDATA, 0, 0, NULL, 0, (DWORD)&thisWindow};
    SetMenuInfo(hMenu, &menuInfo);
  }
  else hMenu = NULL;

/* Within a Windows class it seems to be possible to customise everything for a particular window
   except for the icon, so the classes that we create by RegisterClass will therefore group 
   windows with the same icon. The case of no icon at all is covered by the first class, named
   "<AppName>WndClass0". Subsequent ones are named "<AppName>WndClass1", etc. --- the icon of a new 
   window being compared with the registered ones until a match is found, or, if there is none, a 
   new class is created. */

  int classID;
  WindowClassData *wcd = &s_windowClassData, *prevWcd = &s_windowClassData;
  if (icon)
  {
    classID = 1;
    for (wcd = s_windowClassData.nextWCD; wcd && wcd->icon != icon && *wcd->icon != *icon;
      prevWcd = wcd, wcd = wcd->nextWCD)
        if (classID == wcd->id) classID++;
  }
  else classID = 0;

  if (!wcd)
  {
    wcd = new WindowClassData;
    wcd->refs = 0;
    wcd->id = classID;
    wcd->icon = new Icon(*icon);
    wcd->nextWCD = NULL;
    prevWcd->nextWCD = wcd;
  }
  thisWindow.m_windowData->wcd = wcd;
  if (flags & WS_backingBitmap) thisWindow.m_windowData->screenCache = (HBITMAP)-1;

  char className[256];
  WindowClassName(className, classID);

  if (wcd->refs++ == 0) // Window class created on first use
  {
    WNDCLASSEX wndClassEx =
    {
      sizeof(WNDCLASSEX), CS_DBLCLKS, WindowWndProc, 0, 0, g_hInst, 
      (HICON)(icon? wcd->icon->m_iconData: NULL), (HCURSOR)g_arrowCursor.m_cursorData, (HBRUSH)NULL, 
      NULL, className, (HICON)NULL
    };
    wcd->atom = RegisterClassEx(&wndClassEx);
  }
  DWORD dwStyle = TranslateWindowStyle(flags) | WS_BORDER | WS_HSCROLL | WS_VSCROLL;
  if (flags & WS_displayMask) dwStyle &= ~WS_VISIBLE;
  int x, y, width, height;
  if (&rect)
  {
    if ((x = rect.left) == CW_USEDEFAULT) width = rect.right;
    else width = rect.right - rect.left;
    if ((y = rect.top) == CW_USEDEFAULT) height = rect.bottom;
    else height = rect.bottom - rect.top;
  }
  else x = y = width = height = CW_USEDEFAULT;
  HWND hWnd;
  if (captionIsWide)
  {
    wchar_t classNameW[256];
    size_t n = strlen(className);
    UINT nWide = MultiByteToWideChar(CP_UTF8, 0, className, (int)n, NULL, 0);
    MultiByteToWideChar(CP_UTF8, 0, className, (int)n, classNameW, nWide);
    hWnd = CreateWindowExW(flags & WS_topmost? WS_EX_TOPMOST: 0, classNameW, (const wchar_t *)caption, dwStyle, x, y, width, height,
      parent? parent->m_windowData->hWnd: NULL, hMenu, g_hInst, &thisWindow);
  }
  else
  {
    hWnd = CreateWindowExA(flags & WS_topmost? WS_EX_TOPMOST: 0, className, (const char *)caption, dwStyle, x, y, width, height,
      parent? parent->m_windowData->hWnd: NULL, hMenu, g_hInst, &thisWindow);
  }
  if (!hWnd) throw WindObjError();
  SetClassLong(hWnd, GCL_HCURSOR, (LONG)cursor->m_cursorData);
  ShowScrollBar(thisWindow.m_windowData->hWnd, SB_BOTH, FALSE);
  if (menu) thisWindow.ActivateSpecialKey(); // Activate accelerators, if any
  if (flags & WS_sysMenuWithExit)
  {
    HMENU hSysMenu = GetSystemMenu(thisWindow.m_windowData->hWnd, FALSE);
    AppendMenu(hSysMenu, MF_SEPARATOR, 0, NULL); 
    AppendMenu(hSysMenu, MF_STRING, 0x58, "&Exit"); 
  }
  int nCmdShow;
  switch (flags & WS_displayMask)
  {
  default:           nCmdShow = flags & WS_notActivated? SW_SHOWNOACTIVATE: SW_SHOWNORMAL; break;
  case WS_hidden:    nCmdShow = SW_HIDE; break;
  case WS_minimized: nCmdShow = flags & WS_notActivated? SW_SHOWMINNOACTIVE: SW_SHOWMINIMIZED; break;
  case WS_maximized: nCmdShow = SW_SHOWMAXIMIZED; break;
  }
  ShowWindow(thisWindow.m_windowData->hWnd, nCmdShow);
}

Window::Window(Window *parent, const char *caption, const Rect &rect,
  int flags, Menu *menu, const Icon *icon, Cursor *cursor)
{
  Window_Window(*this, parent, caption, false, rect, flags, menu, icon, cursor);
}

Window::Window(Window *parent, const wchar_t *caption, const Rect &rect,
  int flags, Menu *menu, const Icon *icon, Cursor *cursor)
{
  Window_Window(*this, parent, caption, true, rect, flags, menu, icon, cursor);
}

// Window with no title bar ...

Window::Window(Window *parent, const Rect &rect, int flags, Cursor *cursor)
{
  memset(m_windowData = new WindowData, 0, sizeof(WindowData));
  m_parent = parent;

/* Within a Windows class it seems to be possible to customise everything for a particular window
   except for the icon, so the classes that we create by RegisterClass will therefore group 
   windows with the same icon. In this case, there is icon at all so we use the window class
   "<AppName>WndClass0".  */

  m_windowData->wcd = &s_windowClassData;
  if (flags & WS_backingBitmap) m_windowData->screenCache = (HBITMAP)-1;

  char className[256];
  WindowClassName(className, 0);

  if (s_windowClassData.refs++ == 0) // Window class created on first use
  {
    WNDCLASSEX wndClassEx =
    {
      sizeof(WNDCLASSEX), CS_DBLCLKS, WindowWndProc, 0, 0, g_hInst, 
      (HICON)NULL, (HCURSOR)NULL, (HBRUSH)NULL, NULL, className, (HICON)NULL
    };
    s_windowClassData.atom = RegisterClassEx(&wndClassEx);
  }
  DWORD dwStyle = flags & WS_containedByParent && parent? WS_CHILD | WS_CLIPSIBLINGS: WS_POPUP;
  if (!(flags & WS_drawOnChildren)) dwStyle |= WS_CLIPCHILDREN;
  if (flags & WS_thinBorder) dwStyle |= WS_BORDER;
  int x = 0, y = 0, nWidth = 100, nHeight = 100;
  if (&rect) {x = rect.left; y = rect.top; nWidth = rect.right - rect.left; nHeight = rect.bottom - rect.top;}
  if (!CreateWindowEx((flags & WS_topmost? WS_EX_TOPMOST: 0) | (flags & WS_notInTaskbar? WS_EX_TOOLWINDOW: 0),
    className, NULL, dwStyle, x, y, nWidth, nHeight, parent? parent->m_windowData->hWnd: NULL, NULL, g_hInst, this)) 
    throw WindObjError();
  if (cursor)
  {
    DWORD ret = SetClassLong(m_windowData->hWnd, GCL_HCURSOR, (LONG)cursor->m_cursorData);
    ret = ret;
  }
  ShowWindow(m_windowData->hWnd, flags & WS_hidden || !&rect? SW_HIDE: SW_SHOWDEFAULT);
}

LRESULT CALLBACK WindowWndProc(HWND hWnd, UINT message,
    WPARAM wParam, LPARAM lParam)
{
  Window *w = (Window *)GetWindowLong(hWnd, GWL_USERDATA);
  WindowData *wd;
  LRESULT lRes;

  if (w && StdMessageProcessing(&lRes, w, hWnd, message, wParam, lParam)) return lRes;

/* WM_GETMINMAXINFO, WM_NCCREATE & WM_NCCALCSIZE are sent before this message, but do not
   need to be processed (yet) ... */
  if (!w && message == WM_CREATE)
  {
    CREATESTRUCT *cs = (LPCREATESTRUCT)lParam;
    w = (Window *)cs->lpCreateParams;
    wd = w->m_windowData;
    wd->hWnd = hWnd;
    SetWindowLong(wd->hWnd = hWnd, GWL_USERDATA, (LONG)w);
  }
  return DefWindowProc(hWnd, message, wParam, lParam);
}

/* Windows are either destroyed directly by a delete operator or indirectly when a parent is destroyed.
   In the latter case the window receives a WM_DESTROY message, where the USERDATA is set to NULL and this
   destructor called. Thus the destructor will only call DestroyWindow if the USERDATA is not NULL... */

Window::~Window()
{
  if (GetWindowLong(m_windowData->hWnd, GWL_USERDATA))
  {
    SetWindowLong(m_windowData->hWnd, GWL_USERDATA, NULL); // to prevent a feedback loop
// BCW claims that there is a resource leak unless this line is included ...
    if (m_windowData->menu) DestroyMenu((HMENU)m_windowData->menu->m_menuData->id);
    DestroyWindow(m_windowData->hWnd); // the WM_DESTROY triggered by this now goes to default processing only
  }
  if (m_windowData->menu)
  {
    delete (MenuTable *)m_windowData->menu->m_menuData->data; // stop menus being destroyed twice
    m_windowData->menu->m_menuData->data = NULL;
    delete m_windowData->menu;
  }
  delete m_windowData->accMP;
  if (m_windowData->palette) DeleteObject(m_windowData->palette);
  if (m_windowData->wcd && !(--m_windowData->wcd->refs))
  {
    UnregisterClass((LPCTSTR)m_windowData->wcd->atom, g_hInst);
    if (m_windowData->wcd->icon) // class must be unregistered before icon deleted
    {
      delete m_windowData->wcd->icon;
      WindowClassData *wcd = &s_windowClassData;
      for (; wcd->nextWCD != m_windowData->wcd; wcd = wcd->nextWCD);
      wcd->nextWCD = m_windowData->wcd->nextWCD;
      delete m_windowData->wcd;
    }
  }
  if (m_windowData->screenCache && m_windowData->screenCache != (HBITMAP)-1) 
    DeleteObject(m_windowData->screenCache);
  if (m_windowData->nSpecialKeys > 0)
  {
    for (int i = 0; i < m_windowData->nSpecialKeys; i++)
      delete [] m_windowData->specialKey[i].keyStroke;
    delete [] m_windowData->specialKey;
  }
	delete m_windowData;
  m_windowData = NULL;
  if (g_mouseWind == this) g_mouseWind = NULL;
}

void Window::ErrorMessageBox(const char *formatString, ...)
{
  va_list args;
  va_start(args, formatString);
  char errorText[256], windowTitle[80];
  _vsnprintf(errorText, sizeof(errorText), formatString, args);
  GetWindowText(m_windowData->hWnd, windowTitle, 80);
  windowTitle[sizeof(windowTitle) - 1] = '\0';
  MessageBox(m_windowData->hWnd, errorText, windowTitle, MB_OK | MB_ICONEXCLAMATION);
}

void Window::ErrorMessageBox(const wchar_t *formatString, ...)
{
  va_list args;
  va_start(args, formatString);
  wchar_t errorText[256], windowTitle[80];
  _vsnwprintf(errorText, sizeof(errorText) / sizeof(wchar_t), formatString, args);
  GetWindowTextW(m_windowData->hWnd, windowTitle, sizeof(windowTitle) / sizeof(wchar_t));
  windowTitle[sizeof(windowTitle) / sizeof(wchar_t) - 1] = '\0';
  MessageBoxW(m_windowData->hWnd, errorText, windowTitle, MB_OK | MB_ICONEXCLAMATION);
}

void Window::SetMenus(Menu *menuBar)
{
  if (m_windowData->menu) delete m_windowData->menu;
  (m_windowData->menu = menuBar)->Activate();
  HMENU hMenu = (HMENU)menuBar->m_menuData->id;
  MENUINFO menuInfo = {sizeof(MENUINFO), MIM_MENUDATA, 0, 0, NULL, 0, (DWORD)this};
  BOOL b = SetMenuInfo(hMenu, &menuInfo);
  SetMenu(m_windowData->hWnd, hMenu);
  ActivateSpecialKey(); // Activate accelerators, if any
}

struct OwnedWindEnum
{
  HWND hWnd;
  int n;
};

static BOOL __stdcall EnumThreadWP(HWND hWnd, LPARAM lParam)
{
  OwnedWindEnum *OWE = (OwnedWindEnum *)lParam;
  if (OWE->hWnd == GetWindow(hWnd, GW_OWNER) && GetWindowLong(hWnd, GWL_USERDATA))
  {
    if (OWE->n-- == 0) {OWE->hWnd = hWnd; return FALSE;}
  }
  return TRUE;
}

int Window::NumChildren() const
{
  int n = 0;
  for (HWND hChild = GetWindow(m_windowData->hWnd, GW_CHILD); hChild;
    hChild = GetWindow(hChild, GW_HWNDNEXT))
  {
    if (GetWindowLong(hChild, GWL_USERDATA)) n++;
  }
  OwnedWindEnum OWE = {m_windowData->hWnd, -1};
  EnumThreadWindows(GetCurrentThreadId(), (WNDENUMPROC)EnumThreadWP, (LPARAM)&OWE); 
  return n - OWE.n - 1;
}

Window *Window::Child(int index)
{
  Window *child = NULL;
  for (HWND hChild = GetWindow(m_windowData->hWnd, GW_CHILD); hChild;
    hChild = GetWindow(hChild, GW_HWNDNEXT))
  {
    child = (Window *)GetWindowLong(hChild, GWL_USERDATA);
    if (!child) continue;
    if (index-- == 0) return child;
  }
  OwnedWindEnum OWE = {m_windowData->hWnd, index};
  EnumThreadWindows(GetCurrentThreadId(), (WNDENUMPROC)EnumThreadWP, (LPARAM)&OWE); 
  if (OWE.hWnd != m_windowData->hWnd)
  {
    child = (Window *)GetWindowLong(OWE.hWnd, GWL_USERDATA);
  }
  return child;
}

void Window::Closed()
{
  delete this;
}

void Window::Exposed(DrawingSurface &DS, const Rect &invRect)
{
  DS.Rectangle(invRect, Pen(), Brush(Colour(Colour::window)));
}

void Window::SysColoursChanged()
{
  Draw();
}

void Window::AcceptDroppedFiles()
{
  DragAcceptFiles(m_windowData->hWnd, TRUE);
}

void Window::RejectDroppedFiles()
{
  DragAcceptFiles(m_windowData->hWnd, FALSE);
}

void Window::FileDropped(int number, const char *fileName) {}

void Window::FileDropped(int number, const wchar_t *fileName) {}

void Window::ActivateSpecialKey(char *keyStroke, KeyAction keyAction)
{
  if (!m_windowData->accMP) m_windowData->accMP = new AcceleratorMP(m_windowData->hWnd);
  m_windowData->accMP->Modify(this, keyStroke, keyAction);
}

void Window::SetPalette(const Bitmap *bm)
{
  if (m_windowData->palette) DeleteObject(m_windowData->palette);
  m_windowData->palette = NULL;
  if (bm)
  {
    m_windowData->palette = ClonePalette(bm->m_bitmapData->hPalette);
    SendMessage(m_windowData->hWnd, WM_QUERYNEWPALETTE, 0, 0);
  }
}

void Window::SetPalette()
{
  SetPalette(NULL);
}

int Window::GetText(char *text)
{
  int n = GetWindowTextLength(m_windowData->hWnd);
  if (text) GetWindowText(m_windowData->hWnd, text, n + 1);
  return n;
}

int Window::GetText(wchar_t *text)
{
  int n = GetWindowTextLengthW(m_windowData->hWnd);
  if (text) GetWindowTextW(m_windowData->hWnd, text, n + 1);
  return n;
}

float Window::GetFloatValue()
{
  int n = GetWindowTextLength(m_windowData->hWnd);
  char *buff = (char *)_alloca(n + 1);
  GetWindowText(m_windowData->hWnd, buff, n + 1);
  return (float)atof(buff);
}

int Window::GetIntValue()
{
  int n = GetWindowTextLength(m_windowData->hWnd);
  char *buff = (char *)_alloca(n + 1);
  GetWindowText(m_windowData->hWnd, buff, n + 1);
  return atoi(buff);
}

void Window::SetText(const char *text)
{
  HWND hWndNuu = g_hWndNuu;
  g_hWndNuu = m_windowData->hWnd;
  SetWindowText(g_hWndNuu, text? text: "");
  g_hWndNuu = hWndNuu;
}

void Window::SetText(const wchar_t *text)
{
  HWND hWndNuu = g_hWndNuu;
  g_hWndNuu = m_windowData->hWnd;
  SetWindowTextW(g_hWndNuu, text? text: L"");
  g_hWndNuu = hWndNuu;
}

void Window::SetText(const char *text, int nText)
{
  char *buff = (char *)_alloca(nText + 1);
  if (nText) memcpy(buff, text, nText);
  buff[nText] = '\0';
  SetText(buff);
}

void Window::SetText(const wchar_t *text, int nText)
{
  wchar_t *buff = (wchar_t *)_alloca((nText + 1) * sizeof(wchar_t));
  if (nText) memcpy(buff, text, nText * sizeof(wchar_t));
  buff[nText] = L'\0';
  SetText(buff);
}

void Window::SetText(double value)
{
  char buff[25];
  sprintf(buff, "%lg", value);
  SetText(buff);
}

void Window::SetText(float value)
{
  char buff[20];
  sprintf(buff, "%g", value);
  SetText(buff);
}

void Window::SetText(int value)
{
  char buff[15];
  sprintf(buff, "%d", value);
  SetText(buff);
}

void Window::Size(int width, int height)
{
  SetWindowSize(m_windowData->hWnd, width, height);
}

void Window::SetCanvas(int canvasW, int canvasH)
{
  SetCanvas(0, 0, canvasW, canvasH);
}

void Window::SetCanvas(int canvasX, int canvasY, int canvasW, int canvasH)
{
  m_windowData->org.x = canvasX;
  m_windowData->org.y = canvasY;
  m_windowData->canvasW = canvasW;
  m_windowData->canvasH = canvasH;
  SetCanvas();
}

void Window::SetCanvasToChildren()
{
  int n = NumChildren();
  if (n < 1) return;
  Rect r = Child(0)->BoundingRect();
  for (int i = 1; i < n; i++) r |= Child(i)->BoundingRect();
  SetCanvas(r.right - r.left, r.bottom - r.top);
}

static void MoveChildren(Window *w, const Point &offset)
{
  int n = w->NumChildren();
  for (int i = 0; i < n; i++)
  {
    Window *child = w->Child(i);
    if (GetWindowLong(child->m_windowData->hWnd, GWL_STYLE) & WS_CHILD) // Do not move child overlapped windows
    {
      Point p = child->BoundingRect().TopLeft() - w->ClientOrigin() + offset;
      child->Move(p.x, p.y);
    }
  }
}

void Window::SetCanvasOrg(const Point &newOrigin, bool isRelative)
{
// NB: newOrigin can be out of range: the SetScrollInfo call below will adjust as required
  Point oldOrg = m_windowData->org, shift, newOrg = newOrigin;
  HWND hWndNuu = g_hWndNuu;
  g_hWndNuu = m_windowData->hWnd; // To avoid processing the messages generated by the scroll functions
  if (isRelative) newOrg += m_windowData->org;
  m_windowData->org = newOrg;
  shift = newOrg - oldOrg;
//  MoveChildren(this, -shift);
  SCROLLINFO si = {sizeof(SCROLLINFO), SIF_POS, 0, 0, 0, m_windowData->org.x};
  if (oldOrg.x != m_windowData->org.x)
    m_windowData->org.x = SetScrollInfo(m_windowData->hWnd, SB_HORZ, &si, TRUE);
  if (oldOrg.y != m_windowData->org.y)
  {
    si.nPos = m_windowData->org.y;
    m_windowData->org.y = SetScrollInfo(m_windowData->hWnd, SB_VERT, &si, TRUE);
  }
  g_hWndNuu = hWndNuu;
  ScrollWindowEx(m_windowData->hWnd, oldOrg.x - m_windowData->org.x,
    oldOrg.y - m_windowData->org.y, NULL, NULL, NULL, NULL, SW_INVALIDATE | SW_SCROLLCHILDREN);
}

Rect Window::Canvas() const
{
  return Rect(m_windowData->org.x, m_windowData->org.y,
    m_windowData->org.x + m_windowData->canvasW,
    m_windowData->org.y + m_windowData->canvasH);
}

static bool SetBitmapSize(HBITMAP &hBM, int width, int height)
{
  if (!hBM) return false;
  if (hBM != (HBITMAP)-1)
  {
    BITMAP bm;
    GetObject(hBM, sizeof(BITMAP), &bm);
    if (width == bm.bmWidth && height == bm.bmHeight) return false;
    DeleteObject(hBM);
  }
  HDC hDCscreen = CreateIC("DISPLAY", NULL, NULL, NULL);
  hBM = CreateCompatibleBitmap(hDCscreen, width, height);
  DeleteDC(hDCscreen);
  return true;
}

bool Window::SetCanvas()
{
  RECT rect;
  Point oldOrg = m_windowData->org;
  HWND hWndNuu = g_hWndNuu;
  g_hWndNuu = m_windowData->hWnd;
  ShowScrollBar(m_windowData->hWnd, SB_BOTH, FALSE);
  GetClientRect(m_windowData->hWnd, &rect);
  BOOL showHorizSB, showVertSB;
  int scrollW = GetSystemMetrics(SM_CXVSCROLL), scrollH = GetSystemMetrics(SM_CXHSCROLL);
  if (showVertSB = m_windowData->canvasH > rect.bottom)
  {
    showHorizSB = m_windowData->canvasW > rect.right - scrollW;
  }
  else
  {
    if (showHorizSB = m_windowData->canvasW > rect.right)
      showVertSB = m_windowData->canvasH > rect.bottom - scrollH;
  }
  if (showHorizSB)
  {
    SCROLLINFO si = {sizeof(SCROLLINFO), SIF_ALL, 0, m_windowData->canvasW - 1,
      rect.right - (showVertSB? scrollW: 0), m_windowData->org.x};
    m_windowData->org.x = SetScrollInfo(m_windowData->hWnd, SB_HORZ, &si, TRUE);
    ShowScrollBar(m_windowData->hWnd, SB_HORZ, TRUE);
  }
  else m_windowData->org.x = 0;
  if (showVertSB)
  {
    SCROLLINFO si = {sizeof(SCROLLINFO), SIF_ALL, 0, m_windowData->canvasH - 1,
      rect.bottom - (showHorizSB? scrollH: 0), m_windowData->org.y};
    m_windowData->org.y = SetScrollInfo(m_windowData->hWnd, SB_VERT, &si, TRUE);
    ShowScrollBar(m_windowData->hWnd, SB_VERT, TRUE);
  }
  else m_windowData->org.y = 0;
  if (SetBitmapSize(m_windowData->screenCache, 
      m_windowData->canvasW? m_windowData->canvasW: rect.right,
      m_windowData->canvasH? m_windowData->canvasH: rect.bottom))
  {
    DrawingSurface DS(*this);
    Exposed(DS, ClientRect());
  }
  g_hWndNuu = hWndNuu;
  return oldOrg != m_windowData->org;
}

void Window::Size(Rect rect)
{
  MoveWindow(m_windowData->hWnd, rect.left, rect.top, 
    rect.right - rect.left, rect.bottom - rect.top, TRUE);
}

void Window::SetClientWidth(int width)
{
  SetWindowClientWidth(m_windowData->hWnd, width);
}

void Window::Disable()
{
  EnableWindow(m_windowData->hWnd, FALSE);
}

void Window::Draw()
{
  InvalidateRect(m_windowData->hWnd, NULL, FALSE);
  UpdateWindow(m_windowData->hWnd);
}

void Window::Draw(const Rect &rect)
{
  Rect rect1 = rect - m_windowData->org;
  InvalidateRect(m_windowData->hWnd, (CONST RECT *)&rect1, FALSE);
  UpdateWindow(m_windowData->hWnd);
}

void Window::Enable(bool state)
{
  EnableWindow(m_windowData->hWnd, state);
}

bool Window::IsEnabled()
{
  return IsWindowEnabled(m_windowData->hWnd) == TRUE;
}

void Window::Block(bool state)
{
  m_windowData->blocked = state;
}

void Window::Unblock() {Block(false);}

bool Window::IsBlocked() {return m_windowData->blocked;}

void Window::Hide()
{
  HWND hWndNuu = g_hWndNuu;
  g_hWndNuu = m_windowData->hWnd;
  ShowWindow(g_hWndNuu, SW_HIDE);
  g_hWndNuu = hWndNuu;
}

void Window::Show()
{
  HWND hWndNuu = g_hWndNuu;
  g_hWndNuu = m_windowData->hWnd;
  ShowWindow(g_hWndNuu, SW_SHOW);
  g_hWndNuu = hWndNuu;
}

void Window::Show(bool state)
{
  HWND hWndNuu = g_hWndNuu;
  g_hWndNuu = m_windowData->hWnd;
  ShowWindow(g_hWndNuu, state? SW_SHOW: SW_HIDE);
  g_hWndNuu = hWndNuu;
}

void Window::ShowWithoutActivating()
{
  HWND hWndNuu = g_hWndNuu;
  g_hWndNuu = m_windowData->hWnd;
  ShowWindow(g_hWndNuu, SW_SHOWNOACTIVATE);
  g_hWndNuu = hWndNuu;
}

bool Window::IsShown()
{
  return IsWindowVisible(m_windowData->hWnd) == TRUE;
}

void Window::Minimize()
{
  HWND hWndNuu = g_hWndNuu;
  g_hWndNuu = m_windowData->hWnd;
  ShowWindow(g_hWndNuu, SW_MINIMIZE);
  g_hWndNuu = hWndNuu;
}

bool Window::IsMinimized()
{
  WINDOWPLACEMENT WP;
  GetWindowPlacement(m_windowData->hWnd, &WP);
  return WP.showCmd == SW_MINIMIZE || 
    WP.showCmd == SW_SHOWMINIMIZED ||
    WP.showCmd == SW_FORCEMINIMIZE ||
    WP.showCmd == SW_SHOWMINNOACTIVE;
}

void Window::Maximize()
{
  HWND hWndNuu = g_hWndNuu;
  g_hWndNuu = m_windowData->hWnd;
  ShowWindow(g_hWndNuu, SW_MAXIMIZE);
  g_hWndNuu = hWndNuu;
}

bool Window::IsMaximized()
{
  WINDOWPLACEMENT WP;
  GetWindowPlacement(m_windowData->hWnd, &WP);
  return WP.showCmd == SW_MAXIMIZE || 
    WP.showCmd == SW_SHOWMAXIMIZED;
}

void Window::SizeNormal()
{
  HWND hWndNuu = g_hWndNuu;
  g_hWndNuu = m_windowData->hWnd;
  ShowWindow(g_hWndNuu, SW_RESTORE);
  g_hWndNuu = hWndNuu;
}

void Window::BringToTop()
{
  BringWindowToTop(m_windowData->hWnd);
}

void Window::SetFocus()
{
  HWND hWndNuu = g_hWndNuu;
  g_hWndNuu = m_windowData->hWnd;
  ::SetFocus(g_hWndNuu);
  g_hWndNuu = hWndNuu;
}

bool Window::IsFocal() const
{
  return m_windowData->hWnd == ::GetFocus();
}

void Window::SetMouseCapture() {SetCapture(m_windowData->hWnd);}
void Window::ReleaseMouseCapture() {ReleaseCapture();}
bool Window::IsMouseCaptured() const {return GetCapture() == m_windowData->hWnd;}

Rect Window::BoundingRect() const
{
  RECT rect;
  if (!GetWindowRect(m_windowData->hWnd, &rect)) throw "Failed to get window co-ordinates";
  return Rect(rect.left, rect.top, rect.right, rect.bottom);
}

void Window::BoundingRect(const Rect &rect)
{
  Rect rect1 = rect;
  if (GetWindowLong(m_windowData->hWnd, GWL_STYLE) & WS_CHILD)
    MapWindowPoints(NULL, GetParent(m_windowData->hWnd), (LPPOINT)&rect1, 2);
  if (!MoveWindow(m_windowData->hWnd, rect1.left, rect1.top, rect1.right - rect1.left, rect1.bottom - rect1.top, FALSE))
    throw "MoveWindow call failed";
}

Rect Window::NormalBoundingRect() const
{
  WINDOWPLACEMENT WP;
  GetWindowPlacement(m_windowData->hWnd, &WP);
  return Rect(WP.rcNormalPosition.left, WP.rcNormalPosition.top, WP.rcNormalPosition.right, WP.rcNormalPosition.bottom);
}

void Window::NormalBoundingRect(const Rect &rect)
{
  WINDOWPLACEMENT WP;
  GetWindowPlacement(m_windowData->hWnd, &WP);
  WP.rcNormalPosition.left = rect.left;
  WP.rcNormalPosition.top = rect.top;
  WP.rcNormalPosition.right = rect.right;
  WP.rcNormalPosition.bottom = rect.bottom;
  if (!SetWindowPlacement(m_windowData->hWnd, &WP)) throw "Failed to set bounding rectangle of window";
}

Rect Window::ClientRect() const
{
  Rect rect;
  if (!GetClientRect(m_windowData->hWnd, (RECT *)&rect)) throw "Failed to get window client rectangle";
  return rect + m_windowData->org;
}

void Window::ClientRect(const Rect &rect)
{
  Rect crect = ClientRect();
  HWND hWnd = m_windowData->hWnd;
  RECT brect;
  GetWindowRect1(hWnd, &brect);
  MoveWindow(hWnd, brect.left, brect.top, 
    rect.right - rect.left + brect.right - brect.left - (crect.right - crect.left), 
    rect.bottom - rect.top + brect.bottom - brect.top - (crect.bottom - crect.top), TRUE);
}

Point Window::ClientOrigin() const
{
  Point pt;
  MapWindowPoints(m_windowData->hWnd, NULL, (LPPOINT)&pt, 1);
  return pt;
}

void Window::Move(int x, int y)
{
  SetWindowPos(m_windowData->hWnd, x, y);
}

void Window::KeyPressed(VirtualKeyCode vkcode, char ascii, unsigned short repeat) {}

void Window::KeyReleased(VirtualKeyCode vkcode) {}

void Window::MouseLButtonPressed(int x, int y, int flags) {}

void Window::MouseLButtonReleased(int x, int y, int flags) {}

void Window::MouseLButtonDblClicked(int x, int y, int flags) {}

void Window::MouseRButtonPressed(int x, int y, int flags) {}

void Window::MouseRButtonReleased(int x, int y, int flags) {}

void Window::MouseRButtonDblClicked(int x, int y, int flags) {}

void Window::MouseMoved(int x, int y) {}

void Window::SetCursor(Cursor *&cursor) {}

void Window::MouseLeft() {}

void Window::Moved() {}

void Window::Sized() {}
void Window::Minimized() {}

void Window::Exited() {}

void Window::UserMessage(unsigned int msgId, unsigned int wParam, long lParam) {}

void Window::PostUserMessage(unsigned int msgId, unsigned int wParam, long lParam)
{
  if (!PostMessage(m_windowData->hWnd, msgId, wParam, lParam))
    throw WindObjError();
}

void Window::LostFocus() {}
void Window::GotFocus() {}

void Window::OtherMessage(unsigned int,unsigned int,long) {}

/* This message processing is common to all classes derived from <Window> & provides the link
   from the WM messages to the invocation of the methods that process them. If FALSE is returned
   then either the message was not processed or it may be subject to further processing & so should 
   be passed to the relevant default processing ... */
HBRUSH s_hBr;

BOOL StdMessageProcessing(LRESULT *lRes, Window *w, HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  WindowData *wd = w->m_windowData;
  int i;
  static char buff[256];
  LRESULT lRes1;
  DrawingSurface *DS;
  PAINTSTRUCT ps;
  Rect invRect;
  DSData *dsd;
  HDC hDC;
  Window *ctrl;
  if (lRes) *lRes = 0;
  
  if (wd->blocked) // if input is blocked, then suppress processing of user input messages
  {
    switch (message)
    {
    case WM_KEYDOWN:
    case WM_KEYUP:
    case WM_CHAR:
    case WM_MOUSEMOVE:
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_LBUTTONDBLCLK:
      return TRUE;
    }
  }

  if (ProcessMenuMessage(*lRes, wd, message, wParam, lParam))
    return TRUE;

  switch (message)
  {
  case WM_CHAR:
    w->KeyPressed((VirtualKeyCode)0, (char)wParam, (unsigned short)lParam);
    break;

//  case WM_CHILDACTIVATE:
//    SendMessage(hWnd, WM_NCACTIVATE, TRUE, 0);
//    break;

  case WM_CLOSE:
    w->Closed();
    return TRUE;

  case WM_COMMAND:
    if (lParam)
    {
      Window *child = (Window *)GetWindowLong((HWND)lParam, GWL_USERDATA);
      if (child && child->m_windowData->hWnd != g_hWndNuu) // block messages not generated by the user
      {
        switch (HIWORD(wParam))
        {
        case BN_CLICKED: ((Button *)child)->Clicked(); return TRUE;
        case CBN_EDITCHANGE: ((ComboBox *)child)->TextChanged(); return TRUE;
        case CBN_SELCHANGE: ((ComboBox *)child)->ListIndexChanged(); return TRUE;
        case EN_CHANGE: ((EditBox *)child)->TextChanged(); return TRUE;
        }
      }
    }
    else
    {
      if (wParam == 1 || wParam == 2) // Return or Cancel with no Default or Cancel button
      {
        HWND hWndChild = GetFocus();
        if (hWnd != GetParent(hWndChild))
        {
          if (GetParent(hWndChild = GetParent(hWndChild)) != hWnd) break; // combo box edit box is child of combo
        }
        if (ctrl = (Window *)GetWindowLong(hWndChild, GWL_USERDATA))
        {
          if (wParam == 1) ctrl->KeyPressed(VKC_return, VKC_return, 1);
          else ctrl->KeyPressed(VKC_escape, VKC_escape, 1);
        }
        break;
      }
      if (wParam >= 105536 && wParam < 105536 + (unsigned int)wd->nSpecialKeys)
      {
        (*wd->specialKey[wParam - 105536].keyAction)(w);
        return TRUE;
      }
    }
    break;

  case WM_SYSCOMMAND:
    if (wParam == 0x58)
    {
      w->Exited();
      return TRUE;
    }
    break;

  case WM_CTLCOLOREDIT:
    if ((ctrl = (Window *)GetWindowLong((HWND)lParam, GWL_USERDATA)) ||
        (ctrl = (Window *)GetWindowLong(GetParent((HWND)lParam), GWL_USERDATA)))
    {
      ControlData *cd = (ControlData *)ctrl->m_windowData;
      SetBkMode((HDC)wParam, TRANSPARENT);
      SetTextColor((HDC)wParam, cd->foreColour);
      LOGBRUSH logBrush;
      GetObject(cd->backBrush, sizeof(LOGBRUSH), &logBrush);
      SetBkColor((HDC)wParam, logBrush.lbColor);
      if (lRes) *lRes = (LRESULT)cd->backBrush;
      return TRUE;
    }
    break;

  case WM_CTLCOLORBTN:
  case WM_CTLCOLORDLG:
  case WM_CTLCOLORLISTBOX:
  case WM_CTLCOLORMSGBOX:
  case WM_CTLCOLORSCROLLBAR:
  case WM_CTLCOLORSTATIC:
    if ((ctrl = (Window *)GetWindowLong((HWND)lParam, GWL_USERDATA)) ||
        (ctrl = (Window *)GetWindowLong(GetParent((HWND)lParam), GWL_USERDATA)))
    {
      ControlData *cd = (ControlData *)ctrl->m_windowData;
      SetBkMode((HDC)wParam, TRANSPARENT);
      SetTextColor((HDC)wParam, cd->foreColour);
      if (lRes) *lRes = (LRESULT)cd->backBrush;
      return TRUE;
    }
    break;

  case WM_DESTROY:
    SetWindowLong(hWnd, GWL_USERDATA, NULL);
    {
      HWND hWndChild;
      hWndChild = GetWindow(hWnd, GW_CHILD);
      while (hWndChild)
      {
        hWndChild = GetWindow(hWndChild, GW_HWNDNEXT);
      }
    }
    delete w;
    break;

  case WM_DROPFILES:
    for (i = DragQueryFile((HDROP)wParam, (UINT)-1, NULL, 0); i > 0; i--)
    {
      DragQueryFile((HDROP)wParam, i - 1, buff, 256);
      w->FileDropped(i, buff);
      wchar_t wbuff[256];
      DragQueryFileW((HDROP)wParam, i - 1, wbuff, sizeof(wbuff) / sizeof(wchar_t));
      w->FileDropped(i, wbuff);
    }
    break;

  case WM_HSCROLL:
    if (!wd->canvasW) break;
    {
      int orgX = wd->org.x;
      GetClientRect(hWnd, (RECT *)&invRect);
      switch (LOWORD(wParam))
      {
      case        SB_BOTTOM: orgX = wd->canvasW - invRect; break;
      case      SB_LINELEFT: orgX -= 10; break;
      case     SB_LINERIGHT: orgX += 10; break;
      case      SB_PAGELEFT: orgX -= invRect.right; break;
      case     SB_PAGERIGHT: orgX += invRect.right; break;
      case           SB_TOP: orgX = 0; break;
      case SB_THUMBPOSITION:
      case SB_THUMBTRACK:    orgX = HIWORD(wParam); break;
      default: return FALSE;
      }
      w->SetCanvasOrg(Point(orgX, wd->org.y), false);
    }
    break;

  case WM_KEYDOWN:
    w->KeyPressed((VirtualKeyCode)wParam, 0, (unsigned short)lParam);
    break;

  case WM_KEYUP:
    w->KeyReleased((VirtualKeyCode)wParam);
    break;

  case WM_KILLFOCUS:
    {
/* Deactivate the title bar unless this window is an ancestor of the one receiving the focus ...
      for (HWND lostTo = (HWND)wParam; lostTo = GetParent(lostTo);)
        if (!lostTo || lostTo == hWnd) goto KFND;
      SendMessage(hWnd, WM_NCACTIVATE, 0, 0);
KFND: */
      if (!g_hWndNuu) w->LostFocus();
    }
    break;

  case WM_LBUTTONDBLCLK:
    w->MouseLButtonDblClicked((short)LOWORD(lParam) + wd->org.x, (short)HIWORD(lParam) + wd->org.y, wParam);
    break;

  case WM_LBUTTONDOWN:
    w->MouseLButtonPressed((short)LOWORD(lParam) + wd->org.x, (short)HIWORD(lParam) + wd->org.y, wParam);
    break;

  case WM_LBUTTONUP:
    w->MouseLButtonReleased((short)LOWORD(lParam) + wd->org.x, (short)HIWORD(lParam) + wd->org.y, wParam);
    break;

  case WM_RBUTTONDBLCLK:
    w->MouseRButtonDblClicked((short)LOWORD(lParam) + wd->org.x, (short)HIWORD(lParam) + wd->org.y, wParam);
    break;

  case WM_RBUTTONDOWN:
    w->MouseRButtonPressed((short)LOWORD(lParam) + wd->org.x, (short)HIWORD(lParam) + wd->org.y, wParam);
    break;

  case WM_RBUTTONUP:
    w->MouseRButtonReleased((short)LOWORD(lParam) + wd->org.x, (short)HIWORD(lParam) + wd->org.y, wParam);
    break;

  case WM_MOUSEACTIVATE:
    SendMessage(hWnd, WM_NCACTIVATE, TRUE, 0);
    SetFocus(hWnd);
    if (lRes) *lRes = MA_ACTIVATE;
    return TRUE;

  case WM_MOUSEMOVE:
    w->MouseMoved((short)lParam + wd->org.x, (short)((unsigned long)lParam >> 16) + wd->org.y);
    break;

  case WM_MOVE:
    w->Moved();
    break;

  case WM_NCHITTEST:
    lRes1 = DefWindowProc(hWnd, message, wParam, lParam);
    if (g_mouseWind && (lRes1 != HTCLIENT || g_mouseWind != w)) g_mouseWind->MouseLeft();
    g_mouseWind = lRes1 != HTCLIENT? NULL: w;
    if (lRes) *lRes = lRes1;
    break;

  case WM_PAINT:
    BeginPaint(hWnd, &ps);
    DS = new DrawingSurface(*(Window *)NULL);
    dsd = (DSData *)DS->m_dsData;
    dsd->wnd = w;
    if (ps.hdc)
    {
      dsd->hDC = ps.hdc;
      *(RECT *)&invRect = ps.rcPaint;
    }
    else // when the whole window is redrawn, you get a null hDC
    {
      dsd->hDC = GetDC(wd->hWnd);
      GetClientRect(hWnd, (RECT *)&invRect);
    }
    if (wd->palette) SelectPalette(dsd->hDC, wd->palette, FALSE);
    SetViewportOrgEx(dsd->hDC, -wd->org.x, -wd->org.y, NULL);
    SetBkMode(dsd->hDC, TRANSPARENT);
    invRect += wd->org;
    if (wd->screenCache)
    {
      Rect clientRect = w->ClientRect();
      if (SetBitmapSize(wd->screenCache, clientRect.right, clientRect.bottom))
      {
        DrawingSurface ds1(*w);
        w->Exposed(ds1, clientRect);
      }
      hDC = CreateCompatibleDC(dsd->hDC);
      SelectObject(hDC, wd->screenCache);
      BitBlt(dsd->hDC, invRect.left, invRect.top, invRect.right - invRect.left, 
        invRect.bottom - invRect.top, hDC, invRect.left, invRect.top, SRCCOPY);
      DeleteDC(hDC);
    }
    else w->Exposed(*DS, invRect);
    s_hBr = (HBRUSH)GetCurrentObject(dsd->hDC, OBJ_BRUSH);
    delete DS;
    EndPaint(hWnd, &ps);
    return TRUE;

  case WM_PALETTECHANGED:
    if (wd->palette && wParam != (WPARAM)hWnd)
    {
      hDC = GetDC(hWnd);
      SelectPalette(hDC, wd->palette, FALSE);
      UpdateColors(hDC);
      ReleaseDC(hWnd, hDC);
    }
    break;

  case WM_QUERYNEWPALETTE:
    if (wd->palette)
    {
      hDC = GetDC(hWnd);
      SelectPalette(hDC, wd->palette, FALSE);
      i = RealizePalette(hDC);
      ReleaseDC(hWnd, hDC);
      if (i) InvalidateRect(hWnd, NULL, TRUE);
    }
    break;

  case WM_SETCURSOR:
    if (g_isWaiting) g_waitCursor.Set(); // If the application is in a wait state, set the cursor to the Wait one regardless
    else
    {
      Cursor *cursor = (Cursor *)-1;
      w->SetCursor(cursor);
      if (cursor == (Cursor *)-1) return FALSE; // No cursor was set - go to default processing
      if (cursor) cursor->Set();
    }
    return TRUE;

  case WM_SETFOCUS:
    if (hWnd != g_hWndNuu) w->GotFocus();
    break;

  case WM_SIZE:
    if (hWnd == g_hWndNuu) break;
    if (wParam == SIZE_MINIMIZED) w->Minimized();
    else if (wParam == SIZE_MAXIMIZED || wParam == SIZE_RESTORED)
    {
      if (wd->canvasW || wd->canvasH)
      {
        Point org = wd->org;
        w->SetCanvas();
        if (org != wd->org)
        {
          MoveChildren(w, org - wd->org);
          ScrollWindowEx(wd->hWnd, org.x - wd->org.x, org.y - wd->org.y, 
            NULL, NULL, NULL, NULL, SW_INVALIDATE);
        }
      }
      w->Sized();
    }
    break;

  case WM_SYSCOLORCHANGE:
    w->SysColoursChanged();
    break;

  case WM_VSCROLL:
    {
      if (!wd->canvasH) break;
      GetClientRect(hWnd, (RECT *)&invRect);
      int orgY = wd->org.y, newOrgY;
      switch (LOWORD(wParam))
      {
      case SB_BOTTOM:     newOrgY = wd->canvasH - invRect.bottom; break;
      case SB_LINELEFT:   newOrgY = orgY - 10; break;
      case SB_LINERIGHT:  newOrgY = orgY + 10; break;
      case SB_PAGELEFT:   newOrgY = orgY - invRect.bottom; break;
      case SB_PAGERIGHT:  newOrgY = orgY + invRect.bottom; break;
      case SB_TOP:        newOrgY = 0; break;
      case SB_THUMBPOSITION:
      case SB_THUMBTRACK: newOrgY = HIWORD(wParam); break;
      default: return FALSE;
      }
      w->SetCanvasOrg(Point(wd->org.x, newOrgY), false);
    }
    break;

  case 0x020A: // WM_MOUSEWHEEL - this #define is not being picked up. I don't know why.
    if (wd->canvasH)
    {
      Point newOrg = wd->org + Point(0, -(short)HIWORD(wParam));
      if (newOrg.y < 0) newOrg.y = 0;
      else
      {
        Rect rect;
        GetClientRect(wd->hWnd, (RECT *)&rect);
        int maxY = max(wd->canvasH - rect.bottom, 0);
        if (newOrg.y > maxY) newOrg.y = maxY;
      }
      w->SetCanvasOrg(newOrg, false);
    }
    break;

/* This message is handled rather than being sent to default processing in order to avoid
   WM_MOVE and WM_SIZE messages coming from function calls rather than just user interaction
  case WM_WINDOWPOSCHANGED:
    return TRUE; */

  default:
    if (message >= WM_USER && message <= 0x7FFF)
		{
			w->UserMessage(message, wParam, lParam);
		}
		else
		{
			w->OtherMessage(message, wParam, lParam);
		}

    break;
  }
  return FALSE;
}
