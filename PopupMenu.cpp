#define WINDOBJ_MAIN

#include <WindObj/PopupMenu.h>
#include <WindObj/Private.h>

#include <limits.h>
#include <string.h>
#include <typeinfo.h>
#include <windows.h>

// MenuMW

void MenuMW::MouseLeft()
{
  if (IsHighlighted()) Highlight(false);
}

void MenuMW::MouseMoved(int x, int y)
{
  if (!IsHighlighted()) Highlight(true);
}

bool MenuMW::IsTextMenuItemMW() const {return false;}

bool MenuMW::IsEnabled() const {return false;}

void MenuMW::Highlight(bool) {}
bool MenuMW::IsHighlighted() const {return false;}

// SeparatorMW

void SeparatorMW::CalcSize()
{
  m_rect.bottom = m_rect.top + 4;
  m_rect.right = m_rect.left + 10;
}

void SeparatorMW::Exposed(DrawingSurface &DS, const Rect &invRect)
{
  DS.Rectangle(m_rect, Pen(), Brush(m_backColour));
  DS.Line(Point(m_rect.left, m_rect.top + 2), Point(m_rect.right, m_rect.top + 2), Pen(m_foreColour));
}

WO_EXPORT SeparatorMW *NewSeparatorMW(MiniWindowContainer *parent, Colour foreColour, Colour backColour)
{
  return new SeparatorMW(parent, foreColour, backColour);
}

// TextMenuItemMW

TextMenuItemMW::TextMenuItemMW(MiniWindowContainer *parent, const char *text, bool isEnabled, Bitmap *bitmap, 
  short bitmapSpace, Font *font, Colour foreColour, Colour backColour, Colour highlightColour, 
  Colour highlightTextColour) : 
  MenuMW(parent, 0, 0, 10, 10), m_isEnabled(isEnabled), m_bitmap(bitmap), m_bitmapSpace(bitmapSpace), 
  m_foreColour(foreColour), m_backColour(backColour), m_highlightColour(highlightColour), 
  m_highlightTextColour(highlightTextColour), m_isHighlighted(false)
{
  if (!font) m_font = NewFont(Font::variable);
  else m_font = font;
  m_text = text? strcpy(new char[strlen(text) + 1], text): NULL;
}

TextMenuItemMW::TextMenuItemMW(const TextMenuItemMW &textMW) : 
  MenuMW(textMW.m_parent, textMW.m_rect.left, textMW.m_rect.top, textMW.m_rect.right, textMW.m_rect.bottom) 
{
  m_text = textMW.m_text? strcpy(new char[strlen(textMW.m_text) + 1], textMW.m_text): NULL;
  m_isEnabled = textMW.m_isEnabled;
  m_bitmap = textMW.m_bitmap;
  m_bitmapSpace = textMW.m_bitmapSpace;
  m_font = NewFont(*textMW.m_font);
  m_foreColour = textMW.m_foreColour;
  m_backColour = textMW.m_backColour;
  m_highlightColour = textMW.m_highlightColour;
  m_highlightTextColour = textMW.m_highlightTextColour;
  m_isHighlighted = textMW.m_isHighlighted;
}

TextMenuItemMW::~TextMenuItemMW()
{
  delete [] m_text;
  delete m_font;
}

void TextMenuItemMW::CalcSize()
{
  m_rect.bottom = m_rect.top + m_font->TextHeight();
  const char *tabPos = strchr(m_text, '\t');
  if (tabPos)
  {
    m_rect.right = m_rect.left + m_bitmapSpace + m_font->TextWidth(m_text, tabPos - m_text) +
      4 + m_font->TextWidth(tabPos + 1) + 10;
  }
  else  m_rect.right = m_rect.left + m_bitmapSpace + m_font->TextWidth(m_text) + 10;
}

void TextMenuItemMW::Exposed(DrawingSurface &DS, const Rect &invRect)
{
  DS.Set(Brush(m_isHighlighted? m_highlightColour: m_backColour));
  DS.Set(Pen());
  DS.Set(*m_font);
  DS.Rectangle(m_rect);
  if (m_bitmap && m_bitmapSpace) DS.DrawBitmap(m_rect.TopLeft() + 
      Point(1, (m_rect.bottom - m_rect.top - m_bitmap->Extent().bottom) / 2), *m_bitmap);
  char *tabPos = strchr(m_text, '\t');
  if (tabPos) *tabPos = '\0';
  Colour textColour = m_isHighlighted? m_highlightTextColour: m_foreColour;
  if (!m_isEnabled) textColour = Colour(textColour, m_backColour);
  DS.Text(m_text, m_rect.TopLeft() + Point(m_bitmapSpace, 0), DefFont, textColour);
  if (tabPos)
  {
    *tabPos++ = '\t';
    DS.Text(tabPos, Point(m_rect.right - 10 - m_font->TextWidth(tabPos), m_rect.top), DefFont, textColour);
  }
}

bool TextMenuItemMW::IsEnabled() const {return m_isEnabled;}

bool TextMenuItemMW::IsTextMenuItemMW() const {return true;}

void TextMenuItemMW::Highlight(bool highlight)
{
  m_isHighlighted = highlight;
  Draw();
}

bool TextMenuItemMW::IsHighlighted() const
{
  return m_isHighlighted;
}

TextMenuItemMW *NewTextMenuItemMW(MiniWindowContainer *parent, const char *text, bool isEnabled, Bitmap *bitmap, 
  short bitmapSpace, Font *font, Colour foreColour, Colour backColour, Colour highlightColour, Colour highlightTextColour)
{
  return new TextMenuItemMW(parent, text, isEnabled, bitmap, bitmapSpace, font, foreColour, backColour, 
    highlightColour, highlightTextColour);
}

// BitmapMenuItemMW

BitmapMenuItemMW::BitmapMenuItemMW(MiniWindowContainer *parent, Bitmap *image, bool isEnabled, 
  Bitmap *checkBitmap, short checkBitmapSpace, Colour backColour, Colour highlightColour) : 
  MenuMW(parent, 0, 0, 10, 10), m_isEnabled(isEnabled), m_checkBitmap(checkBitmap), m_checkBitmapSpace(checkBitmapSpace), 
  m_backColour(backColour), m_highlightColour(highlightColour), m_isHighlighted(false)
{
  m_image = new Bitmap(*image);
}

BitmapMenuItemMW::BitmapMenuItemMW(const BitmapMenuItemMW &bmMW) : 
  MenuMW(bmMW.m_parent, bmMW.m_rect.left, bmMW.m_rect.top, bmMW.m_rect.right, bmMW.m_rect.bottom) 
{
  m_image = new Bitmap(*bmMW.m_image);
  m_isEnabled = bmMW.m_isEnabled;
  m_checkBitmap = bmMW.m_checkBitmap;
  m_checkBitmapSpace = bmMW.m_checkBitmapSpace;
  m_backColour = bmMW.m_backColour;
  m_highlightColour = bmMW.m_highlightColour;
  m_isHighlighted = bmMW.m_isHighlighted;
}

BitmapMenuItemMW::~BitmapMenuItemMW()
{
  delete m_image;
}

void BitmapMenuItemMW::CalcSize()
{
  Rect extent = m_image->Extent();
  m_rect.bottom = m_rect.top + extent.bottom + 4;
  m_rect.right = m_rect.left + m_checkBitmapSpace + extent.right + 14;
}

void BitmapMenuItemMW::Exposed(DrawingSurface &DS, const Rect &invRect)
{
  DS.Set(Brush(m_isHighlighted && m_isEnabled? m_highlightColour: m_backColour));
  DS.Set(Pen());
  DS.Rectangle(m_rect);
  if (m_checkBitmap && m_checkBitmapSpace) DS.DrawBitmap(m_rect.TopLeft() + 
      Point(1, (m_rect.bottom - m_rect.top - m_checkBitmap->Extent().bottom) / 2), *m_checkBitmap);
  DS.DrawBitmap(m_rect.TopLeft() + Point(m_checkBitmapSpace + 2, 2), *m_image);
}

bool BitmapMenuItemMW::IsEnabled() const {return m_isEnabled;}

void BitmapMenuItemMW::Highlight(bool highlight)
{
  m_isHighlighted = highlight;
  Draw();
}

bool BitmapMenuItemMW::IsHighlighted() const
{
  return m_isHighlighted;
}

BitmapMenuItemMW *NewBitmapMenuItemMW(MiniWindowContainer *parent, Bitmap *image, bool isEnabled,
  Bitmap *checkBitmap, short checkBitmapSpace, Colour backColour, Colour highlightColour)
{
  return new BitmapMenuItemMW(parent, image, isEnabled, checkBitmap, checkBitmapSpace, backColour, highlightColour);
}

// CascadeMenuItemMW

CascadeMenuItemMW::CascadeMenuItemMW(MiniWindowContainer *parent, const char *text, PopupMenu *cascadeMenu, bool isEnabled, 
  Bitmap *bitmap, short bitmapSpace, Font *font, Colour foreColour, Colour backColour,
  Colour highlightColour, Colour highlightTextColour) : m_cascadeMenu(cascadeMenu),
    TextMenuItemMW(parent, text, isEnabled, bitmap, bitmapSpace, font, foreColour, backColour,
      highlightColour, highlightTextColour) {}

CascadeMenuItemMW::~CascadeMenuItemMW()
{
  delete m_cascadeMenu;
}

void CascadeMenuItemMW::CalcSize()
{
  TextMenuItemMW::CalcSize();
  m_rect.right += m_font->TextHeight();
}

void CascadeMenuItemMW::Exposed(DrawingSurface &DS, const Rect &invRect)
{
  DS.Set(Brush(m_isHighlighted? m_highlightColour: m_backColour));
  DS.Set(Pen());
  DS.Set(*m_font);
  DS.Rectangle(m_rect);
  if (m_bitmap && m_bitmapSpace) DS.DrawBitmap(m_rect.TopLeft() + 
      Point(1, (m_rect.bottom - m_rect.top - m_bitmap->Extent().bottom) / 2), *m_bitmap);
  char *tabPos = strchr(m_text, '\t');
  if (tabPos) *tabPos = '\0';
  Colour textColour = m_isHighlighted? m_highlightTextColour: m_foreColour;
  if (!m_isEnabled) textColour = Colour(textColour, m_backColour);
  DS.Text(m_text, m_rect.TopLeft() + Point(m_bitmapSpace, 0), DefFont, textColour);
  int h = m_font->TextHeight();
  if (tabPos)
  {
    *tabPos++ = '\t';
    DS.Text(tabPos, Point(m_rect.right - 10 - m_font->TextWidth(tabPos) - h, m_rect.top), DefFont, textColour);
  }
  Point cascadeArrow[3];
  int x1 = m_rect.right - h / 2, hh = h / 6;
  cascadeArrow[0] = Point(x1, m_rect.top + hh);
  cascadeArrow[1] = Point(x1, m_rect.top + 5 * hh);
  cascadeArrow[2] = Point(m_rect.right - h / 6, m_rect.top + 3 * hh);
  DS.Polygon(cascadeArrow, 3, Pen(), Brush(textColour));
}

bool CascadeMenuItemMW::IsTextMenuItemMW() const {return false;}

void CascadeMenuItemMW::Highlight(bool highlight)
{
  if (!m_isEnabled) return;
  if (highlight && !m_isHighlighted)
  {
    Rect b = m_cascadeMenu->BoundingRect();
    Point cascadeSize = b.BottomRight() - b.TopLeft();
    Point cascadePos = m_parent->ClientOrigin() + m_rect.TopRight();
    if (cascadePos.x + cascadeSize.x > Screen.width)
    {
      cascadePos.x = m_parent->ClientOrigin().x + m_rect.left - cascadeSize.x;
    }
    if (cascadePos.y + cascadeSize.y > Screen.height)
    {
      cascadePos.y = Screen.height - cascadeSize.y;
    }
    m_cascadeMenu->Move(cascadePos.x, cascadePos.y);
    m_cascadeMenu->ShowWithoutActivating();
    ((PopupMenu *)m_parent)->m_cascadeMenuItemMW = this;
  }
  else if (!highlight && m_isHighlighted)
  {
    ((PopupMenu *)m_parent)->m_cascadeMenuItemMW = NULL;
    m_cascadeMenu->Hide();
  }
  else return;
  m_isHighlighted = highlight;
  Draw();
}

// PopupMenu

SeparatorMW *PopupMenu::SetMenuItem(int index, Colour foreColour, Colour backColour)
{
  return (SeparatorMW *)(m_MW[index] = new SeparatorMW(this, foreColour, backColour));
}

TextMenuItemMW *PopupMenu::SetMenuItem(int index, const char *text, bool isEnabled, Bitmap *bitmap, short bitmapSpace,
  Font *font, Colour foreColour, Colour backColour, Colour highlightColour, Colour highlightTextColour) 
{
  return (TextMenuItemMW *)(m_MW[index] = new TextMenuItemMW(this, text, isEnabled, bitmap, bitmapSpace,
    font, foreColour, backColour, highlightColour, highlightTextColour));
}

BitmapMenuItemMW *PopupMenu::SetMenuItem(int index, Bitmap *image, bool isEnabled,
  Bitmap *checkBitmap, short checkBitmapSpace, Colour backColour, Colour highlightColour)
{
  return (BitmapMenuItemMW *)(m_MW[index] = new BitmapMenuItemMW(this, image, isEnabled, checkBitmap, checkBitmapSpace, backColour, highlightColour));
}

CascadeMenuItemMW *PopupMenu::SetMenuItem(int index, const char *text, PopupMenu *cascadeMenu, bool isEnabled, Bitmap *bitmap, 
  short bitmapSpace, Font *font, Colour foreColour, Colour backColour, 
  Colour highlightColour, Colour highlightTextColour)
{
  return (CascadeMenuItemMW *)(m_MW[index] = new CascadeMenuItemMW(this, text, cascadeMenu, isEnabled, bitmap,
    bitmapSpace, font, foreColour, backColour, highlightColour, highlightTextColour));
}

static inline bool IsEnabledTextMW(MiniWindow *MW)
{
  TextMenuItemMW *textMW = (TextMenuItemMW *)MW;
  return textMW->IsTextMenuItemMW() && textMW->m_isEnabled;
}

void ClosePopup(PopupMenu *popupMenu)
{
  int index;
  MenuMW *mouseMW = (MenuMW *)popupMenu->m_mouseMW;
  if (mouseMW && mouseMW->IsEnabled())
  {
    mouseMW->Highlight(false);
    index = popupMenu->MiniWindowAtPoint(mouseMW->m_rect.left, mouseMW->m_rect.top);
  }
  else index = -1;
  popupMenu->Hide();
  popupMenu->ReleaseMouseCapture();
  if (index < 0 || strcmp(typeid(*popupMenu->m_MW[index]).name(), "struct CascadeMenuItemMW"))
    popupMenu->SelectionMade(index, (IsKeyDown(VKC_shift)? 1: 0) | (IsKeyDown(VKC_control)? 2: 0));
}

static int NearestTextMenuMW(PopupMenu *popupMenu, int idx)
{
  if (IsEnabledTextMW(popupMenu->m_MW[idx])) return idx;
  if (idx + 1 < popupMenu->m_nMWs && IsEnabledTextMW(popupMenu->m_MW[idx + 1]) && 
    popupMenu->m_MW[idx]->m_rect.left == popupMenu->m_MW[idx + 1]->m_rect.left)
      return idx + 1; 
  bool goUp = true;
  for (int i = idx - 1, j = 2;; j++, goUp = !goUp)
  {
    if (i >= 0 && i < popupMenu->m_nMWs && IsEnabledTextMW(popupMenu->m_MW[i])) return i;
    i = goUp? i + j: i - j;
  }
}

void PopupMenu::KeyPressed(VirtualKeyCode vkCode, char ascii, unsigned short repeat)
{
  if (vkCode == VKC_escape || vkCode == VKC_return)
  {
    if (vkCode == VKC_escape) m_mouseMW = NULL;
    ClosePopup(this);
    return;
  }
  if (vkCode != VKC_up && vkCode != VKC_down && vkCode != VKC_left && vkCode != VKC_right) return;
  int idx = m_mouseMW? MiniWindowAtPoint(m_mouseMW->m_rect.left, m_mouseMW->m_rect.top): -1;
  if (idx >= 0)
  {
    ((MenuMW *)m_mouseMW)->Highlight(false);
    m_mouseMW->Draw();
  }
  if (vkCode == VKC_up)
  {
    for (idx--; idx >= 0 && !IsEnabledTextMW(m_MW[idx]); idx--);
    if (idx < 0) idx = m_nMWs - 1;
  }
  else if (vkCode == VKC_down)
  {
    for (idx++; idx < m_nMWs && !IsEnabledTextMW(m_MW[idx]); idx++);
    if (idx >= m_nMWs) idx = 0;
  }
  else if (vkCode == VKC_right)
  {
    if (idx < 0) idx = 0;
    else
    {
      int midLevel = (m_mouseMW->m_rect.top + m_mouseMW->m_rect.bottom) / 2;
      idx = MiniWindowAtPoint(m_mouseMW->m_rect.right + 1, midLevel);
      if (idx < 0) idx = MiniWindowAtPoint(1, midLevel);
      idx = NearestTextMenuMW(this, max(idx, 0));
    }
  }
  else if (vkCode == VKC_left)
  {
    if (idx < 0) idx = m_nMWs - 1;
    else
    {
      idx = MiniWindowAtPoint(m_mouseMW->m_rect.left > 0? m_mouseMW->m_rect.left - 2:
        ClientRect().right - 2, (m_mouseMW->m_rect.top + m_mouseMW->m_rect.bottom) / 2);
      idx = NearestTextMenuMW(this, max(idx, 0));
    }
  }
  ((MenuMW *)(m_mouseMW = m_MW[idx]))->Highlight(true);
  m_mouseMW->Draw();
}

void PopupMenu::LostFocus()
{
  m_mouseMW = NULL;
  ClosePopup(this);
}

void PopupMenu::MouseLButtonReleased(int x, int y, int flags)
{
  if (SetMouseMiniWindow(x, y))
  {
    if (m_mouseMW == m_cascadeMenuItemMW) return; // Cascade root item cannot be selected
    m_mouseMW->MouseLButtonReleased(x, y, flags);
  }
  else if (m_cascadeMenuItemMW)
  {
    PopupMenu *cascadeMenu = m_cascadeMenuItemMW->m_cascadeMenu;
    Point p = Point(x, y) + ClientOrigin() - cascadeMenu->ClientOrigin();
    cascadeMenu->MouseLButtonReleased(p.x, p.y, flags);
  }
  ClosePopup(this);
}

static int CountColumns(int &nextTrialHeight, int height, MiniWindow **MW, int nMWs)
{
  if (nMWs < 1) return 1; 
  int i, colHeight = 0, numCols = 1;
  nextTrialHeight = INT_MAX;
  for (i = 0;; i++)
  {
    int h = MW[i]->m_rect.bottom - MW[i]->m_rect.top;
    colHeight += h;
    if (colHeight < height)
    {
      if (i == nMWs - 1) break;
    }
    else if (colHeight == height)
    {
      if (i == nMWs - 1) break;
      nextTrialHeight = min(nextTrialHeight, colHeight + MW[i + 1]->m_rect.bottom - MW[i + 1]->m_rect.top);
      colHeight = 0;
      numCols++;
    }
    else
    {
      if (i == nMWs - 1) {numCols++; break;}
      nextTrialHeight = min(nextTrialHeight, colHeight);
      colHeight = 0;
      numCols++;
      i--;
    }
  }
  return numCols;
}

void PopupMenu::Configure()
{
  if (!m_nMWs) return;
  int i, totalHeight = 0, width, height;
  for (i = 0; i < m_nMWs; i++)
  {
    m_MW[i]->CalcSize();
    totalHeight += m_MW[i]->m_rect.bottom - m_MW[i]->m_rect.top;
  }
  int maxHeight = 7 * Screen.height / 8, nextTrialHeight;
  int nColumns = (totalHeight + maxHeight - 1) / maxHeight;
  height = (totalHeight + nColumns - 1) / nColumns;
  while (CountColumns(nextTrialHeight, height, m_MW, m_nMWs) > nColumns) height = nextTrialHeight;
  int left = 0, topOffset = 0, colWidth = 0;
  for (i = 0; i < m_nMWs; i++)
  {
    Rect &r = m_MW[i]->m_rect;
    int w = r.right - r.left, h = r.bottom - r.top;
    if (topOffset + h <= height)
    {
      r.right = (r.left = left) + w;
      topOffset = r.bottom = (r.top = topOffset) + h;
      if (w > colWidth) colWidth = w;
    }
    else
    {
      left += colWidth;
      r.right = (r.left = left) + (colWidth = w);
      r.top = 0;
      r.bottom = topOffset = h;
    }
  }
  width = left + colWidth;
  int colRight = width;
  for (i = m_nMWs - 1; i >= 0; i--)
  {
    Rect &r = m_MW[i]->m_rect;
    if (r.left < left)
    {
      colRight = left;
      left = r.left;
    }
    r.right = colRight;
  }
  BoundingRect(Rect(0, 0, width + 2, height + 2)); // allow for 1 pixel border
}

static int MWPosCompare(int x, int y, MiniWindow *MW)
{
  Rect &r = MW->m_rect;
  if (x < r.left) return -1;
  if (x >= r.right) return 1;
  if (y < r.top) return -1;
  return y >= r.bottom;
}

int PopupMenu::MiniWindowAtPoint(int x, int y) const
{
  if (m_nMWs < 1) return -1;
  int iMin = 0, iMax = m_nMWs - 1;
  int d = MWPosCompare(x, y, m_MW[iMin]);
  if (d < 0) return -1;
  if (!d) return iMin;
  d = MWPosCompare(x, y, m_MW[iMax]);
  if (d > 0) return -1;
  if (!d) return iMax;
  while (iMax > iMin + 1)
  {
    int iMid = (iMin + iMax) / 2;
    d = MWPosCompare(x, y, m_MW[iMid]);
    if (!d) return iMid;
    if (d < 0) iMax = iMid;
    else iMin = iMid;
  }
  return iMin;
}

bool PopupMenu::SetMouseMiniWindow(int x, int y)
{
  int idx = MiniWindowAtPoint(x, y);
  MiniWindow *mouseMW = idx >= 0? m_MW[idx]: NULL;
  bool isOutside = !mouseMW;
  if (isOutside) mouseMW = m_cascadeMenuItemMW;
  if (mouseMW != m_mouseMW && m_mouseMW) m_mouseMW->MouseLeft();
  m_mouseMW = mouseMW;
  return !isOutside;
}

bool g_eatMouseMoved = false;

void PopupMenu::Activate(int x, int y, int index, bool mouseActivated)
{
  Rect boundingRect = BoundingRect();
  int width = boundingRect.right - boundingRect.left,
    height = boundingRect.bottom - boundingRect.top;
  if (index < 0 || index >= m_nMWs)
  {
    if (x + width > Screen.width) x = Screen.width - width - 10;
    if (y + height + 5 > Screen.height) y = Screen.height - height - 5;
  }
  else
  {
    Point disp;
    x -= (m_MW[index]->m_rect.left + m_MW[index]->m_rect.right) / 2;
    if (x < 0) {disp.x = -x; x = 0;}
    else if (x + width > Screen.width)
    {
      disp.x = Screen.width - x - width - 10;
      x += disp.x;
    }
    y -= (m_MW[index]->m_rect.top + m_MW[index]->m_rect.bottom) / 2;
    if (y < 0) {disp.y = -y; y = 0;}
    else if (y + height + 5 > Screen.height)
    {
      disp.y = Screen.height - y - height - 10;
      y += disp.y;
    }
    if (mouseActivated)
    {
      if (disp != Point(0, 0)) PointerPosition(PointerPosition() + disp); 
    }
    else
    {
      g_eatMouseMoved = true;
      ((MenuMW *)(m_mouseMW = m_MW[index]))->Highlight(true);
    }
  }
  Move(x, y);
  ShowWithoutActivating();
  SetMouseCapture();
  BringToTop();
//  MouseMoved(x, y);
}

void PopupMenu::MouseMoved(int x, int y)
{
  if (g_eatMouseMoved) {g_eatMouseMoved = false; return;}
  bool isInside = SetMouseMiniWindow(x, y);
  if (m_cascadeMenuItemMW && !isInside)
  {
    PopupMenu *cascadeMenu = m_cascadeMenuItemMW->m_cascadeMenu;
    Point p = Point(x, y) + ClientOrigin() - cascadeMenu->ClientOrigin();
    cascadeMenu->MouseMoved(p.x, p.y);
  }
  else if (m_mouseMW) m_mouseMW->MouseMoved(x, y);
//  else m_cursor->Set();
}
