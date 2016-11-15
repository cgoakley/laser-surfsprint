#define WINDOBJ_MAIN

#include <WindObj/TiledContainer.h>

TiledContainer::~TiledContainer()
{
  delete m_paneStructure;
}

TCPane::TCPane(Window *window)
{
  m_split = TCPane::none;
  m_division = 0;
  m_subPane[0] = m_subPane[1] = NULL;
  m_userSize = false;
  m_width = 0;
  m_node = window;
}

TCPane::TCPane(TCPane::Split split, float division, TCPane *subPane1, TCPane *subPane2,
               bool userSize, int width, const Colour &colour, const Colour &borderColour)
{
  m_split = split;
  m_division = division;
  m_subPane[0] = subPane1;
  m_subPane[1] = subPane2;
  m_userSize = userSize;
  m_width = width;
  m_colour = colour;
  m_borderColour = borderColour;
  m_node = NULL;
}

TCPane::~TCPane()
{
  delete m_subPane[0];
  delete m_subPane[1];
}

static bool PaneSubAreas(Rect &rect1, Rect &rect2, Rect &divider,
                        TCPane *pane, const Rect &area)
{
  if (!pane) return false;
  if (pane->m_split == TCPane::horiz)
  {
    divider.left = area.left - pane->m_width / 2;
    if (pane->m_division >= 1) divider.left += (int)pane->m_division;
    else divider.left += (int)(pane->m_division * (area.right - area.left));
    rect2.left = divider.right = divider.left + pane->m_width;
    rect1.top = rect2.top = divider.top = area.top;
    rect1.bottom = rect2.bottom = divider.bottom = area.bottom;
    rect1.left = area.left;
    rect1.right = divider.left;
    rect2.right = area.right;
  }
  else if (pane->m_split == TCPane::vert)
  {
    divider.top = area.top - pane->m_width / 2;
    if (pane->m_division >= 1) divider.top += (int)pane->m_division;
    else divider.top += (int)(pane->m_division * (area.bottom - area.top));
    rect2.top = divider.bottom = divider.top + pane->m_width;
    rect1.left = rect2.left = divider.left = area.left;
    rect1.right = rect2.right = divider.right = area.right;
    rect1.top = area.top;
    rect1.bottom = divider.top;
    rect2.bottom = area.bottom;
  }
  else return false;
  return true;
}

static void SizePane(TiledContainer *window, TCPane *pane, Rect &drawArea)
{
  Rect rect1, rect2, divider;
  if (PaneSubAreas(rect1, rect2, divider, pane, drawArea))
  {
    SizePane(window, pane->m_subPane[0], rect1);
    SizePane(window, pane->m_subPane[1], rect2);
    window->Draw(divider);
  }
  else if (pane && pane->m_node) pane->m_node->Size(drawArea);
}

void TiledContainer::Sized()
{
  SizePane(this, m_paneStructure, ClientRect());
}

static void DrawPane(TCPane *pane, DrawingSurface &DS, Rect &drawArea)
{
  Rect rect1, rect2, divider;
  if (PaneSubAreas(rect1, rect2, divider, pane, drawArea))
  {
    DrawPane(pane->m_subPane[0], DS, rect1);
    DrawPane(pane->m_subPane[1], DS, rect2);
    DS.Rectangle(divider, Pen(pane->m_borderColour), Brush(pane->m_colour));
  }
  else if (!pane || !pane->m_node)
    DS.Rectangle(drawArea, Pen(), Brush(Colour::window));
}

void TiledContainer::Exposed(DrawingSurface &DS, const Rect &invRect)
{
  DrawPane(m_paneStructure, DS, ClientRect());
}

static TCPane *PointerPane(Rect &divider, Rect &area,
                           TCPane *pane, const Rect &origArea, int x, int y)
{
  Rect rect1, rect2;
  if (!PaneSubAreas(rect1, rect2, divider, pane, origArea)) return NULL;
  if (x >= divider.left && x < divider.right &&
      y >= divider.top && y < divider.bottom)
  {
    area = origArea;
    return pane;
  }
  TCPane *pane1 = PointerPane(divider, area, pane->m_subPane[0], rect1, x, y);
  if (pane1) return pane1;
  return PointerPane(divider, area, pane->m_subPane[1], rect2, x, y);
}

TCPane *g_draggingPane = NULL;
int g_dragX, g_dragY;
Rect g_dragRect, g_origRect, g_paneArea;

void TiledContainer::MouseMoved(int x, int y)
{
  if (!g_draggingPane || !g_draggingPane->m_userSize) return;
  DrawingSurface DS(*this);
  DS.Rectangle(g_dragRect, Pen(c_white), Brush(), DefColour, 
    DrawingSurface::defaultBM, DrawingSurface::xorPen);
  if (g_draggingPane->m_split == TCPane::horiz)
  {
    g_dragRect.left += x - g_dragX;
    g_dragRect.right += x - g_dragX;
  }
  else
  {
    g_dragRect.top += y - g_dragY;
    g_dragRect.bottom += y - g_dragY;
  }
  g_dragX = x; g_dragY = y;
  DS.Rectangle(g_dragRect, Pen(c_white), Brush(), DefColour,
    DrawingSurface::defaultBM, DrawingSurface::xorPen);
}

Cursor g_arrowNS(Cursor::arrowNS), g_arrowEW(Cursor::arrowEW);

void TiledContainer::SetCursor(Cursor *&cursor)
{
  if (!g_draggingPane || !g_draggingPane->m_userSize) return;
  Rect divider, area;
  Point p = PointerPosition() - ClientOrigin();
  TCPane *pane = PointerPane(divider, area, m_paneStructure, ClientRect(), p.x, p.y);
  if (!pane || !pane->m_userSize) return;
  if (pane->m_split == TCPane::horiz) cursor = &g_arrowEW;
  else if (pane->m_split == TCPane::vert) cursor = &g_arrowNS;
}

void TiledContainer::MouseLButtonPressed(int x, int y, int flags)
{
  g_draggingPane = PointerPane(g_origRect, g_paneArea, m_paneStructure, ClientRect(), x, y);
  if (!g_draggingPane || !g_draggingPane->m_userSize) return;
  g_dragX = x;
  g_dragY = y;
  DrawingSurface DS(*this);
  DS.Rectangle(g_dragRect = g_origRect, Pen(c_white), Brush(), DefColour, 
    DrawingSurface::defaultBM, DrawingSurface::xorPen);
  SetMouseCapture();
}

void TiledContainer::MouseLButtonReleased(int x, int y, int flags)
{
  if (!g_draggingPane || !g_draggingPane->m_userSize) return;
  ReleaseMouseCapture();
// Erase rectangle ...
  {
    DrawingSurface DS(*this);
    DS.Rectangle(g_dragRect, Pen(c_white), Brush(), DefColour, 
      DrawingSurface::defaultBM, DrawingSurface::xorPen);
  }
  if (g_draggingPane->m_split == TCPane::horiz)
  {
    if (g_dragRect.left < g_paneArea.left || g_dragRect.right > g_paneArea.right)
    {
      g_draggingPane = NULL;
      return;
    }
    int offs = (g_dragRect.left + g_dragRect.right) / 2 - g_paneArea.left;
    if (g_draggingPane->m_division < 1) g_draggingPane->m_division = offs / 
      (float)(g_paneArea.right - g_paneArea.left);
    else g_draggingPane->m_division = (float)offs;
  }
  else
  {
    if (g_dragRect.top < g_paneArea.top || g_dragRect.bottom > g_paneArea.bottom)
    {
      g_draggingPane = NULL;
      return;
    }
    int offs = (g_dragRect.top + g_dragRect.bottom) / 2 - g_paneArea.top;
    if (g_draggingPane->m_division < 1) g_draggingPane->m_division = offs / 
      (float)(g_paneArea.bottom - g_paneArea.top);
    else g_draggingPane->m_division = (float)offs;
  }
  g_draggingPane = NULL;
  Sized();
  Draw();
}
