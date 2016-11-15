#define WINDOBJ_MAIN

#include <WindObj/Private.h>

#define _USE_MATH_DEFINES

#include <limits.h>
#include <malloc.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

Point::Point(int xi, int yi)
{
  x = xi;
  y = yi;
}

Point::Point() {x = y = 0;}

bool Point::operator==(const Point &pt) {return !memcmp(this, &pt, sizeof(Point));}
bool Point::operator!=(const Point &pt) {return !(*this == pt);}

Point Point::operator+(const Point &pt)
{
  Point pt1(*this);
  pt1.x += pt.x;
  pt1.y += pt.y;
  return pt1;
}

Point Point::operator-(const Point &pt)
{
  Point pt1(*this);
  pt1.x -= pt.x;
  pt1.y -= pt.y;
  return pt1;
}

Point &Point::operator+=(const Point &pt)
{
  x += pt.x;
  y += pt.y;
  return *this;
}

Point &Point::operator-=(const Point &pt)
{
  x -= pt.x;
  y -= pt.y;
  return *this;
}

Rect::Rect() {}

Rect::Rect(int l, int t, int r, int b)
{
  left = l;
  top = t;
  right = r;
  bottom = b;
}

Rect::Rect(const Point &topLeft, const Point &bottomRight)
{
  left = topLeft.x;
  top = topLeft.y;
  right = bottomRight.x;
  bottom = bottomRight.y;
}

Rect::operator bool() const
{
  return left != right && top != bottom;
}

bool Rect::operator!() const
{
  return left == right || top == bottom;
}

// intersection ...

Rect Rect::operator&(const Rect &rect) const
{
  Rect r = rect;
  if (r.right > r.left)
  {
    if (right < r.right) r.right = right;
    if (left > r.left) r.left = left;
    if (r.right <= r.left) return Rect(0, 0, 0, 0);
  }
  else if (r.right == r.left) return Rect(0, 0, 0, 0);
  else
  {
    if (right > r.right) r.right = right;
    if (left < r.left) r.left = left;
    if (r.right >= r.left) return Rect(0, 0, 0, 0);
  }
  if (r.bottom > r.top)
  {
    if (bottom < r.bottom) r.bottom = bottom;
    if (top > r.top) r.top = top;
    if (r.bottom <= r.top) return Rect(0, 0, 0, 0);
  }
  else if (r.bottom == r.top) return Rect(0, 0, 0, 0);
  else
  {
    if (bottom > r.bottom) r.bottom = bottom;
    if (top < r.top) r.top = top;
    if (r.bottom >= r.top) return Rect(0, 0, 0, 0);
  }
  return r;
}

Rect &Rect::operator&=(const Rect &rect)
{
  return *this = *this & rect;
}

// smallest rectangle that encloses both ...

Rect Rect::operator|(const Rect &rect) const
{
  if (top == bottom || left == right) return rect;
  if (rect.top == rect.bottom || rect.left == rect.right) return *this;
  return Rect(min(left, rect.left), min(top, rect.top), max(right, rect.right), max(bottom, rect.bottom));
}

Rect &Rect::operator|=(const Rect &rect)
{
  return *this = *this | rect;
}

bool Rect::operator&(const Point &pt) const
{
  if (right > left)
  {
    if (pt.x > right || pt.x < left) return false;
  }
  else
  {
    if (pt.x > left || pt.x < right) return false;
  }
  if (bottom > top)
  {
    if (pt.y > bottom || pt.y < top) return false;
  }
  else
  {
    if (pt.y > top || pt.y < bottom) return false;
  }
  return true;
}

Rect Rect::operator+(Point pt) const
{
  return Rect(left + pt.x, top + pt.y, right + pt.x, bottom + pt.y);
}

Rect Rect::operator-(Point pt) const
{
  return Rect(left - pt.x, top - pt.y, right - pt.x, bottom - pt.y);
}

Rect &Rect::operator+=(Point pt)
{
  left += pt.x;
  top += pt.y;
  right += pt.x;
  bottom += pt.y;
  return *this;
}

Rect &Rect::operator-=(Point pt)
{
  left -= pt.x;
  top -= pt.y;
  right -= pt.x;
  bottom -= pt.y;
  return *this;
}

bool Rect::operator==(Rect rect) const
{
  return !memcmp(this, &rect, sizeof(Rect));
}

bool Rect::operator!=(Rect rect) const
{
  return !(*this == rect);
}

bool Rect::operator>=(Rect rect) const
{
  if (right > left)
  {
    if (rect.right <= rect.left) return true;
    if (right < rect.right || left > rect.left) return false;
  }
  else if (right < left)
  {
    if (rect.right >= rect.left) return true;
    if (right > rect.right || left < rect.left) return false;
  }
  else return false;
  if (bottom > top)
  {
    if (rect.bottom <= rect.top) return true;
    if (bottom < rect.right || top > rect.top) return false;
  }
  else if (bottom < top)
  {
    if (rect.bottom >= rect.top) return true;
    if (bottom > rect.bottom || top < rect.top) return false;
  }
  else return false;
  return true;
}

bool Rect::operator<=(Rect rect) const
{
  return rect >= *this;
}

bool Rect::operator<(Rect rect) const
{
  return !(*this >= rect);
}

bool Rect::operator>(Rect rect) const
{
  return !(*this <= rect);
}

Point Rect::TopLeft() const
{
  return Point(left, top);
}

Point Rect::TopRight() const
{
  return Point(right, top);
}

Point Rect::BottomLeft() const
{
  return Point(left, bottom);
}

Point Rect::BottomRight() const
{
  return Point(right, bottom);
}

Colour::Colour() : std(0), red(0), green(0), blue(0) {}

Colour::Colour(unsigned char r, unsigned char g, unsigned char b) : 
  std(0), red(r), green(g), blue(b) {}

Colour::Colour(Std sc) : std((unsigned char)sc), red(0), green(0), blue(0) {}

unsigned char BlendedColour(unsigned char c1, unsigned char c2, float fac)
{
  float c = (1 - fac) * c1 + fac * c2;
  if (c <= 0.5) return 0;
  if (c >= 254.5) return 255;
  return (unsigned char)c;
}

Colour::Colour(Colour clr1, Colour clr2, float blendFactor)
{
  Colour clr1RGB = clr1.rgb(), clr2RGB = clr2.rgb();
  std = 0;
  red = BlendedColour(clr1RGB.red, clr2RGB.red, blendFactor);
  green = BlendedColour(clr1RGB.green, clr2RGB.green, blendFactor);
  blue = BlendedColour(clr1RGB.blue, clr2RGB.blue, blendFactor);
}

bool Colour::operator==(const Colour &clr)
{
  return !memcmp(this, &clr, sizeof(Colour));
}

bool Colour::operator!=(const Colour &clr)
{
  return !!memcmp(this, &clr, sizeof(Colour));
}

Colour Colour::rgb() const
{
  Colour clr = *this;
  if (std)
  {
    COLORREF cr = GetSysColor((int)std - 1);
    clr.std = 0;
    clr.red = GetRValue(cr);
    clr.green = GetGValue(cr);
    clr.blue = GetBValue(cr);
  }
  return clr;
}

COLORREF CVTCLR(Colour clr)
{
  Colour clrRGB = clr.rgb();
  return RGB(clrRGB.red, clrRGB.green, clrRGB.blue);
}

bool ConfigureColour(Colour &colour, Window *parent)
{
  const Colour::Std s_stdColour[] =
  {
    Colour::window, Colour::windowText, Colour::windowFrame, 
    Colour::highlight, Colour::highlightText, 
    Colour::buttonFace, Colour::buttonShadow, Colour::buttonHighlight, Colour::buttonText, 
    Colour::menu, Colour::menuText, Colour::greyText, 
    Colour::background,
    Colour::activeCaption, 
    Colour::scrollBar, 
    Colour::captionText
  };
  static COLORREF s_colorRef[16];
  static bool s_custColoursInit = false;
  if (!s_custColoursInit)
  {
    s_custColoursInit = true;
    for (int i = 0; i < 16; i++)
    {
      Colour s = Colour(s_stdColour[i]).rgb();
      s_colorRef[i] = RGB(s.red, s.green, s.blue);
    }
  }
  CHOOSECOLOR CC;
  memset(&CC, 0, sizeof(CHOOSECOLOR));
  CC.lStructSize = sizeof(CHOOSECOLOR);
  CC.hwndOwner = parent? parent->m_windowData->hWnd: NULL;
  CC.rgbResult = RGB(colour.red, colour.green, colour.blue);
  CC.Flags = CC_RGBINIT | CC_SOLIDCOLOR;
  CC.lpCustColors = s_colorRef;
  if (!ChooseColor(&CC)) return false;
  colour.std = 0;
  colour.red = GetRValue(CC.rgbResult);
  colour.green = GetGValue(CC.rgbResult);
  colour.blue = GetBValue(CC.rgbResult);
  return true;
}

DrawingSurface::DrawingSurface()
{
  DSData *dsd = new DSData;
  m_dsData = dsd;
  dsd->m_refs = 1;
  dsd->hDC = CreateIC("DISPLAY", NULL, NULL, NULL);
  dsd->wnd = NULL;
}

DrawingSurface::DrawingSurface(Window &w)
{
  DSData *dsd = new DSData;
  m_dsData = dsd;
  dsd->m_refs = 1;
  if (!&w) return;
  HBITMAP hScreenCache = w.m_windowData->screenCache;
  if (hScreenCache && hScreenCache != (HBITMAP)-1)
  {
    dsd->hDC = CreateCompatibleDC(NULL); // compatible with current screen
    dsd->wnd = NULL;
    SelectObject(dsd->hDC, w.m_windowData->screenCache);
  }
  else
  {
    dsd->wnd = &w;
    dsd->hDC = GetDC(w.m_windowData->hWnd);
    SetViewportOrgEx(dsd->hDC, -w.m_windowData->org.x, -w.m_windowData->org.y, NULL);
  }
  SetBkMode(dsd->hDC, TRANSPARENT);
}

DrawingSurface::DrawingSurface(const Bitmap &bitmap)
{
  DSData *dsd = new DSData;
  m_dsData = dsd;
  dsd->m_refs = 1;
  dsd->hDC = CreateCompatibleDC(NULL); // compatible with current screen
  SetBkMode(dsd->hDC, TRANSPARENT);
  dsd->wnd = NULL;
  SelectObject(dsd->hDC, bitmap.m_bitmapData->hBitmap);
  if (bitmap.m_bitmapData->hPalette) SelectPalette(dsd->hDC, bitmap.m_bitmapData->hPalette, 0);
}

DrawingSurface::DrawingSurface(const char *printerName, bool portraitMode)
{
  int nSize = DocumentPropertiesA(NULL, NULL, (LPSTR)printerName, NULL, NULL, 0);
  if (nSize < 0) throw WindObjError();
  DEVMODE *devMode = (DEVMODE *)malloc(nSize);
  DocumentPropertiesA(NULL, NULL, (LPSTR)printerName, devMode, NULL, DM_OUT_BUFFER);
  devMode->dmOrientation = portraitMode? DMORIENT_PORTRAIT: DMORIENT_LANDSCAPE;
  m_dsData = new DSData;
  m_dsData->m_refs = 1;
  m_dsData->wnd = NULL;
  m_dsData->hDC = CreateDCA("WINSPOOL", printerName, NULL, devMode);
  free(devMode);
  if (!m_dsData->hDC) throw WindObjError();
  DOCINFO docInfo;
  memset(&docInfo, 0, sizeof(DOCINFO));
  int res = StartDoc(m_dsData->hDC, &docInfo);
  if (!res) throw WindObjError();
  if (StartPage(m_dsData->hDC) <= 0) throw WindObjError();
}

void DrawingSurface::NewPage()
{
  EndPage(m_dsData->hDC);
  StartPage(m_dsData->hDC);
}

DrawingSurface::DrawingSurface(const DrawingSurface &DS)
{
  DS.m_dsData->m_refs++;
}

DrawingSurface::~DrawingSurface()
{
  if (--m_dsData->m_refs > 0) return;
  if (GetDeviceCaps(m_dsData->hDC, TECHNOLOGY) == DT_RASPRINTER)
  {
    EndPage(m_dsData->hDC);
    EndDoc(m_dsData->hDC);
  }
  HBRUSH hBrush = (HBRUSH)GetCurrentObject(m_dsData->hDC, OBJ_BRUSH);
  HFONT hFont = (HFONT)GetCurrentObject(m_dsData->hDC, OBJ_FONT);
  HPEN hPen = (HPEN)GetCurrentObject(m_dsData->hDC, OBJ_PEN);
  if (m_dsData->wnd) ReleaseDC(m_dsData->wnd->m_windowData->hWnd, m_dsData->hDC);
  else if (m_dsData->hDC) DeleteDC(m_dsData->hDC);
  delete m_dsData;
  DeleteObject(hBrush);
  DeleteObject(hFont);
  DeleteObject(hPen);
}

DrawingSurface &DrawingSurface::operator=(const DrawingSurface &DS)
{
  DrawingSurface::~DrawingSurface();
  ((DSData *)(m_dsData = DS.m_dsData))->m_refs++;
  return *this;
}

void DrawingSurface::Origin(int x, int y)
{
  if (!SetViewportOrgEx(m_dsData->hDC, x, y, NULL))
    throw WindObjError();
}

void DrawingSurface::Scale(int xlog, int xpix, int ylog, int ypix)
{
  HDC hDC = m_dsData->hDC;

  SetMapMode(hDC, xlog == xpix && ylog == ypix? MM_TEXT : (xlog == ylog || xlog == -ylog) && 
    (xpix == ypix || xpix == -ypix)? MM_ISOTROPIC: MM_ANISOTROPIC);
  SetWindowExtEx(hDC, xlog, ylog, NULL);     // logical extents
  SetViewportExtEx(hDC, xpix, ypix, NULL);   // pixel extents
}

FontInfoStruct DrawingSurface::FontInfo() const
{
  TEXTMETRIC tm;
  GetTextMetrics(m_dsData->hDC, &tm);
  FontInfoStruct FIS = {tm.tmHeight};
  return FIS;
}

GlyphMetricsStruct DrawingSurface::GlyphMetrics(char glyph) const
{
  GLYPHMETRICS GM;
  MAT2 mx = {{0, 1}, {0, 0}, {0, 0}, {0, 1}};
  GetGlyphOutline(m_dsData->hDC, glyph, GGO_METRICS, &GM, 0, NULL, &mx);
  GlyphMetricsStruct gm;
  gm.origin.x = GM.gmptGlyphOrigin.x;
  gm.origin.y = GM.gmptGlyphOrigin.y;
  gm.blackBox.x = GM.gmBlackBoxX;
  gm.blackBox.y = GM.gmBlackBoxY;
  gm.increment.x = GM.gmCellIncX;
  gm.increment.y = GM.gmCellIncY;
  return gm;
}

void DrawingSurface::TextColour(const Colour &clr)
{
  SetTextColor(m_dsData->hDC, CVTCLR(clr));
}

void DrawingSurface::TextColour(Colour::Std sc)
{
  TextColour(Colour(sc));
}

Colour DrawingSurface::TextColour() const
{
  COLORREF cr = GetTextColor(m_dsData->hDC);
  return Colour(GetRValue(cr), GetGValue(cr), GetBValue(cr));
}

static int MLTextWidth(const DrawingSurface &DS, const char *buff)
{
  if (!buff || !buff[0]) return 0;
  const char *ptr1 = buff, *ptr2;
  SIZE siz = {0, 0};
  for (ptr2 = ptr1;; ptr2++)
  {
    if (*ptr2 && !strchr("\n\v\r\f", *ptr2)) continue;
    SIZE siz1;
    GetTextExtentPoint32(DS.m_dsData->hDC, ptr1, ptr2 - ptr1, &siz1);
    if (siz1.cx > siz.cx) siz.cx = siz1.cx;
    if (!*ptr2) return siz.cx;
    ptr1 = ptr2 + 1;
  }
}

static int MLTextWidth(const DrawingSurface &DS, const wchar_t *buff)
{
  if (!buff || !buff[0]) return 0;
  const wchar_t *ptr1 = buff, *ptr2;
  SIZE siz = {0, 0};
  for (ptr2 = ptr1;; ptr2++)
  {
    if (*ptr2 && !wcschr(L"\n\v\r\f", *ptr2)) continue;
    SIZE siz1;
    GetTextExtentPoint32W(DS.m_dsData->hDC, ptr1, ptr2 - ptr1, &siz1);
    if (siz1.cx > siz.cx) siz.cx = siz1.cx;
    if (!*ptr2) return siz.cx;
    ptr1 = ptr2 + 1;
  }
}

static int MLTextWidth(const DrawingSurface &DS, const void *buff, bool isWide)
{
  return isWide? MLTextWidth(DS, (const wchar_t *)buff):
    MLTextWidth(DS, (const char *)buff);
}

Rect DrawingSurface::TextRect(const char *buff, int maxWidth, const Font &font) const
{
  HGDIOBJ oldFont;
  if (&font) oldFont = SelectObject(m_dsData->hDC, (HGDIOBJ)font.m_fontData);
  RECT rect = {0, 0, maxWidth? maxWidth: MLTextWidth(*this, buff), LONG_MAX};
  if (buff && buff[0]) DrawText(m_dsData->hDC, buff, strlen(buff), &rect, DT_CALCRECT | DT_WORDBREAK | DT_NOPREFIX);
  if (&font) SelectObject(m_dsData->hDC, oldFont);
  return Rect(0, 0, rect.right, rect.bottom);
}

Rect DrawingSurface::TextRect(const wchar_t *buff, int maxWidth, const Font &font) const
{
  HGDIOBJ oldFont;
  if (&font) oldFont = SelectObject(m_dsData->hDC, (HGDIOBJ)font.m_fontData);
  RECT rect = {0, 0, maxWidth? maxWidth: MLTextWidth(*this, buff), LONG_MAX};
  if (buff && buff[0]) DrawTextW(m_dsData->hDC, buff, wcslen(buff), &rect, DT_CALCRECT | DT_WORDBREAK | DT_NOPREFIX);
  if (&font) SelectObject(m_dsData->hDC, oldFont);
  return Rect(0, 0, rect.right, rect.bottom);
}

int DrawingSurface::Width() const {return GetDeviceCaps(m_dsData->hDC, HORZRES);}
int DrawingSurface::Height() const {return GetDeviceCaps(m_dsData->hDC, VERTRES);}

int DrawingSurface::XUnitsPerInch() const {return GetDeviceCaps(m_dsData->hDC, LOGPIXELSX);}
int DrawingSurface::YUnitsPerInch() const {return GetDeviceCaps(m_dsData->hDC, LOGPIXELSY);}

void DrawingSurface::BkColour(const Colour &clr)
{
  SetBkColor(m_dsData->hDC, CVTCLR(clr));
}

void DrawingSurface::BkColour(Colour::Std sc)
{
  BkColour(Colour(sc));
}

void DrawingSurface::Set(const Brush &brush)
{
  HBRUSH hBr = CloneBrush((HBRUSH)brush.m_brushData);
  HBRUSH hBr1 = (HBRUSH)GetCurrentObject(m_dsData->hDC, OBJ_BRUSH);
  DeleteObject(hBr = (HBRUSH)SelectObject(m_dsData->hDC, hBr));
}

void DrawingSurface::Set(const Font &font)
{
  DeleteObject(SelectObject(m_dsData->hDC, CloneFont((HFONT)font.m_fontData)));
}

void DrawingSurface::Set(const Pen &pen)
{
  DeleteObject(SelectObject(m_dsData->hDC, ClonePen((HPEN)pen.m_penData)));
}

void DrawingSurface::Set(const Point &point)
{
  MoveToEx(m_dsData->hDC, point.x, point.y, NULL);
}

void DrawingSurface::Set(DrawingSurface::BackgroundMode BM)
{
  SetBkMode(m_dsData->hDC, BM);
}

void DrawingSurface::Set(DrawingSurface::RasterOp rOp)
{
  SetROP2(m_dsData->hDC, rOp); 
}

void DrawingSurface::SetTextHAlign(DrawingSurface::TextHAlign THA)
{
  if (THA == defaultTHA) return;
  HDC hDC = m_dsData->hDC;
  UINT newTHA = GetTextAlign(hDC);
  switch (THA)
  {
  case left: newTHA &= ~TA_CENTER; break;
  case right: newTHA = (newTHA & ~TA_CENTER) | TA_RIGHT; break;
  case centred: newTHA |= TA_CENTER; break;
  default: break;
  }
  SetTextAlign(hDC, newTHA);
}

void DrawingSurface::SetTextVAlign(DrawingSurface::TextVAlign TVA)
{
  if (TVA == defaultTVA) return;
  HDC hDC = m_dsData->hDC;
  UINT newTVA = GetTextAlign(hDC);
  switch (TVA)
  {
  case top: newTVA &= ~TA_BASELINE; break;
  case baseLine: case vCentred: newTVA |= TA_BASELINE; break;
  case bottom: newTVA = (newTVA & ~TA_BASELINE) | TA_BOTTOM; break;
  default: break;
  }
  SetTextAlign(hDC, newTVA);
}

void DrawingSurface::SetClip(const Rect &rect)
{
  if (&rect)
  {
    Rect r = DeviceCoords(rect);
    HRGN hRgn = CreateRectRgn(r.left, r.top, r.right, r.bottom);
    SelectClipRgn(m_dsData->hDC, hRgn);
    DeleteObject(hRgn);
  }
  else SelectClipRgn(m_dsData->hDC, NULL);
}

Colour DrawingSurface::BkColour() const
{
  COLORREF cr = GetBkColor(m_dsData->hDC);
  return Colour(GetRValue(cr), GetGValue(cr), GetBValue(cr));
}

const float c_degToRad = (float)(M_PI / 180);

void DrawingSurface_Text(DrawingSurface &DS, const void *text, bool isWide, const Point &pt, const Font &font, const Colour &colour, 
  DrawingSurface::TextHAlign THA, DrawingSurface::TextVAlign TVA, DrawingSurface::BackgroundMode BM,
  const Colour &cellColour, int extraSpacing, float angle)
{
  POINT pt1;
  HDC hDC = DS.m_dsData->hDC;
  if (&pt) {pt1.x = pt.x; pt1.y = pt.y;}
  else GetCurrentPositionEx(hDC, &pt1);
  HGDIOBJ oldFont;
  if (&font) oldFont = SelectObject(hDC, (HGDIOBJ)font.m_fontData);
  COLORREF oldTC, oldBC;
  if (&colour) oldTC = SetTextColor(hDC, CVTCLR(colour));
  int oldBM = -1;
  if (BM != DrawingSurface::defaultBM) oldBM = SetBkMode(hDC, BM);
  UINT oldTA = GetTextAlign(hDC);
  DS.SetTextHAlign(THA);
  DS.SetTextVAlign(TVA);
  if (&cellColour) oldBC = SetBkColor(hDC, CVTCLR(cellColour));
  int oldES, oldGraphicsMode;
  XFORM xForm, oldXForm;
  if (extraSpacing >= 0) oldES = SetTextCharacterExtra(hDC, extraSpacing);
  if (angle != 0)
  {
    oldGraphicsMode = GetGraphicsMode(hDC);
    SetGraphicsMode(hDC, GM_ADVANCED);
    GetWorldTransform(hDC, &oldXForm);
    xForm.eDx = 0;
    xForm.eDy = 0;
    xForm.eM11 = xForm.eM22 = cos(angle * c_degToRad);
    xForm.eM21 = sin(angle * c_degToRad);
    xForm.eM12 = -xForm.eM21;
    SetWorldTransform(hDC, &xForm);
    POINT pt2 = {(int)(xForm.eM11 * pt1.x + xForm.eM12 * pt1.y), (int)(xForm.eM21 * pt1.x + xForm.eM22 * pt1.y)};
    pt1 = pt2;
  }
  if (isWide) TextOutW(hDC, pt1.x, pt1.y, (const wchar_t *)text, text? wcslen((const wchar_t *)text): 0);
  else TextOutA(hDC, pt1.x, pt1.y, (const char *)text, text? strlen((const char *)text): 0);
  if (angle != 0)
  {
    SetWorldTransform(hDC, &oldXForm);
    SetGraphicsMode(hDC, oldGraphicsMode);
  }
  if (extraSpacing >= 0) SetTextCharacterExtra(hDC, oldES);
  if (&cellColour) SetBkColor(hDC, oldBC);
  SetTextAlign(hDC, oldTA);
  if (oldBM != -1) SetBkMode(hDC, oldBM);
  if (&font) SelectObject(hDC, oldFont);
  if (&colour) SetTextColor(hDC, oldTC);
}

void DrawingSurface::Text(const char *text, const Point &pt, const Font &font, const Colour &colour, 
  DrawingSurface::TextHAlign THA, DrawingSurface::TextVAlign TVA, DrawingSurface::BackgroundMode BM,
  const Colour &cellColour, int extraSpacing, float angle)
{
  DrawingSurface_Text(*this, text, false, pt, font, colour, THA, TVA, BM, cellColour, extraSpacing, angle);
}

void DrawingSurface::Text(const wchar_t *text, const Point &pt, const Font &font, const Colour &colour, 
  DrawingSurface::TextHAlign THA, DrawingSurface::TextVAlign TVA, DrawingSurface::BackgroundMode BM,
  const Colour &cellColour, int extraSpacing, float angle)
{
  DrawingSurface_Text(*this, text, true, pt, font, colour, THA, TVA, BM, cellColour, extraSpacing, angle);
}

RECT DefaultRect(DSData *dsd)
{
  RECT rect1 = {0, 0, 0, 0};
  if (dsd->wnd) GetClientRect(dsd->wnd->m_windowData->hWnd, &rect1);
  else
  {
    BITMAP bm1;
    HBITMAP hbm = (HBITMAP)GetCurrentObject(dsd->hDC, OBJ_BITMAP);
    if (!hbm) return rect1;
    GetObject(hbm, sizeof(BITMAP), &bm1);
    rect1.right = bm1.bmWidth;
    rect1.bottom = bm1.bmHeight;
  }
  DPtoLP(dsd->hDC, (LPPOINT)&rect1, 2);
  return rect1;
}

Rect DrawingSurface_TextRect(const DrawingSurface &DS, const void *text, bool isWide, int maxWidth, const Font &font)
{
  return isWide? DS.TextRect((const wchar_t *)text, maxWidth, font): DS.TextRect((const char *)text, maxWidth, font);
}

void DrawingSurface_Text(DrawingSurface &DS, const void *text, bool isWide, const Rect &rect, const Font &font,
  const Colour &colour, DrawingSurface::TextHAlign THA, DrawingSurface::TextVAlign TVA,
  DrawingSurface::BackgroundMode BM, const Colour &cellColour, int extraSpacing, float angle)
{
  RECT rect1;
  if (&rect)
  {
    rect1.left = rect.left; rect1.top = rect.top;
    rect1.right = rect.right; rect1.bottom = rect.bottom;
  }
  else rect1 = DefaultRect(DS.m_dsData);
  HDC hDC = DS.m_dsData->hDC;
  HGDIOBJ oldFont;
  if (&font) oldFont = SelectObject(hDC, (HGDIOBJ)font.m_fontData);
  COLORREF oldTC, oldBC;
  if (&colour) oldTC = SetTextColor(hDC, CVTCLR(colour));
  int oldBM = -1;
  if (BM != DrawingSurface::defaultBM) oldBM = SetBkMode(hDC, BM);
  UINT drawTextOpts = DT_NOPREFIX | DT_WORDBREAK;

  if (THA == DrawingSurface::right || THA == DrawingSurface::defaultTHA &&
    (GetTextAlign(hDC) & TA_CENTER) == TA_RIGHT) drawTextOpts |= DT_RIGHT;
  else if (THA == DrawingSurface::centred || THA == DrawingSurface::defaultTHA &&
    (GetTextAlign(hDC)) & TA_CENTER == TA_CENTER) drawTextOpts |= DT_CENTER;

  if (&cellColour) oldBC = SetBkColor(hDC, CVTCLR(cellColour));
  int oldES;
  if (extraSpacing >= 0) oldES = SetTextCharacterExtra(hDC, extraSpacing);
  if (rect1.right == rect1.left)
    rect1.right = rect1.left + min(MLTextWidth(DS, text, isWide), DefaultRect(DS.m_dsData).right - rect1.left);
  if (rect1.bottom == rect1.top)
    rect1.bottom = rect1.top + DrawingSurface_TextRect(DS, text, isWide, rect1.right - rect1.left, font).bottom;
  if (TVA == DrawingSurface::vCentred || TVA == DrawingSurface::bottom)
  {
    int vOffset = rect1.bottom - rect1.top - DrawingSurface_TextRect(DS, text, isWide, rect1.right - rect1.left, font).bottom;
    if (TVA == DrawingSurface::vCentred) vOffset /= 2;
    rect1.top += vOffset;
    rect1.bottom += vOffset;
  }
  int oldGraphicsMode;
  XFORM xForm, oldXForm;
  if (extraSpacing >= 0) oldES = SetTextCharacterExtra(hDC, extraSpacing);
  if (angle != 0)
  {
    oldGraphicsMode = GetGraphicsMode(hDC);
    SetGraphicsMode(hDC, GM_ADVANCED);
    GetWorldTransform(hDC, &oldXForm);
    xForm.eDx = 0;
    xForm.eDy = 0;
    xForm.eM11 = xForm.eM22 = cos(angle * c_degToRad);
    xForm.eM21 = sin(angle * c_degToRad);
    xForm.eM12 = -xForm.eM21;
    SetWorldTransform(hDC, &xForm);
    POINT newOrg = {(int)(xForm.eM11 * rect1.left + xForm.eM12 * rect1.top),
      (int)(xForm.eM21 * rect1.left + xForm.eM22 * rect1.top)};
    rect1.right -= rect1.left;
    rect1.bottom -= rect1.top;
    rect1.left = newOrg.x;
    rect1.top = newOrg.y;
    rect1.right += newOrg.x;
    rect1.bottom += newOrg.y;
  }
  if (isWide) DrawTextW(hDC, (const wchar_t *)text, -1, &rect1, drawTextOpts);
  else DrawText(hDC, (const char *)text, -1, &rect1, drawTextOpts);
  if (angle != 0)
  {
    SetWorldTransform(hDC, &oldXForm);
    SetGraphicsMode(hDC, oldGraphicsMode);
  }
  if (extraSpacing >= 0) SetTextCharacterExtra(hDC, oldES);
  if (&cellColour) SetBkColor(hDC, oldBC);
  if (oldBM != -1) SetBkMode(hDC, oldBM);
  if (&font) SelectObject(hDC, oldFont);
  if (&colour) SetTextColor(hDC, oldTC);
}

void DrawingSurface::Text(const char *text, const Rect &rect, const Font &font,
  const Colour &colour, DrawingSurface::TextHAlign THA, DrawingSurface::TextVAlign TVA,
  DrawingSurface::BackgroundMode BM, const Colour &cellColour, int extraSpacing, float angle)
{
  DrawingSurface_Text(*this, text, false, rect, font, colour, THA, TVA, BM, cellColour, extraSpacing, angle);
}

void DrawingSurface::Text(const wchar_t *text, const Rect &rect, const Font &font,
  const Colour &colour, DrawingSurface::TextHAlign THA, DrawingSurface::TextVAlign TVA,
  DrawingSurface::BackgroundMode BM, const Colour &cellColour, int extraSpacing, float angle)
{
  DrawingSurface_Text(*this, text, true, rect, font, colour, THA, TVA, BM, cellColour, extraSpacing, angle);
}

void DrawingSurface::Text(char chr, const Point &pt, const Font &font, const Colour &colour, 
  DrawingSurface::TextHAlign THA, DrawingSurface::TextVAlign TVA, DrawingSurface::BackgroundMode BM,
  const Colour &cellColour, int extraSpacing, float angle)
{
  char s[2] = {chr, '\0'};
  Text(s, pt, font, colour, THA, TVA, BM, cellColour, extraSpacing, angle);
}

void DrawingSurface::Text(wchar_t chr, const Point &pt, const Font &font, const Colour &colour, 
  DrawingSurface::TextHAlign THA, DrawingSurface::TextVAlign TVA, DrawingSurface::BackgroundMode BM,
  const Colour &cellColour, int extraSpacing, float angle)
{
  wchar_t s[2] = {chr, '\0'};
  Text(s, pt, font, colour, THA, TVA, BM, cellColour, extraSpacing, angle);
}

void DrawingSurface::Line(Point end, const Point &start, const Pen &pen, const Colour &backColour, 
    BackgroundMode bm, RasterOp rasterOp)
{
  HDC hDC = m_dsData->hDC;
  if (&start) MoveToEx(hDC, start.x, start.y, NULL);
  HGDIOBJ oldPen;
  if (&pen) oldPen = SelectObject(hDC, (HPEN)pen.m_penData);

  COLORREF oldBC;
  if (&backColour) oldBC = SetBkColor(hDC, CVTCLR(backColour));

  int oldBM, oldROp;
  if (bm) oldBM = SetBkMode(hDC, bm);
  if (rasterOp) oldROp = SetROP2(hDC, rasterOp);
  
  LineTo(hDC, end.x, end.y);

  if (rasterOp) SetROP2(hDC, oldROp);
  if (bm) SetBkMode(hDC, oldBM);
  if (&backColour) SetBkColor(hDC, oldBC);
  if (&pen) SelectObject(hDC, oldPen);
}

void DrawingSurface::Polygon(const Point *vertex, int nVertices, const Pen &pen, const Brush &brush,
  const Colour &backColour, BackgroundMode bm, RasterOp rasterOp, FillMode fm)
{
  POINT *pt = (POINT *)_alloca(nVertices * sizeof(POINT));
  for (int i = 0; i < nVertices; i++)
  {
    pt[i].x = vertex[i].x;
    pt[i].y = vertex[i].y;
  }
  HDC hDC = m_dsData->hDC;
  HGDIOBJ oldPen, oldBrush;
  if (&pen) oldPen = SelectObject(hDC, (HPEN)pen.m_penData);
  if (&brush) oldBrush = SelectObject(hDC, (HPEN)brush.m_brushData);

  COLORREF oldBC;
  if (&backColour) oldBC = SetBkColor(hDC, CVTCLR(backColour));

  int oldBM, oldROp, oldfm;
  if (bm) oldBM = SetBkMode(hDC, bm);
  if (rasterOp) oldROp = SetROP2(hDC, rasterOp);
  if (fm) oldfm = SetPolyFillMode(hDC, fm);
  
  ::Polygon(hDC, pt, nVertices);

  if (fm) SetPolyFillMode(hDC, oldfm);
  if (rasterOp) SetROP2(hDC, oldROp);
  if (bm) SetBkMode(hDC, oldBM);
  if (&backColour) SetBkColor(hDC, oldBC);
  if (&brush) SelectObject(hDC, oldBrush);
  if (&pen) SelectObject(hDC, oldPen);
}

void DrawingSurface::Rectangle(const Rect &rect, const Pen &pen, const Brush &brush,
  const Colour &backColour, BackgroundMode bm, RasterOp rasterOp)
{
  RECT rect1;
  if (&rect)
  {
    rect1.left = rect.left; rect1.top = rect.top;
    rect1.right = rect.right; rect1.bottom = rect.bottom;
  }
  else rect1 = DefaultRect(m_dsData);
  HDC hDC = m_dsData->hDC;

  if (IsRectEmpty(Rect(rect1.left, rect1.top, rect1.right, rect1.bottom))) return;
  HGDIOBJ oldPen, oldBrush;
  if (&pen) oldPen = SelectObject(hDC, (HPEN)pen.m_penData);
  if (&brush) oldBrush = SelectObject(hDC, (HPEN)brush.m_brushData);

  COLORREF oldBC;
  if (&backColour) oldBC = SetBkColor(hDC, CVTCLR(backColour));

  int oldBM, oldROp;
  if (bm) oldBM = SetBkMode(hDC, bm);
  if (rasterOp) oldROp = SetROP2(hDC, rasterOp);
  
  if (GetCurrentObject(hDC, OBJ_PEN) == GetStockObject(NULL_PEN))
    FillRect(hDC, &rect1, (HBRUSH)GetCurrentObject(hDC, OBJ_BRUSH));
  else
    ::Rectangle(hDC, rect1.left, rect1.top, rect1.right, rect1.bottom);

  if (rasterOp) SetROP2(hDC, oldROp);
  if (bm) SetBkMode(hDC, oldBM);
  if (&backColour) SetBkColor(hDC, oldBC);
  if (&brush) SelectObject(hDC, oldBrush);
  if (&pen) SelectObject(hDC, oldPen);
}

void DrawingSurface::Ellipse(const Rect &boundingRect, const Pen &pen, const Brush &brush,
  const Colour &backColour, BackgroundMode bm, RasterOp rasterOp)
{
  HDC hDC = m_dsData->hDC;

  HGDIOBJ oldPen, oldBrush;
  if (&pen) oldPen = SelectObject(hDC, (HPEN)pen.m_penData);
  if (&brush) oldBrush = SelectObject(hDC, (HPEN)brush.m_brushData);

  COLORREF oldBC;
  if (&backColour) oldBC = SetBkColor(hDC, CVTCLR(backColour));

  int oldBM, oldROp;
  if (bm) oldBM = SetBkMode(hDC, bm);
  if (rasterOp) oldROp = SetROP2(hDC, rasterOp);
  
  ::Ellipse(hDC, boundingRect.left, boundingRect.top, boundingRect.right, boundingRect.bottom);

  if (rasterOp) SetROP2(hDC, oldROp);
  if (bm) SetBkMode(hDC, oldBM);
  if (&backColour) SetBkColor(hDC, oldBC);
  if (&brush) SelectObject(hDC, oldBrush);
  if (&pen) SelectObject(hDC, oldPen);
}

void DrawingSurface::BrushOrigin(int x, int y)
{
  SetBrushOrgEx(m_dsData->hDC, x, y, NULL);
}

bool DrawingSurface::IsRectEmpty(const Rect &rect) const
{
  HDC hDC = m_dsData->hDC;
  SIZE s1, s2;
  GetViewportExtEx(hDC, &s1);
  GetWindowExtEx(hDC, &s2);
  int w = rect.right - rect.left, h = rect.bottom - rect.top;
  if ((s1.cx < 0) ^ (s2.cx < 0)) w = -w;
  if ((s1.cy < 0) ^ (s2.cy < 0)) h = -h;
  return  h <= 0 || w <= 0;
}

void DrawingSurface::BeginPath() {::BeginPath(m_dsData->hDC);}

void DrawingSurface::EndPath() {::EndPath(m_dsData->hDC);}

void DrawingSurface::PolyBezier(const Point *point, int nPoints, const Pen &pen)
{
  HDC hDC = m_dsData->hDC;
  if (&pen)
  {
    HGDIOBJ hOldPen = SelectObject(hDC, (HPEN)pen.m_penData);
    ::PolyBezier(hDC, (POINT *)point, nPoints);
    SelectObject(hDC, hOldPen);
  }
  else ::PolyBezier(hDC, (POINT *)point, nPoints);
}

void DrawingSurface::PolyLine(const Point *point, int nPoints, const Pen &pen)
{
  HDC hDC = m_dsData->hDC;
  if (&pen)
  {
    HGDIOBJ hOldPen = SelectObject(hDC, (HPEN)pen.m_penData);
    ::Polyline(hDC, (POINT *)point, nPoints);
    SelectObject(hDC, hOldPen);
  }
  else ::Polyline(hDC, (POINT *)point, nPoints);
}

void DrawingSurface::FillPath(const Brush &brush)
{
  HDC hDC = m_dsData->hDC;
  if (&brush)
  {
    HGDIOBJ hOldBrush = SelectObject(hDC, (HBRUSH)brush.m_brushData);
    ::FillPath(hDC);
    SelectObject(hDC, hOldBrush);
  }
  else ::FillPath(hDC);
}

void DrawingSurface::SetCoords(const Rect &logicalRect, const Rect &deviceRect)
{
  HDC hDC = m_dsData->hDC;
  SetMapMode(hDC, MM_ANISOTROPIC);
  SetWindowExtEx(hDC, logicalRect.right - logicalRect.left,
    logicalRect.bottom - logicalRect.top, NULL);
  SetViewportExtEx(hDC, deviceRect.right - deviceRect.left,
    deviceRect.bottom - deviceRect.top, NULL);
  SetViewportOrgEx(hDC, deviceRect.left, deviceRect.top, NULL);
  SetWindowOrgEx(hDC, logicalRect.left, logicalRect.top, NULL);
}

Rect DrawingSurface::LogicalCoords(const Rect &deviceCoords) const
{
  Rect r = deviceCoords;
  DPtoLP(m_dsData->hDC, (LPPOINT)&r, 2);
  return r;
}

Point DrawingSurface::LogicalCoords(const Point &deviceCoords) const
{
  Point p = deviceCoords;
  DPtoLP(m_dsData->hDC, (LPPOINT)&p, 1);
  return p;
}

Rect DrawingSurface::DeviceCoords(const Rect &logicalCoords) const
{
  Rect r = logicalCoords;
  LPtoDP(m_dsData->hDC, (LPPOINT)&r, 2);
  return r;
}

Point DrawingSurface::DeviceCoords(const Point &logicalCoords) const
{
  Point p = logicalCoords;
  LPtoDP(m_dsData->hDC, (LPPOINT)&p, 1);
  return p;
}

Bitmap::Bitmap()
{
  m_bitmapData = new BitmapData;
  m_bitmapData->hBitmap = NULL;
  m_bitmapData->hPalette = NULL;
}

Bitmap::Bitmap(int resID)
{
  m_bitmapData = new BitmapData;
  m_bitmapData->hPalette = NULL;
  if (!(m_bitmapData->hBitmap = LoadBitmap(g_hInst, MAKEINTRESOURCE(resID))))
    throw "Resource not found";
}

Bitmap::Bitmap(const char *fileName)
{
  m_bitmapData = new BitmapData;
  m_bitmapData->hBitmap = LoadBitmapFile(&m_bitmapData->hPalette, fileName);
}

Bitmap::Bitmap(Bitmap::Std stdBM)
{
  m_bitmapData = new BitmapData;
  m_bitmapData->hPalette = NULL;
  if (stdBM == menuCheck) m_bitmapData->hBitmap = LoadBitmap(NULL, (LPCSTR)OBM_CHECK);
  else m_bitmapData->hBitmap = NULL;
}

Bitmap::Bitmap(const Bitmap &bm) // copy constructor
{
  m_bitmapData = new BitmapData;
  m_bitmapData->hBitmap = CloneBitmap(bm.m_bitmapData->hBitmap);
  HPALETTE hPal = bm.m_bitmapData->hPalette;
  if (hPal == GetStockObject(DEFAULT_PALETTE)) m_bitmapData->hPalette = NULL;
  else m_bitmapData->hPalette = ClonePalette(hPal);
}

Bitmap::Bitmap(const Bitmap &bm, const Rect &rect)
{
  m_bitmapData = new BitmapData;
  m_bitmapData->hBitmap = SubBitmap(bm.m_bitmapData->hBitmap, rect.left, rect.top, 
    rect.right - rect.left, rect.bottom - rect.top);
  HPALETTE hPal = bm.m_bitmapData->hPalette;
  if (hPal == GetStockObject(DEFAULT_PALETTE)) m_bitmapData->hPalette = NULL;
  else m_bitmapData->hPalette = ClonePalette(hPal);
}

Bitmap::Bitmap(int width, int height)
{
  m_bitmapData = new BitmapData;
  m_bitmapData->hPalette = NULL;
  HDC hDCscreen;
  if (!(hDCscreen = CreateIC("DISPLAY", NULL, NULL, NULL)) ||
    !(m_bitmapData->hBitmap = CreateCompatibleBitmap(hDCscreen, width, height)) ||
    !DeleteDC(hDCscreen)) throw WindObjError();
}

Bitmap::Bitmap(Point pt)
{
  Bitmap::Bitmap(pt.x, pt.y);
}

Bitmap::Bitmap(WindObjStream &ifs)
{
  HGLOBAL hDib = NULL;
  m_bitmapData = new BitmapData;
  try
  {
    m_bitmapData->hBitmap = BitmapFromPackedDIB(&m_bitmapData->hPalette, hDib = ReadPackedDIB(ifs));
  }
  catch (...) {if (hDib) GlobalFree(hDib); throw;}
  if (hDib) GlobalFree(hDib);
}
  
Bitmap::~Bitmap()
{
  if (m_bitmapData->hBitmap) DeleteObject(m_bitmapData->hBitmap);
  if (m_bitmapData->hPalette) DeleteObject(m_bitmapData->hPalette);
  delete m_bitmapData;
}

Bitmap &Bitmap::operator=(const Bitmap &bm)
{
  this->Bitmap::~Bitmap();
  this->Bitmap::Bitmap(bm);
  return *this;
}

Bitmap& Bitmap::operator=(int resID)
{
  this->Bitmap::~Bitmap();
  this->Bitmap::Bitmap(resID);
  return *this;
}

Bitmap& Bitmap::operator=(const char *fileName)
{
  this->Bitmap::~Bitmap();
  this->Bitmap::Bitmap(fileName);
  return *this;
}

Bitmap& Bitmap::operator=(Bitmap::Std stdBM)
{
  this->Bitmap::~Bitmap();
  this->Bitmap::Bitmap(stdBM);
  return *this;
}

Bitmap& Bitmap::operator=(Point pt)
{
  this->Bitmap::~Bitmap();
  this->Bitmap::Bitmap(pt);
  return *this;
}

Bitmap& Bitmap::operator=(WindObjStream &ifs)
{
  this->Bitmap::~Bitmap();
  this->Bitmap::Bitmap(ifs);
  return *this;
}

bool Bitmap::IsNull() {return !m_bitmapData->hBitmap;}

Rect Bitmap::Extent()
{
  BITMAP bm;

  if (!m_bitmapData->hBitmap) return Rect(0, 0, 0, 0);
  if (!GetObject(m_bitmapData->hBitmap, sizeof(BITMAP), &bm))
    throw WindObjError();
  return Rect(0, 0, bm.bmWidth, bm.bmHeight);
}

bool Bitmap::operator==(const Bitmap &bm)
{
  if (!PaletteComp(m_bitmapData->hPalette, bm.m_bitmapData->hPalette)) return true;
  return !BitmapComp(m_bitmapData->hBitmap, bm.m_bitmapData->hBitmap);
}

bool Bitmap::operator!=(const Bitmap &bm)
{
  return !(*this == bm);
}

void DrawingSurface::DrawBitmap(Point topLeft, const Bitmap &bm)
{
  BITMAP bms;

  GetObject(bm.m_bitmapData->hBitmap, sizeof(BITMAP), (void *)&bms);
  RECT b = {0, 0, bms.bmWidth, bms.bmHeight};
  DPtoLP(m_dsData->hDC, (POINT *)&b, 2);
  DrawBitmap(topLeft, bm, Rect(0, 0, b.right - b.left, b.bottom - b.top));
}

void DrawingSurface::DrawBitmap(Point topLeft, const Bitmap &bm, const Bitmap &mask)
{
  DSData *dsd = (DSData *)m_dsData;
  BITMAP bms;

  GetObject(bm.m_bitmapData->hBitmap, sizeof(BITMAP), (void *)&bms);
  HDC hDCmem = CreateCompatibleDC(NULL);
  HPALETTE hPal = bm.m_bitmapData->hPalette;
  if (hPal) SelectPalette(dsd->hDC, hPal, 0);
  SetMapMode(hDCmem, GetMapMode(dsd->hDC));
  SelectObject(hDCmem, mask.m_bitmapData->hBitmap);
  HBRUSH hPattBrush = CreatePatternBrush(bm.m_bitmapData->hBitmap);
  HGDIOBJ hOldBrush = SelectObject(dsd->hDC, hPattBrush);
  POINT tl = {topLeft.x, topLeft.y}, pt;
  LPtoDP(dsd->hDC, &tl, 1);
  SetBrushOrgEx(dsd->hDC, tl.x, tl.y, &pt);
  if (!BitBlt(dsd->hDC, topLeft.x, topLeft.y, bms.bmWidth, bms.bmHeight, hDCmem, 0, 0, 0xE20746))
    throw WindObjError();
  SelectObject(dsd->hDC, hOldBrush);
  SetBrushOrgEx(dsd->hDC, pt.x, pt.y, NULL);
  DeleteObject(hPattBrush);
  DeleteDC(hDCmem);
}

void DrawingSurface::DrawBitmap(Point topLeft, const Bitmap &bm, Rect bmRect)
{
  DSData *dsd = (DSData *)m_dsData;

  HDC hDCmem = CreateCompatibleDC(dsd->hDC);
  HPALETTE hPal = bm.m_bitmapData->hPalette;
  if (hPal) SelectPalette(dsd->hDC, hPal, 0);
  SetMapMode(hDCmem, GetMapMode(dsd->hDC));
  RECT b = {bmRect.left, bmRect.top, bmRect.right, bmRect.bottom};
  SelectObject(hDCmem, bm.m_bitmapData->hBitmap);
  if (!BitBlt(dsd->hDC, topLeft.x, topLeft.y, b.right - b.left, b.bottom - b.top, 
    hDCmem, b.left, b.top, SRCCOPY)) throw WindObjError();
  DeleteDC(hDCmem);
}

static const LPCSTR s_stdCursor[] = 
{
  IDC_ARROW,
  IDC_SIZEWE,
  IDC_APPSTARTING,
  IDC_SIZENESW,
  IDC_SIZENS,
  IDC_SIZENWSE,
  IDC_HELP,
  IDC_CROSS,
  IDC_SIZEALL,
  IDC_WAIT,
  IDC_IBEAM,
  IDC_ICON,
  IDC_NO,
  IDC_SIZE,
  IDC_UPARROW
};

#define s_nStdCursors (sizeof(s_stdCursor) / sizeof(LPCTSTR))

Cursor::Cursor(Std sc)
{
  int i = (int)sc;
  if (i < 0 || i >= s_nStdCursors) i = hourGlass;
  if (!(m_cursorData = (void *)LoadCursor(NULL, s_stdCursor[i])))
    throw WindObjError();
}

Cursor::Cursor(unsigned short resID)
{
  if (!(m_cursorData = LoadCursor(g_hInst, MAKEINTRESOURCE(resID))))
    throw WindObjError();
}

Cursor::Cursor(const char *resname)
{
  if (!(m_cursorData = (void *)LoadCursor(g_hInst, resname)))
    throw WindObjError();
}

Cursor::Cursor(const Cursor &cr)
{
  if (!(m_cursorData = CopyCursor(cr.m_cursorData)))
    throw WindObjError();
}

Cursor &Cursor::operator=(const Cursor &cr)
{
  if (!DestroyCursor((HCURSOR)m_cursorData) || 
    !(m_cursorData = CopyCursor(cr.m_cursorData)))
    throw WindObjError();
  return *this;
}

void Cursor::Set()
{
  SetCursor((HCURSOR)m_cursorData);
}

HBRUSH p_backgroundBrush;

Brush::Brush()
{
  if (!(m_brushData = GetStockObject(NULL_BRUSH)))
    throw WindObjError();
}

Brush::Brush(const Colour &clr)
{
  if (!(m_brushData = CreateSolidBrush(CVTCLR(clr))))
    throw WindObjError();
}

Brush::Brush(const Colour::Std sc)
{
  if (!(m_brushData = CreateSolidBrush(CVTCLR(Colour(sc)))))
    throw WindObjError();
}

Brush::Brush(const Bitmap &bm)
{
  if (bm.m_bitmapData->hPalette)
  {
    HGLOBAL hDib = PackedDIBFromBitmap(bm.m_bitmapData->hBitmap, bm.m_bitmapData->hPalette);
    m_brushData = CreateDIBPatternBrushPt(GlobalLock(hDib), DIB_RGB_COLORS);
    GlobalFree(hDib);
  }
  else m_brushData = CreatePatternBrush(bm.m_bitmapData->hBitmap);
}

Brush::~Brush()
{
  DeleteObject(m_brushData);
}

Brush::Brush(const Brush &br)
{
  m_brushData = CloneBrush((HBRUSH)br.m_brushData);
}

Brush& Brush::operator=(const Brush &br)
{
  DeleteObject(m_brushData);
  m_brushData = CloneBrush((HBRUSH)br.m_brushData);
  return *this;
}

Brush& Brush::operator=(const Colour &clr)
{
  DeleteObject(m_brushData);
  this->Brush::Brush(clr);
  return *this;
}

Brush& Brush::operator=(const Colour::Std sc)
{
  DeleteObject(m_brushData);
  this->Brush::Brush(sc);
  return *this;
}

Brush& Brush::operator=(const Bitmap &bm)
{
  DeleteObject(m_brushData);
  this->Brush::Brush(bm);
  return *this;
}

Pen::Pen()
{
  m_penData = GetStockObject(NULL_PEN);
}

Pen::Pen(const Colour &clr, int width, Style style, Pen::EndCap endCap, Pen::Join join)
{
  if (style == Pen::solid && endCap == Pen::roundEnd && join == Pen::roundJoin)
    m_penData = (void *)CreatePen(style, width, CVTCLR(clr));
  else
  {
    DWORD penStyle = PS_GEOMETRIC | (int)style;
    switch (endCap)
    {
    case roundEnd:  penStyle |= PS_ENDCAP_ROUND; break;
    case squareEnd: penStyle |= PS_ENDCAP_SQUARE; break;
    case flatEnd:   penStyle |= PS_ENDCAP_FLAT; break;
    } 
    switch (join)
    {
    case bevelJoin: penStyle |= PS_JOIN_BEVEL; break;
    case mitreJoin: penStyle |= PS_JOIN_MITER; break;
    case roundJoin: penStyle |= PS_JOIN_ROUND; break;
    }
    LOGBRUSH lb = {BS_SOLID, CVTCLR(clr)};
    m_penData = (void *)ExtCreatePen(penStyle, width, &lb, 0, NULL);
  }
}

Pen::Pen(const Colour::Std sc, int width, Pen::Style style, Pen::EndCap endCap, Pen::Join join)
{
  this->Pen::Pen(Colour(sc), width, style, endCap, join);
}

Pen::Pen(const Pen &pen)
{
  m_penData = ClonePen((HPEN)pen.m_penData);
}

Pen::~Pen()
{
  DeleteObject((HPEN)m_penData);
}

Pen &Pen::operator=(const Pen &pen)
{
  DeleteObject(m_penData);
  m_penData = ClonePen((HPEN)pen.m_penData);
  return *this;
}

Icon::Icon(unsigned short resID)
{
  if (!(m_iconData = LoadIcon(g_hInst, MAKEINTRESOURCE(resID))))
    throw WindObjError();
}

Icon::Icon(const char *name)
{
  if (!(m_iconData = (void *)LoadIcon(g_hInst, name)))
    throw WindObjError();
}

Icon::Icon(const Icon &icon)
{
  m_iconData = CloneIcon((HICON)icon.m_iconData);
}

Icon &Icon::operator=(const Icon &icon)
{
  Icon::~Icon();
  m_iconData = CloneIcon((HICON)icon.m_iconData);
  return *this;
}

Icon::~Icon()
{
  if (m_iconData)
    if (!DestroyIcon((HICON)m_iconData)) throw WindObjError();
}

bool Icon::operator==(const Icon &icon)
{
  if (m_iconData == icon.m_iconData) return true;
  if (!m_iconData || !icon.m_iconData) return false;
  ICONINFO info1, info2;
  GetIconInfo((HICON)m_iconData, &info1);
  GetIconInfo((HICON)icon.m_iconData, &info2);
  if (BitmapComp(info1.hbmMask, info2.hbmMask)) return false;
  return !BitmapComp(info1.hbmColor, info2.hbmColor);
}

bool Icon::operator!=(const Icon &icon)
{
  return !(*this == icon);
}

Font::Font(Std stdfont)
{
  int fontID;
  switch (stdfont)
  {
  case variable: fontID = ANSI_VAR_FONT;   break;
  case fixed:    fontID = ANSI_FIXED_FONT; break;
  default:       fontID = SYSTEM_FONT;     break;
  }
  m_fontData = GetStockObject(fontID);
}

Font::Font(char *faceName, int size, short weight, short charSet, bool isItalic, 
    bool isUnderlined, bool isStruckThrough)
{
  m_fontData = (void *)CreateFont(size, 0, 0, 0, weight, isItalic? TRUE: FALSE, isUnderlined? TRUE: FALSE, 
    isStruckThrough? TRUE: FALSE, charSet, 0, 0, 0, 0, faceName);
  if (!m_fontData) throw WindObjError();
}

Font::~Font()
{
  DeleteObject((HFONT)m_fontData);
}

Font::Font(const Font &font)
{
  m_fontData = CloneFont((HFONT)font.m_fontData);
}

Font &Font::operator=(const Font &font)
{
  DeleteObject(m_fontData);
  m_fontData = CloneFont((HFONT)font.m_fontData);
  return *this;
}

inline int IntAbs(int x) {return x >= 0? x: -x;}

bool Font::operator==(const Font &font) const
{
  if (this == &font) return true;
  if (!this || !&font) return false;
  LOGFONT logFont1, logFont2;
  if (!GetObject((HFONT)m_fontData, sizeof(LOGFONT), &logFont1))
    throw WindObjError();
  if (!GetObject((HFONT)font.m_fontData, sizeof(LOGFONT), &logFont2))
    throw WindObjError();
  return !strcmp(logFont1.lfFaceName, logFont1.lfFaceName) &&
    IntAbs(logFont1.lfHeight) == IntAbs(logFont2.lfHeight) &&
    logFont1.lfWeight == logFont2.lfWeight &&
    logFont1.lfItalic == logFont2.lfItalic &&
    logFont1.lfUnderline == logFont2.lfUnderline &&
    logFont1.lfStrikeOut == logFont2.lfStrikeOut;
}

int Font::FaceName(char *buff) const
{
  LOGFONT logFont;
  if (!GetObject((HFONT)m_fontData, sizeof(LOGFONT), &logFont))
    throw WindObjError();
  if (buff) strcpy(buff, logFont.lfFaceName);
  return strlen(logFont.lfFaceName);
}

int Font::LogicalHeight()
{
  LOGFONT logFont;
  if (!GetObject((HFONT)m_fontData, sizeof(LOGFONT), &logFont))
    throw WindObjError();
  return logFont.lfHeight;
}

float Font::Size() const
{
  LOGFONT logFont;
  if (!GetObject((HFONT)m_fontData, sizeof(LOGFONT), &logFont))
    throw WindObjError();
  HDC hScreenIC = CreateIC("DISPLAY", NULL, NULL, NULL);
  float size = 72.f * logFont.lfHeight / GetDeviceCaps(hScreenIC, LOGPIXELSY);
  DeleteDC(hScreenIC);
  return size < 0? -size: size;
}

const char *FontWeightText(short fontWeight)
{
  const char *weightText[] =
    {"Thin", "Extra Light", "Light", "", "Medium", "Semi-Bold", "Bold", "Extra Bold", "Heavy"};
  int weightNo = (fontWeight + 50) / 100;
  if (weightNo < 1) return "";
  if (weightNo > 9) return "Black";
  return weightText[weightNo - 1];
}

short Font::Weight() const
{
  LOGFONT logFont;
  if (!GetObject((HFONT)m_fontData, sizeof(LOGFONT), &logFont))
    throw WindObjError();
  return (short)logFont.lfWeight;
}

short Font::CharSet() const
{
  LOGFONT logFont;
  if (!GetObject((HFONT)m_fontData, sizeof(LOGFONT), &logFont))
    throw WindObjError();
  return logFont.lfCharSet;
}

bool Font::IsItalic() const
{
  LOGFONT logFont;
  if (!GetObject((HFONT)m_fontData, sizeof(LOGFONT), &logFont))
    throw WindObjError();
  return logFont.lfItalic != 0;
}

bool Font::IsUnderlined() const
{
  LOGFONT logFont;
  if (!GetObject((HFONT)m_fontData, sizeof(LOGFONT), &logFont))
    throw WindObjError();
  return logFont.lfUnderline != 0;
}

bool Font::IsStruckThrough() const
{
  LOGFONT logFont;
  if (!GetObject((HFONT)m_fontData, sizeof(LOGFONT), &logFont))
    throw WindObjError();
  return logFont.lfStrikeOut != 0;
}

int Font::Description(char *buff) const
{
  LOGFONT logFont;
  if (!GetObject((HFONT)m_fontData, sizeof(LOGFONT), &logFont))
    throw WindObjError();
  char buf1[256], *ptr = buff? buff: buf1, *ptr0 = ptr;
  HDC hScreenIC = CreateIC("DISPLAY", NULL, NULL, NULL);
  float size = 72.f * logFont.lfHeight / GetDeviceCaps(hScreenIC, LOGPIXELSY);
  DeleteDC(hScreenIC);
  if (size < 0) size = -size;
  sprintf(ptr, "%s %g pt", logFont.lfFaceName, size);
  ptr += strlen(ptr);
  const char *fontWeightText = FontWeightText((short)logFont.lfWeight);
  if (fontWeightText)
  {
    *ptr++ = ' ';
    strcpy(ptr, fontWeightText);
    ptr += strlen(ptr);
  }
  if (logFont.lfItalic)
  {
    strcpy(ptr, " Italic");
    ptr += strlen(ptr);
  }
  if (logFont.lfUnderline)
  {
    strcpy(ptr, " Underlined");
    ptr += strlen(ptr);
  }
  if (logFont.lfStrikeOut)
  {
    strcpy(ptr, " StrikeOut");
    ptr += strlen(ptr);
  }
  return ptr - ptr0;
}

int Font::TextHeight()
{
  return ::TextHeight(this? (HFONT)m_fontData: (HFONT)GetStockObject(SYSTEM_FONT));
}

int Font::TextWidth(const char *text)
{
  return text? TextWidth(text, strlen(text)): 0;
}

int Font::TextWidth(const wchar_t *text)
{
  return text? TextWidth(text, wcslen(text)): 0;
}

int Font::TextWidth(const char *text, int nText)
{
  return ::TextWidth(this? (HFONT)m_fontData: (HFONT)GetStockObject(SYSTEM_FONT), text, nText);
}

int Font::TextWidth(const wchar_t *text, int nText)
{
  return ::TextWidth(this? (HFONT)m_fontData: (HFONT)GetStockObject(SYSTEM_FONT), text, nText);
}

Font *NewFont(Font::Std stdFont) {return new Font(stdFont);}
Font *NewFont(char *faceName, int height) {return new Font(faceName, height);}
Font *NewFont(const Font &font) {return new Font(font);}

bool ConfigureFont(Font &font, Colour &fontColour, Window *parent)
{
  LOGFONT logFont;
  CHOOSEFONT chooseFont;
  chooseFont.lStructSize = sizeof(CHOOSEFONT);
  chooseFont.hwndOwner = parent? parent->m_windowData->hWnd: NULL;
  chooseFont.hDC = NULL;
  chooseFont.lpLogFont = &logFont;
  chooseFont.iPointSize = 0;
  chooseFont.Flags = CF_EFFECTS | CF_FORCEFONTEXIST | CF_SCREENFONTS;
  if (font.m_fontData)
  {
    if (!GetObject(font.m_fontData, sizeof(LOGFONT), &logFont))
      throw WindObjError();
    chooseFont.Flags |= CF_INITTOLOGFONTSTRUCT;
  }
  chooseFont.rgbColors = CVTCLR(fontColour);
  chooseFont.lCustData = NULL;
  chooseFont.lpfnHook = NULL;
  chooseFont.lpTemplateName = NULL;
  chooseFont.hInstance = NULL;
  chooseFont.lpszStyle = NULL;
  if (!ChooseFont(&chooseFont)) return false;
  if (font.m_fontData) DeleteObject(font.m_fontData);
  font.m_fontData = (void *)CreateFontIndirect(&logFont);
  fontColour.red = GetRValue(chooseFont.rgbColors);
  fontColour.green = GetGValue(chooseFont.rgbColors);
  fontColour.blue = GetBValue(chooseFont.rgbColors);
  return true;
}
