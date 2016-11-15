#define WINDOBJ_MAIN

#include <WindObj/Private.h>

static LRESULT CALLBACK WorkspaceWndProc(HWND hWnd, UINT message,
    WPARAM wParam, LPARAM lParam);

static LRESULT CALLBACK NewMDIWndProc(HWND hWnd, UINT message,
    WPARAM wParam, LPARAM lParam);

static WNDPROC g_oldMdiWndProc;

Window *g_mouseWind = NULL;

Workspace::Workspace(const char *title, const Icon *icon, Menu *mainMenu, Menu *windowMenu)
{
  WorkspaceData *wd = new WorkspaceData;
  m_windowData = wd;
  static bool registered = false;
  HMENU hMenu = NULL;
  HICON hIcon = icon? (HICON)icon->m_iconData: NULL;

  wd->hWnd = NULL;
  wd->hWindowMenu = NULL;

  if (wd->menu = mainMenu)
  {
    mainMenu->Activate();
    hMenu = (HMENU)mainMenu->m_menuData->id;
    if (windowMenu)
    {
      MenuData *wmd = windowMenu->m_menuData;
      wd->hWindowMenu = wmd->type & MF_POPUP? (HMENU)wmd->id: NULL;
    }
  }

  if (!g_hInstPrev && !registered)
  {
    WNDCLASS wndClass =
    {
      0, WorkspaceWndProc, 0, 0, g_hInst, hIcon, LoadCursor(NULL, IDC_ARROW), 
      (HBRUSH)(COLOR_APPWORKSPACE + 1), NULL, "WO_Workspace"
    };
    RegisterClass(&wndClass);
    registered = TRUE;
  }

  CreateWindow("WO_Workspace", title, WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
    CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, hMenu, g_hInst, this);

  wd->messProc = new WorkspaceMP(wd->hWnd);

  ShowWindow(wd->hWnd, SW_SHOWDEFAULT);
}

Workspace::~Workspace()
{
  WorkspaceData *wd = (WorkspaceData *)m_windowData;
  delete wd->messProc;
}

static LRESULT CALLBACK WorkspaceWndProc(HWND hWnd, UINT message,
    WPARAM wParam, LPARAM lParam)
{
  Workspace *ws = (Workspace *)GetWindowLong(hWnd, GWL_USERDATA);
  WorkspaceData *wd;
  CLIENTCREATESTRUCT s_ccs;
  HWND hChild;
//  DrawingSurface DS;

  if (!ws)
  {
/* WM_GETMINMAXINFO, WM_NCCREATE & WM_NCCALCSIZE are sent before this message, but do not
   need to be processed (yet) ... */
    if (message != WM_CREATE) return DefWindowProc(hWnd, message, wParam, lParam);
    ws = (Workspace *)((CREATESTRUCT *)lParam)->lpCreateParams;
    wd = (WorkspaceData *)ws->m_windowData;
    SetWindowLong(wd->hWnd = hWnd, GWL_USERDATA, (LONG)ws);
    s_ccs.hWindowMenu = wd->hWindowMenu;
    s_ccs.idFirstChild = 50000;
    wd->hWndMdi = CreateWindow("MDICLIENT", NULL,
      WS_CHILD | WS_CLIPCHILDREN | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL,
      0, 0, 0, 0, hWnd, NULL, g_hInst, &s_ccs);
    g_oldMdiWndProc = (WNDPROC)SetWindowLong(wd->hWndMdi, GWL_WNDPROC, (LONG)NewMDIWndProc);
    return 0;
  }
  wd = (WorkspaceData *)ws->m_windowData;
  LRESULT lRes;
  if (ProcessMenuMessage(lRes, wd, message, wParam, lParam)) return lRes;

  switch (message)
  {
  case WM_CLOSE:
    ws->Closed();
    return 0;

// needed (only) for the popup label, to make sure its background is the same as the window background ...
  case WM_CTLCOLORSTATIC:
    return (LRESULT)p_backgroundBrush;

  case WM_NCHITTEST:
    if (g_mouseWind != (Window *)ws)
    {
      if (g_mouseWind) g_mouseWind->MouseLeft();
      g_mouseWind = (Window *)ws;
    }
    break;

  case WM_SYSCOLORCHANGE:
    DeleteObject(p_backgroundBrush);
    p_backgroundBrush = CreateSolidBrush(GetSysColor(COLOR_WINDOW));
    ws->SysColoursChanged();
    for (hChild = GetWindow(wd->hWndMdi, GW_CHILD); hChild; hChild = GetWindow(hChild, GW_HWNDNEXT))
      SendMessage(hChild, WM_SYSCOLORCHANGE, 0, 0);
    break;
  }
  return DefFrameProc(hWnd, wd->hWndMdi, message, wParam, lParam);
}

/* The sub-classed MDI client window procedure passes on keyboard messages to
   an MDI child which has failed to get the keyboard focus (the kb messages will not
   arrive here if everything worked ok) ... */ 

static LRESULT CALLBACK NewMDIWndProc(HWND hWnd, UINT message,
    WPARAM wParam, LPARAM lParam)
{
  HWND hWndFocus = GetFocus();
  if ((message == WM_KEYUP || message == WM_KEYDOWN || message == WM_CHAR) && hWnd != hWndFocus)
    SendMessage(hWndFocus, message, wParam, lParam);
  else if (message == WM_NCHITTEST && g_mouseWind)
  {
    g_mouseWind->MouseLeft();
    g_mouseWind = NULL;
  }
  return CallWindowProc(g_oldMdiWndProc, hWnd, message, wParam, lParam);
}

static LRESULT CALLBACK MDIChildWndProc(HWND hWnd, UINT message,
    WPARAM wParam, LPARAM lParam);

Window *Workspace::AddWindow(const char *title, const Icon *icon)
{
  Window *w = new Window;
  WindowData *wd = new WindowData;
  w->m_windowData = wd;
  static bool registd = false;
  HICON hIcon = icon? (HICON)icon->m_iconData: NULL;

  memset(wd, 0, sizeof(WindowData));
  
  if (!g_hInstPrev && !registd)
  {
    WNDCLASS wndClass =
    {
      CS_DBLCLKS, MDIChildWndProc, 
        0, 0, g_hInst, hIcon, (HCURSOR)NULL, (HBRUSH)NULL, 
        NULL, "WO_MDIChild"
    };
    RegisterClass(&wndClass);
    registd = true;
  }
  CreateWindowEx(WS_EX_MDICHILD, "WO_MDIChild", (char *)title, 
    WS_VISIBLE | WS_CHILD | WS_CLIPSIBLINGS, CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, 
    m_windowData->hWnd, NULL, g_hInst, (LPVOID)w);
  return w;
}

static LRESULT CALLBACK MDIChildWndProc(HWND hWnd, UINT message,
    WPARAM wParam, LPARAM lParam)
{
  Window *w = (Window *)GetWindowLong(hWnd, GWL_USERDATA);
  WindowData *wd;
  LRESULT lRes = 0;

  if (w && StdMessageProcessing(&lRes, w, hWnd, message, wParam, lParam)) return lRes;

/* WM_GETMINMAXINFO, WM_NCCREATE & WM_NCCALCSIZE are sent before this message, but do not
   need to be processed (yet) ... */
  if (!w && message == WM_CREATE)
  {
    w = (Window *)((MDICREATESTRUCT *)((LPCREATESTRUCT)lParam)->lpCreateParams)->lParam;
    wd = w->m_windowData;
    wd->hWnd = hWnd;
    SetWindowLong(wd->hWnd = hWnd, GWL_USERDATA, (LONG)w);
  }
  return DefMDIChildProc(hWnd, message, wParam, lParam);
}
