#define WINDOBJ_MAIN

#include <WindObj/Private.h>

#include <malloc.h>

static WNDPROC s_oldDlgProc = NULL;

static LRESULT CALLBACK NewDialogProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  LRESULT lRes = 0;
  Window *w = (Window *)GetWindowLong(hWnd, GWL_USERDATA);
  if (w && message != WM_PAINT && StdMessageProcessing(&lRes, w, hWnd, message, wParam, lParam)) return lRes;
  return CallWindowProc(s_oldDlgProc, hWnd, message, wParam, lParam);
}

static BOOL CALLBACK DialProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
    {return 0;}

// (x, y) = offset of top left corner from parent (pix ?)
// (width, height) = client area dimensions in dialog box units

ModelessDialog::ModelessDialog(Window *parent, int x, int y, int width, int height, const char *title, int flags)
{
  DialogBoxData *dbd = new DialogBoxData;
  m_windowData = dbd;
  static bool registered = true;
  static char *fontName = "MS Sans Serif";
  static int fontSize = 8;
  if (!title) title = "";
  wchar_t *dlgTemp = (wchar_t *)_alloca(((sizeof(DLGTEMPLATE) + 1) / 2 + 
    strlen(title) + strlen(fontName) + 5) * sizeof(wchar_t)), *ptr;

  m_parent = parent;
  DLGTEMPLATE _dlgTemp =
  {
    DS_MODALFRAME | DS_SETFONT | WS_POPUP | WS_CAPTION | TranslateWindowStyle(flags),
    WS_EX_ACCEPTFILES | WS_EX_CONTROLPARENT, 0, x, y, width, height
  };
  *(DLGTEMPLATE *)dlgTemp = _dlgTemp;
  ptr = (wchar_t *)((DLGTEMPLATE *)dlgTemp + 1);
  ptr[0] = ptr[1] = 0;
  MultiByteToWideChar(CP_UTF8, 0, title, -1, ptr + 2, strlen(title) + 1);
  ptr += strlen(title) + 3;
  ptr[0] = fontSize;
  MultiByteToWideChar(CP_UTF8, 0, fontName, -1, ptr + 1, strlen(fontName) + 1);

  memset(dbd, 0, sizeof(DialogBoxData));
  dbd->hWnd = CreateDialogIndirect(g_hInst, (DLGTEMPLATE *)dlgTemp, parent->m_windowData->hWnd, 
    (DLGPROC)DialProc);
  if (!dbd->hWnd) throw WindObjError();

  dbd->dialMP = new DialogMP(dbd->hWnd); // translates TAB & CURSOR keystrokes
  SetWindowLong(dbd->hWnd, GWL_USERDATA, (LONG)this);
  dbd->foreColour = GetSysColor(COLOR_WINDOWTEXT);
  dbd->backBrush = CreateSolidBrush(GetSysColor(COLOR_3DFACE));
  WNDPROC dbp = (WNDPROC)SetWindowLong(dbd->hWnd, GWL_WNDPROC, (LONG)NewDialogProc);
  if (!s_oldDlgProc) s_oldDlgProc = dbp;
}

ModelessDialog::~ModelessDialog()
{
  delete ((DialogBoxData *)m_windowData)->dialMP;
  DeleteObject(((ControlData *)m_windowData)->backBrush);
}

// Convert from dialog box (font-based) units to pixels ...

Rect ModelessDialog::DlgToPixel(const Rect &dlgRect)
{
  RECT rect = {dlgRect.left, dlgRect.top, dlgRect.right, dlgRect.bottom};
  MapDialogRect(((WindowData *)m_windowData)->hWnd, &rect);
  Rect rect1 = Rect(rect.left, rect.top, rect.right, rect.bottom);
  return rect1;
}

Rect ModelessDialog::DlgToPixel(int x, int y, int w, int h)
{
  return DlgToPixel(Rect(x, y, x + w, y + h));
}

Label *ModelessDialog::AddLabel(int x, int y, int w, int h, const char *text, TextAlignment ta)
{
  return new Label((Window *)this, DlgToPixel(x, y, w, h), text, ta);
}

Label *ModelessDialog::AddLabel(int x, int y, const Bitmap &bm)
{
  return new Label(this, DlgToPixel(x, y, 0, 0).TopLeft(), bm);
}

EditBox *ModelessDialog::AddEditBox(int x, int y, int w, int h, int maxChars, int style, const char *text)
{
  return new EditBox((Window *)this, DlgToPixel(x, y, w, h), NULL, maxChars, style, text);
}

EditBox *ModelessDialog::AddEditBox(int x, int y, int w, int h, int maxChars, TextChangedAction tca, int style, const char *text)
{
  return new SimpleEditBox((Window *)this, DlgToPixel(x, y, w, h), maxChars, tca, style, text);
}

GroupBox *ModelessDialog::AddGroupBox(int x, int y, int w, int h, const char *text)
{
  return new GroupBox((Window *)this, DlgToPixel(x, y, w, h), text);
}

Button *ModelessDialog::AddButton(int x, int y, int w, int h, const char *text, ButtonClickedAction bca, Button::Style bs)
{
  return new SimpleButton((Window *)this, DlgToPixel(x, y, w, h), text, bca, bs);
}

RadioButton *ModelessDialog::AddRadioButton(int x, int y, int w, int h, const char *text, ButtonClickedAction bca, Button::Style bs)
{
  return new SimpleRadioButton((Window *)this, DlgToPixel(x, y, w, h), text, bca, bs);
}

CheckBox *ModelessDialog::AddCheckBox(int x, int y, int w, int h, const char *text)
{
  return new CheckBox((Window *)this, DlgToPixel(x, y, w, h), text);
}

// Alert boxes ...

WO_EXPORT void ShowAlertBox(Window *parent, const char *message, const char *title, 
                                        AlertBoxAction aba, AB_Type type)
{
  AB_Action act;

  switch(MessageBox(parent->m_windowData->hWnd, message, title, 
    type == AB_yesNoCancel? MB_YESNOCANCEL: MB_YESNO))
  {
  case IDCANCEL: default: act = AB_cancelSelected; break;
  case IDNO:  act = AB_noSelected; break;
  case IDYES: act = AB_yesSelected; break;
  }
  (*aba)(parent, act); 
}

// File selection dialog box ...

void ShowFileSelectDialog(Window *parent, const char *filter, const char *initialDir, FileSelectCallback FCB)
{
  char fileTitle[260], file[260];
  OPENFILENAME ofn; 
  file[0] = '\0';
  ofn.lStructSize = sizeof(OPENFILENAME); 
  ofn.hwndOwner = parent? parent->m_windowData->hWnd: NULL;
  ofn.hInstance = NULL; 
  ofn.lpstrFilter = filter; 
  ofn.lpstrCustomFilter = NULL; 
  ofn.nMaxCustFilter = 0;
  ofn.nFilterIndex = 0; 
  ofn.lpstrFile = file; 
  ofn.nMaxFile = sizeof(file); 
  ofn.lpstrFileTitle = fileTitle; 
  ofn.nMaxFileTitle = sizeof(fileTitle); 
  ofn.lpstrInitialDir = initialDir; 
  ofn.lpstrTitle = NULL;
  ofn.Flags = OFN_SHOWHELP | OFN_PATHMUSTEXIST |  
      OFN_FILEMUSTEXIST | OFN_LONGNAMES; 
  ofn.lpstrDefExt = NULL;
  ofn.lCustData = 0;
  ofn.lpfnHook = NULL;
  ofn.lpTemplateName = NULL;
  memset(fileTitle, 0, 260);
  BOOL notCancelled = GetOpenFileName(&ofn);
  (*FCB)(notCancelled? file: NULL);
}
