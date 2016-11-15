#define WINDOBJ_MAIN

#include <WindObj/Private.h>
#include <WindObj/MiniWindow.h>

#include <malloc.h>

// Push buttons ...

static WNDPROC s_oldButtonProc = NULL;

static LRESULT CALLBACK NewButtonProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  Window *w = (Window *)GetWindowLong(hWnd, GWL_USERDATA);
  LRESULT lRes;
  if (w && message != WM_PAINT && StdMessageProcessing(&lRes, w, hWnd, message, wParam, lParam))
    return lRes;
  return CallWindowProc(s_oldButtonProc, hWnd, message, wParam, lParam);
}

Button::Button() {}

Button::Button(Window *parent, Rect rect, const char *text, Style bs, Font *font)
{
  ControlData *cd = new ControlData;
  m_windowData = cd;
  HWND hWndParent = parent->m_windowData->hWnd;
  m_parent = parent;

  memset(cd, 0, sizeof(ControlData));
  cd->hWnd = CreateWindow("BUTTON", text, WS_CHILD | WS_VISIBLE | WS_TABSTOP | 
    (bs == defaultAction? BS_DEFPUSHBUTTON: BS_PUSHBUTTON), 
    rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, 
    hWndParent, (HMENU)(bs == defaultAction? IDOK: bs == cancel? IDCANCEL: 0), g_hInst, NULL);
  HFONT hFont;
  if (font) hFont = CloneFont((HFONT)font->m_fontData);
  else
  {
    hFont = (HFONT)SendMessage(hWndParent, WM_GETFONT, 0, 0);
    if (!hFont) hFont = (HFONT)GetStockObject(ANSI_VAR_FONT);
  }
  SendMessage(cd->hWnd, WM_SETFONT, (WPARAM)hFont, 0);
  cd->foreColour = GetSysColor(COLOR_WINDOWTEXT);
  cd->backBrush = CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
  SetWindowLong(cd->hWnd, GWL_USERDATA, (LONG)this);
  WNDPROC bp = (WNDPROC)SetWindowLong(cd->hWnd, GWL_WNDPROC, (LONG)NewButtonProc);
  if (!s_oldButtonProc) s_oldButtonProc = bp;
}

Button::Button(Window *parent, Rect rect, const Bitmap &bm, Style bs)
{
  ControlData *cd = new ControlData;
  m_windowData = cd;
  m_parent = parent;

  memset(cd, 0, sizeof(ControlData));
  cd->hWnd = CreateWindow("BUTTON", "JIM", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_BITMAP |
    (bs == defaultAction? BS_DEFPUSHBUTTON: BS_PUSHBUTTON), 
    rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, 
    parent->m_windowData->hWnd, (HMENU)(bs == defaultAction? IDOK: bs == cancel? IDCANCEL: 0), g_hInst, NULL);
  cd->foreColour = GetSysColor(COLOR_WINDOWTEXT);
  cd->backBrush = CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
  SetWindowLong(cd->hWnd, GWL_USERDATA, (LONG)this);
  SendMessage(cd->hWnd, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)bm.m_bitmapData->hBitmap);
  WNDPROC bp = (WNDPROC)SetWindowLong(cd->hWnd, GWL_WNDPROC, (LONG)NewButtonProc);
  if (!s_oldButtonProc) s_oldButtonProc = bp;
}

Button::~Button()
{
  HBRUSH hBr = ((ControlData *)m_windowData)->backBrush;
  if (hBr) DeleteObject(hBr);
}

void Button::Clicked() {}

Colour Button::TextColour()
{
  COLORREF clr = ((ControlData *)m_windowData)->foreColour;
  return Colour(GetRValue(clr), GetGValue(clr), GetBValue(clr));
}

void Button::TextColour(Colour clr)
{
  ((ControlData *)m_windowData)->foreColour = CVTCLR(clr);
  Draw();
}

SimpleButton::SimpleButton(Window *parent, Rect rect, const char *text, ButtonClickedAction bca, Style buttonStyle, Font *font) :
   Button(parent, rect, text, buttonStyle, font)
{
  m_bca = bca;
}

void SimpleButton::Clicked()
{
  (*m_bca)(this);
}

// Check boxes ...

CheckBox::CheckBox() {}

CheckBox::CheckBox(Window *parent, Rect rect, const char *text)
{
  ControlData *cd = new ControlData;
  m_windowData = cd;
  HWND hWndParent = parent->m_windowData->hWnd;
  m_parent = parent;

  memset(cd, 0, sizeof(ControlData));
  cd->hWnd = CreateWindow("BUTTON", text, WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX, 
    rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, 
    hWndParent, 0, g_hInst, NULL);
  SendMessage(cd->hWnd, WM_SETFONT, SendMessage(hWndParent, WM_GETFONT, 0, 0), 0);
  cd->foreColour = GetSysColor(COLOR_WINDOWTEXT);
  cd->backBrush = CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
  SetWindowLong(cd->hWnd, GWL_USERDATA, (LONG)this);
  WNDPROC bp = (WNDPROC)SetWindowLong(cd->hWnd, GWL_WNDPROC, (LONG)NewButtonProc);
  if (!s_oldButtonProc) s_oldButtonProc = bp;
}

bool CheckBox::Check(bool state)
{
  SendMessage(m_windowData->hWnd, BM_SETCHECK, 
    state? BST_CHECKED: BST_UNCHECKED, 0);
  return state;
}

CheckBox::operator bool()
{
  return (SendMessage(m_windowData->hWnd, BM_GETSTATE, 0, 0) & 3) == BST_CHECKED;
}

// Radio buttons ...

RadioButton::RadioButton(Window *parent, Rect rect, const char *text, Style bs)
{
  ControlData *cd = new ControlData;
  m_windowData = cd;
  HWND hWndParent = parent->m_windowData->hWnd;
  m_parent = parent;

  memset(cd, 0, sizeof(ControlData));
  cd->hWnd = CreateWindow("BUTTON", text, WS_CHILD | WS_VISIBLE | WS_TABSTOP | 
    BS_AUTORADIOBUTTON | (bs == startOfGroup? WS_GROUP: 0), 
    rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, 
    hWndParent, 0, g_hInst, NULL);
  SendMessage(cd->hWnd, WM_SETFONT, SendMessage(hWndParent, WM_GETFONT, 0, 0), 0);
  cd->foreColour = GetSysColor(COLOR_WINDOWTEXT);
  cd->backBrush = CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
  SetWindowLong(cd->hWnd, GWL_USERDATA, (LONG)this);
  WNDPROC bp = (WNDPROC)SetWindowLong(cd->hWnd, GWL_WNDPROC, (LONG)NewButtonProc);
  if (!s_oldButtonProc) s_oldButtonProc = bp;
}

SimpleRadioButton::SimpleRadioButton(Window *parent, Rect rect, const char *text, ButtonClickedAction bca, Style bs) : 
   RadioButton(parent, rect, text, bs)
{
  m_bca = bca;
}

void SimpleRadioButton::Clicked()
{
  if (m_bca) (*m_bca)(this);
}

// Combo boxes ...

static WNDPROC s_oldComboProc = NULL;

static LRESULT CALLBACK NewComboProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  Window *w = (Window *)GetWindowLong(hWnd, GWL_USERDATA);
  LRESULT lRes;
  if (w && message != WM_PAINT && StdMessageProcessing(&lRes, w, hWnd, message, wParam, lParam))
    return lRes;
  return CallWindowProc(s_oldComboProc, hWnd, message, wParam, lParam);
}

ComboBox::ComboBox(){}

ComboBox::ComboBox(Window *parent, Rect rect, int maxChars, int style, Font *font)
{
  ControlData *cd = new ControlData;
  m_windowData = cd;
  HWND hWndParent = parent->m_windowData->hWnd;
  m_parent = parent;

  memset(cd, 0, sizeof(ControlData));
  cd->hWnd = CreateWindow("COMBOBOX", "", WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP | 
    CBS_AUTOHSCROLL | style,
    rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, 
    hWndParent, 0, g_hInst, NULL);
  SendMessage(cd->hWnd, WM_SETFONT, font? (LRESULT)CloneFont((HFONT)font->m_fontData): SendMessage(hWndParent, WM_GETFONT, 0, 0), 0);
  if (maxChars > 0) SendMessage(cd->hWnd, CB_LIMITTEXT, maxChars, 0);
  cd->backBrush = CreateSolidBrush(GetSysColor(COLOR_WINDOW));
  cd->foreColour = GetSysColor(COLOR_WINDOWTEXT);
  SetWindowLong(cd->hWnd, GWL_USERDATA, (LONG)this);
  WNDPROC cbp = (WNDPROC)SetWindowLong(cd->hWnd, GWL_WNDPROC, (LONG)NewComboProc);
  if (!s_oldComboProc) s_oldComboProc = cbp;
}

ComboBox::~ComboBox()
{
  DeleteObject(((ControlData *)m_windowData)->backBrush);
}

void ComboBox::Clear()
{
  SendMessage(m_windowData->hWnd, CB_RESETCONTENT, 0, 0);
}

void ComboBox::Delete(int idx)
{
  SendMessage(m_windowData->hWnd, CB_DELETESTRING, idx, 0);
}

int ComboBox::Insert(const char *buff)
{
  g_hWndNuu = m_windowData->hWnd;
  LRESULT lRes = SendMessage(m_windowData->hWnd, CB_ADDSTRING, 0, (LPARAM)buff);
  g_hWndNuu = NULL;
  return lRes;
}

int ComboBox::Insert(int idx, const char *buff)
{
  g_hWndNuu = m_windowData->hWnd;
  LRESULT lRes = SendMessage(m_windowData->hWnd, CB_INSERTSTRING, idx, (LPARAM)buff);
  g_hWndNuu = NULL;
  return lRes;
}

int ComboBox::ListIndex()
{
  return SendMessage(m_windowData->hWnd, CB_GETCURSEL, 0, 0);
}

void ComboBox::ListIndex(int idx)
{
  SendMessage(m_windowData->hWnd, CB_SETCURSEL, idx, 0);
}

int ComboBox::ListCount()
{
  return SendMessage(m_windowData->hWnd, CB_GETCOUNT, 0, 0);
}

int ComboBox::GetText(char *text, int idx)
{
  if (idx < 0) idx = ListIndex();
  LRESULT lRes = SendMessage(m_windowData->hWnd, CB_GETLBTEXTLEN, idx, 0);
  int n = lRes == LB_ERR? 0: lRes;
  if (text) SendMessage(m_windowData->hWnd, CB_GETLBTEXT, idx, (LPARAM)text);
  return n;
}

int ComboBox::GetText(char *text)
{
  return Window::GetText(text);
}

void ComboBox::SetText(int idx, const char *text)
{
  g_hWndNuu = m_windowData->hWnd;
  int i, n = ListCount();
  if (idx < 0)
    SendMessage(g_hWndNuu, CB_INSERTSTRING, (WPARAM)-1, (LPARAM)text);
  else if (idx >= n)
  {
    for (i = idx; i < n; i++)
      SendMessage(g_hWndNuu, CB_ADDSTRING, 0, (LPARAM)"");
    SendMessage(g_hWndNuu, CB_ADDSTRING, 0, (LPARAM)text);
  }
  else
  {
    SendMessage(g_hWndNuu, CB_DELETESTRING, idx, 0);
    SendMessage(g_hWndNuu, CB_INSERTSTRING, idx, (LPARAM)text);
  }
  g_hWndNuu = NULL;
}

void ComboBox::SetText(int idx, const char *text, int nText)
{
  char *buff = (char *)_alloca(nText + 1);
  memcpy(buff, text, nText);
  buff[nText] = '\0';
  SetText(idx, buff);
}

void ComboBox::SetText(const char *text)
{
  Window::SetText(text);
}

void ComboBox::SetText(const char *text, int nText)
{
  Window::SetText(text, nText);
}

Colour ComboBox::TextColour()
{
  COLORREF clr = ((ControlData *)m_windowData)->foreColour;
  return Colour(GetRValue(clr), GetGValue(clr), GetBValue(clr));
}

void ComboBox::TextColour(Colour clr)
{
  ((ControlData *)m_windowData)->foreColour = CVTCLR(clr);
  Draw();
}

void ComboBox::ListIndexChanged() {}
void ComboBox::TextChanged() {}

// Drop down list ...

DropDownList::DropDownList(Window *parent, Rect rect, Font *font)
{
  ControlData *cd = new ControlData;
  m_windowData = cd;
  HWND hWndParent = parent->m_windowData->hWnd;
  m_parent = parent;
  memset(cd, 0, sizeof(ControlData));
  cd->hWnd = CreateWindow("COMBOBOX", "", WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP |
    CBS_DROPDOWNLIST, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, 
    hWndParent, 0, g_hInst, NULL);
  SendMessage(cd->hWnd, WM_SETFONT, font? (LRESULT)CloneFont((HFONT)font->m_fontData): SendMessage(hWndParent, WM_GETFONT, 0, 0), 0);
  cd->backBrush = CreateSolidBrush(GetSysColor(COLOR_WINDOW));
  cd->foreColour = GetSysColor(COLOR_WINDOWTEXT);
  SetWindowLong(cd->hWnd, GWL_USERDATA, (LONG)this);
  WNDPROC oldComboProc = (WNDPROC)SetWindowLong(cd->hWnd, GWL_WNDPROC, (LONG)NewComboProc);
  if (!s_oldComboProc) s_oldComboProc = oldComboProc;
}

// methods same as Combo, except that edit box ones will not do anything

void SimpleDropDownList::ListIndexChanged()
{
  (*m_lica)(this);
}

// Edit boxes ...

static WNDPROC s_oldEditProc = NULL;

static LRESULT CALLBACK NewEditProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  Window *w = (Window *)GetWindowLong(hWnd, GWL_USERDATA);
  LRESULT lRes;
  if (w && message != WM_PAINT && StdMessageProcessing(&lRes, w, hWnd, message, wParam, lParam))
    return lRes;
  return CallWindowProc(s_oldEditProc, hWnd, message, wParam, lParam);
}

EditBox::EditBox(Window *parent, Rect rect, const Font *font, int maxChars, int style, const char *text,
                 const Colour &backColour, const Colour &foreColour)
{
  ControlData *cd = new ControlData;
  m_windowData = cd;
  HWND hWndParent = parent->m_windowData->hWnd;
  m_parent = parent;
  memset(cd, 0, sizeof(ControlData));
  cd->hWnd = CreateWindow("EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER | WS_GROUP | WS_TABSTOP | style, 
    rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, 
    hWndParent, 0, g_hInst, NULL);
  SendMessage(cd->hWnd, WM_SETFONT, font? (LPARAM)CloneFont((HFONT)font->m_fontData): 
    SendMessage(hWndParent, WM_GETFONT, 0, 0), 0);
  if (maxChars > 0) SendMessage(cd->hWnd, EM_SETLIMITTEXT, maxChars, 0);
  if (text) SetWindowText(cd->hWnd, text);
  cd->backBrush = CreateSolidBrush(&backColour? CVTCLR(backColour): GetSysColor(COLOR_WINDOW));
  cd->foreColour = &foreColour? CVTCLR(foreColour): GetSysColor(COLOR_WINDOWTEXT);
  SetWindowLong(cd->hWnd, GWL_USERDATA, (LONG)this);
  WNDPROC ep = (WNDPROC)SetWindowLong(cd->hWnd, GWL_WNDPROC, (LONG)NewEditProc);
  if (!s_oldEditProc) s_oldEditProc = ep;
}

EditBox::~EditBox()
{
  ControlData *cd = (ControlData *)m_windowData;
  DeleteObject((HFONT)SendMessage(cd->hWnd, WM_GETFONT, 0, 0));
  DeleteObject(cd->backBrush);
}

void EditBox::Clear()
{
  SendMessage(m_windowData->hWnd, WM_SETTEXT, 0, (LPARAM)"");
}

void EditBox::TextChanged() {}

int EditBox::Selection(int &selStart, int &selStop)
{
  SendMessage(m_windowData->hWnd, EM_GETSEL, (WPARAM)&selStart, (LPARAM)&selStop);
  return selStop - selStart;
}

int EditBox::SetSelection(int selStart, int selStop)
{
  SendMessage(m_windowData->hWnd, EM_SETSEL, (WPARAM)selStart, (LPARAM)selStop);
  return selStop - selStart;
}

int Selection(int &selStart, int &selStop)
{
  char buff[6] = {'\0'};
  HWND hWnd;

  GetAtomName(GetClassWord(hWnd = GetFocus(), GCW_ATOM), buff, 6);
  if (!_stricmp(buff, "edit")) SendMessage(hWnd, EM_GETSEL, 
    (WPARAM)&selStart, (LPARAM)&selStop);
  else selStart = selStop = 0;
  return selStop - selStart;
}

Colour EditBox::TextColour()
{
  COLORREF clr = ((ControlData *)m_windowData)->foreColour;
  return Colour(GetRValue(clr), GetGValue(clr), GetBValue(clr));
}

void EditBox::TextColour(Colour clr)
{
  ((ControlData *)m_windowData)->foreColour = CVTCLR(clr);
  Draw();
}

SimpleEditBox::SimpleEditBox(Window *parent, Rect rect, int maxChars, TextChangedAction tca, int style, const char *text, 
  Font *font, const Colour &backColour, const Colour &foreColour) : 
    EditBox(parent, rect, font, maxChars, style, text, backColour, foreColour)
{
  m_tca = tca;
}

void SimpleEditBox::TextChanged()
{
  (*m_tca)(this);
}

GroupBox::GroupBox(Window *parent, Rect rect, const char *caption, Font *font, Colour &foreColour, Colour &backColour) : 
  Window(parent, rect), m_foreColour(foreColour), m_backColour(backColour)
{
  m_caption = caption? strcpy(m_caption = new char[strlen(caption) + 1], caption): NULL;
  m_font = font? new Font(*font): NULL;
}

void GroupBox::Exposed(DrawingSurface &DS, const Rect &invRect)
{
  DS.Rectangle(invRect, Pen(), Brush(m_backColour));
  Rect clientRect = ClientRect();
  DS.Rectangle(Rect(clientRect.left, clientRect.top + 1 + m_font->TextHeight() / 2, clientRect.right - 1, clientRect.bottom - 1), 
    Pen(m_foreColour, 0), Brush(m_backColour));
  DS.Text(m_caption, clientRect.TopLeft() + Point(m_font->TextWidth("M"), 0), *m_font, m_foreColour, DrawingSurface::left,
    DrawingSurface::top, DrawingSurface::opaque, m_backColour);
}

GroupBox::~GroupBox()
{
  delete [] m_caption;
  delete m_font;
}

// Labels ...

static WNDPROC s_oldLabelProc = NULL;

static LRESULT CALLBACK NewLabelProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  Window *w = (Window *)GetWindowLong(hWnd, GWL_USERDATA);
  LRESULT lRes;
  if (w && message != WM_PAINT && StdMessageProcessing(&lRes, w, hWnd, message, wParam, lParam))
    return lRes;
  return CallWindowProc(s_oldLabelProc, hWnd, message, wParam, lParam);
}

Label::Label(Window *parent, Rect rect, const char *text, TextAlignment ta, const Font *font, 
    const Colour &backColour, const Colour &foreColour)
{
  ControlData *cd = new ControlData;
  m_windowData = cd;
  HWND hWndParent = parent->m_windowData->hWnd;
  m_parent = parent;

  memset(cd, 0, sizeof(ControlData));
  cd->hWnd = CreateWindow("STATIC", text, WS_CHILD | WS_VISIBLE | 
    (ta == TA_centre? SS_CENTER: ta == TA_right? SS_RIGHT: SS_LEFT), 
    rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, 
    hWndParent, 0, g_hInst, NULL);
  SendMessage(cd->hWnd, WM_SETFONT, font? (LPARAM)CloneFont((HFONT)font->m_fontData): 
    SendMessage(hWndParent, WM_GETFONT, 0, 0), 0);
  cd->backBrush = CreateSolidBrush(&backColour? CVTCLR(backColour): GetSysColor(COLOR_3DFACE));
  cd->foreColour = &foreColour? CVTCLR(foreColour): GetSysColor(COLOR_WINDOWTEXT);
  SetWindowLong(cd->hWnd, GWL_USERDATA, (LONG)this);
  WNDPROC lp = (WNDPROC)SetWindowLong(cd->hWnd, GWL_WNDPROC, (LONG)NewLabelProc);
  if (!s_oldLabelProc) s_oldLabelProc = lp;
}

Label::Label(Window *parent, Point pt, const Bitmap &bm)
{
  ControlData *cd = new ControlData;
  m_windowData = cd;
  m_parent = parent;
  
  memset(cd, 0, sizeof(ControlData));
  cd->hWnd = CreateWindow("STATIC", "", WS_CHILD | WS_VISIBLE | SS_BITMAP, 
    pt.x, pt.y, 0, 0, parent->m_windowData->hWnd, 0, g_hInst, NULL);
  cd->backBrush = CreateSolidBrush(GetSysColor(COLOR_3DFACE));
  cd->foreColour = GetSysColor(COLOR_WINDOWTEXT);
  SendMessage(cd->hWnd, STM_SETIMAGE, IMAGE_BITMAP,
    (LPARAM)CloneBitmap(bm.m_bitmapData->hBitmap));
  SetWindowLong(cd->hWnd, GWL_USERDATA, (LONG)this);
  WNDPROC lp = (WNDPROC)SetWindowLong(cd->hWnd, GWL_WNDPROC, (LONG)NewLabelProc);
  if (!s_oldLabelProc) s_oldLabelProc = lp;
}

Label::~Label()
{
  HBITMAP hbm = (HBITMAP)SendMessage(m_windowData->hWnd, STM_GETIMAGE, IMAGE_BITMAP, 0);
  if (hbm) DeleteObject(hbm);
  HBRUSH hBr = ((ControlData *)m_windowData)->backBrush;
  if (hBr) DeleteObject(hBr);
}

void Label::SetImage(const Bitmap &bm)
{
  HBITMAP hbm = (HBITMAP)SendMessage(m_windowData->hWnd, STM_GETIMAGE, IMAGE_BITMAP, 0);
  if (hbm) DeleteObject(hbm);
  SendMessage(m_windowData->hWnd, STM_SETIMAGE, IMAGE_BITMAP,
    (LPARAM)CloneBitmap(bm.m_bitmapData->hBitmap));
}

/* List boxes ...

static WNDPROC s_oldListProc = NULL;

static LRESULT CALLBACK NewListProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  Window *w = (Window *)GetWindowLong(hWnd, GWL_USERDATA);
  LRESULT lRes = 0;
    if (w && message != WM_PAINT && StdMessageProcessing(&lRes, w, hWnd, message, wParam, lParam))
    return lRes;
  return CallWindowProc(s_oldListProc, hWnd, message, wParam, lParam);
}

ListBox::ListBox(Window *parent, Rect rect, int nCols)
{
  ControlData *cd = new ControlData;
  m_windowData = cd;
  HWND hWndParent = parent->m_windowData->hWnd;
  m_parent = parent;

  memset(cd, 0, sizeof(ControlData));
  cd->hWnd = CreateWindow("LISTBOX", "", WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP | LBS_USETABSTOPS |
    LBS_NOINTEGRALHEIGHT | WS_VSCROLL,
    rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, 
    hWndParent, 0, g_hInst, NULL);
  SendMessage(cd->hWnd, WM_SETFONT, SendMessage(hWndParent, WM_GETFONT, 0, 0), 0);
  cd->backBrush = CreateSolidBrush(GetSysColor(COLOR_WINDOW));
  cd->foreColour = GetSysColor(COLOR_WINDOWTEXT);
  m_nCols = nCols;
  m_minWidth = new int[2 * nCols];
  m_tabPos = m_minWidth + nCols;
  memset(m_minWidth, 0, 2 * nCols * sizeof(int));
  SetWindowLong(cd->hWnd, GWL_USERDATA, (LONG)this);
  WNDPROC lp = (WNDPROC)SetWindowLong(cd->hWnd, GWL_WNDPROC, (LONG)NewListProc);
  if (!s_oldListProc) s_oldListProc = lp;
}

ListBox::~ListBox()
{
  delete m_minWidth;
  DeleteObject(((ControlData *)m_windowData)->backBrush);
}

/* Widens a list box, if necessary, to accommodate multi-column text whose
   tab positions have been set by a call to SetColWidths() 
   Returns the new list box width in pixels ...

int ListBox::AutoWidth(int minWidth)
{
  RECT rectW, rectC;
  int ww;
  HWND hWnd = m_windowData->hWnd;

  GetWindowRect1(hWnd, &rectW);
  GetClientRect(hWnd, &rectC);
  ww = m_tabPos[m_nCols - 1] + rectW.right - rectW.left - rectC.right;
  if (minWidth > ww) ww = minWidth;
  MoveWindow(hWnd, rectW.left, rectW.top, ww, rectW.bottom - rectW.top, TRUE);
  return ww;
}

void ListBox::Clear()
{
  g_hWndNuu = m_windowData->hWnd;
  SendMessage(g_hWndNuu, LB_RESETCONTENT, 0, 0);
  g_hWndNuu = NULL;
}

void ListBox::Delete(int idx)
{
  g_hWndNuu = m_windowData->hWnd;
  SendMessage(m_windowData->hWnd, LB_DELETESTRING, idx, 0);
  g_hWndNuu = NULL;
}

int ListBox::Insert(const char *buff)
{
  g_hWndNuu = m_windowData->hWnd;
  LRESULT lRes = SendMessage(m_windowData->hWnd, LB_ADDSTRING, 0, (LPARAM)buff);
  g_hWndNuu = NULL;
  return lRes;
}

int ListBox::Insert(int idx, const char *buff)
{
  g_hWndNuu = m_windowData->hWnd;
  LRESULT lRes = SendMessage(m_windowData->hWnd, LB_INSERTSTRING, idx, (LPARAM)buff);
  g_hWndNuu = NULL;
  return lRes;
}

Rect ListBox::ItemRect(int idx) const
{
  RECT iRect;
  SendMessage(m_windowData->hWnd, LB_GETITEMRECT, idx, (LPARAM)&iRect);
  return Rect(iRect.left, iRect.top, iRect.right, iRect.bottom);
}

int ListBox::GetText(char *text, int idx) const
{
  if (idx < 0) idx = ListIndex();
  LRESULT lRes = SendMessage(m_windowData->hWnd, LB_GETTEXTLEN, idx, 0);
  int n = lRes == LB_ERR? 0: lRes;
  if (text) SendMessage(m_windowData->hWnd, LB_GETTEXT, idx, (LPARAM)text);
  return n;
}

int ListBox::GetText(char *text) const
{
  return GetText(text, ListIndex());
}

int ListBox::ItemHeight() const
{
  return SendMessage(m_windowData->hWnd, LB_GETITEMHEIGHT, 0, 0);
}

int ListBox::ListCount() const
{
  return SendMessage(m_windowData->hWnd, LB_GETCOUNT, 0, 0);
}

int ListBox::ListIndex() const
{
  return SendMessage(m_windowData->hWnd, LB_GETCURSEL, 0, 0);
}

void ListBox::ListIndexChanged() {}

/* m_tabPos[] = tab stops (offsets from left in pixels) --
     m_tabPos[0] is the tab position of the second column. m_tabPos[m_nCols - 1] 
     is the window width required to accomodate all the text

   m_minWidth[] (pixels) can be set to > 0 to guarantee minimum column widths 

void ListBox::SetColWidths()
{
  int i, j, nBuff = 0, n = ListCount();
  char *ptr, *ptr1;
  AutomaticArray(int, wid, m_nCols)
  memset(wid, 0, m_nCols * sizeof(int));
  SIZE siz;
  HDC hDC = CreateIC("DISPLAY", NULL, NULL, NULL);
  HWND hWnd = m_windowData->hWnd;
  SelectObject(hDC, (HFONT)SendMessage(hWnd, WM_GETFONT, 0, 0));

  for (i = nBuff = 0; i < n; i++)
    if ((j = GetText(NULL, i)) > nBuff) nBuff = j;
  AutomaticArray(char, buff, nBuff + 1)
  for (i = 0; i < n; i++)
  {
    GetText(buff, i);
    for (ptr = ptr1 = buff, j = 0; *ptr1 && j < m_nCols; ptr = ptr1 + 1, j++)
    {
      ptr1 = strchr(ptr, '\t');
      if (!ptr1) ptr1 = ptr + strlen(ptr);
      GetTextExtentPoint32(hDC, ptr, ptr1 - ptr, &siz);
      if (siz.cx > wid[j]) wid[j] = siz.cx;
    }
  }
  m_tabPos[0] = max(wid[0], m_minWidth[0]) + 4;
  for (i = 1; i < m_nCols; i++)
    m_tabPos[i] = max(wid[i], m_minWidth[i]) + 4 + m_tabPos[i - 1];
  DeleteDC(hDC);
  SetTabStops();
}

void ListBox::ListIndex(int idx)
{
  SendMessage(g_hWndNuu = m_windowData->hWnd, LB_SETCURSEL, idx, 0);
  g_hWndNuu = NULL;
}

int ListBox::MatchString(const char *text) const
{
  return (int)SendMessage(m_windowData->hWnd, LB_FINDSTRINGEXACT, (WPARAM)-1, (LPARAM)text);
}

/* Sets up tab stops in list box based on information given in 
   m_tabPos[] array

void ListBox::SetTabStops()
{
  Rect rect = ((ModelessDialog *)m_parent)->DlgToPixel(Rect(0, 0, 2100, 0));
  AutomaticArray(int, tp, m_nCols - 1)

  for (int i = 0; i < m_nCols - 1; i++)
    tp[i] = m_tabPos[i] * 2100 / rect.right;

  SendMessage(m_windowData->hWnd, LB_SETTABSTOPS, m_nCols - 1, (LPARAM)tp);
}

void ListBox::SetText(int idx, const char *text)
{
  g_hWndNuu = m_windowData->hWnd;
  int i, n = ListCount();
  if (idx < 0)
    SendMessage(g_hWndNuu, LB_INSERTSTRING, (WPARAM)-1, (LPARAM)text);
  else if (idx >= n)
  {
    for (i = idx; i < n; i++)
      SendMessage(g_hWndNuu, LB_ADDSTRING, 0, (LPARAM)"");
    SendMessage(g_hWndNuu, LB_ADDSTRING, 0, (LPARAM)text);
  }
  else
  {
    SendMessage(g_hWndNuu, LB_DELETESTRING, idx, 0);
    SendMessage(g_hWndNuu, LB_INSERTSTRING, idx, (LPARAM)text);
    ListIndex(idx);
  }
  g_hWndNuu = NULL;
}

void ListBox::SetText(int idx, const char *text, int nText)
{
  char *buff = (char *)_alloca(nText + 1);
  memcpy(buff, text, nText);
  buff[nText] = '\0';
  SetText(idx, buff);
}

void ListBox::SetText(const char *text)
{
  int i = ListIndex();
  SetText(i == -1? ListCount(): i, text);
}

void ListBox::SetText(const char *text, int nText)
{
  char *buff = (char *)_alloca(nText + 1);
  memcpy(buff, text, nText);
  buff[nText] = '\0';
  SetText(buff);
}

void ListBox::SetTopIndex(int idx)
{
  g_hWndNuu = m_windowData->hWnd;
  SendMessage(g_hWndNuu, LB_SETTOPINDEX, idx, 0);
  g_hWndNuu = NULL;
}

HWND g_floatingEdit = NULL;
int g_floatEdColNo = -1;

void ListBox::Edit(int col)
{
  if (col < 0)
  {
    if (g_floatingEdit && IsWindowVisible(g_floatingEdit))
    {
      if (col == -1)
      {
        int nText = GetWindowTextLength(g_floatingEdit);
        char *text = (char *)alloca(nText + 1);
        GetWindowText(g_floatingEdit, text, nText + 1);
        SetText(g_floatEdColNo, text);
      }
      ShowWindow(g_floatingEdit, SW_HIDE);
      g_floatEdColNo = -1;
    }
    SetFocus();
  }
  else
  {
    g_hWndNuu = m_windowData->hWnd;
    RECT rect;
    SendMessage(g_hWndNuu, LB_GETITEMRECT, (WPARAM)ListIndex(), (LPARAM)&rect);
//    MapWindowPoints(g_hWndNuu, hwnddlg, (LPPOINT)&rect, 2);
    char *text = (char *)alloca(GetText(NULL, col) + 1);
    GetText(text, g_floatEdColNo = col);
    if (!g_floatingEdit)
      g_floatingEdit = CreateWindow("EDIT", text, WS_POPUP | WS_VISIBLE | WS_BORDER, 
        rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, 
        NULL, 0, g_hInst, NULL);
    else
    {
      MoveWindow(g_floatingEdit, rect.left + m_tabPos[col] - 1, rect.top - 3,
      m_tabPos[col + 1] - m_tabPos[col], rect.bottom - rect.top + 7, FALSE);
      ::SetWindowText(g_floatingEdit, text);
    }
    SendMessage(g_floatingEdit, EM_SETSEL, 0, -1);
//    SetFocus(g_floatingEdit);
    ShowWindow(g_floatingEdit, SW_SHOW);
  }
}

int ListBox::EditCol() const {return g_floatEdColNo;}

bool ListBox::EditMode() const {return g_floatEdColNo != -1;} */
