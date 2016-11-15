#define WINDOBJ_MAIN

#include <WindObj/MiniWindow.h>
#include <WindObj/Private.h>

#include <limits.h>
#include <string.h>

MiniWindowContainer::~MiniWindowContainer()
{
  SetSize(0);
  if (m_MW) ClientFree(m_MW);
}

void MiniWindowContainer::SetSize(int nMWs)
{
  if (nMWs > m_nMWs)
  {
    if ((nMWs + 49) / 50 > (m_nMWs + 49) / 50)
      m_MW = (MiniWindow **)ClientRealloc(m_MW, (nMWs + 49) / 50 * 50 * sizeof(MiniWindow *));
    memset(m_MW + m_nMWs, 0, (nMWs - m_nMWs) * sizeof(MiniWindow *));
  }
  else
  {
    for (int i = nMWs; i < m_nMWs; i++)
      delete m_MW[i];
  }
  m_nMWs = nMWs;
  m_mouseMW = m_focalMW = NULL;
}

MiniWindow *MiniWindowContainer::AddMiniWindow(MiniWindow *MW)
{
  SetSize(m_nMWs + 1);
  return m_MW[m_nMWs - 1] = MW;
}

int MiniWindowContainer::MiniWindowAtPoint(int x, int y) const
{
  Point pt(x, y);
  int idx = m_nMWs - 1;
  for (; idx >= 0 && !(m_MW[idx]->m_rect & pt); idx--);
  return idx;
}

bool MiniWindowContainer::SetMouseMiniWindow(int x, int y)
{
  int idx = MiniWindowAtPoint(x, y);
  MiniWindow *mouseMW = idx >= 0? m_MW[idx]: NULL;
  if (mouseMW != m_mouseMW)
  {
    if (m_mouseMW) m_mouseMW->MouseLeft();
  }
  return (m_mouseMW = mouseMW) != NULL;
}

struct RectangleList
{
  Rect *m_rect;
  int m_nRects;

  RectangleList() : m_rect(NULL), m_nRects(0) {}
  RectangleList(int nRects);
  RectangleList(const RectangleList &);
  RectangleList(const Rect &);
  ~RectangleList();

  void SetSize(int nRects);
  void Clean(); // remove all null rectangles

  RectangleList operator &(const Rect &);
  RectangleList &operator &=(const Rect &);

  RectangleList operator &(const RectangleList &);
  RectangleList &operator &=(const RectangleList &);
};

RectangleList::RectangleList(int nRects) : m_rect(NULL), m_nRects(0)
{
  SetSize(nRects);
}

RectangleList::RectangleList(const RectangleList &RL) : m_rect(NULL), m_nRects(0)
{
  SetSize(RL.m_nRects);
  memcpy(m_rect, RL.m_rect, m_nRects * sizeof(Rect));
}

RectangleList::RectangleList(const Rect &rect) : m_rect(NULL), m_nRects(0)
{
  SetSize(1);
  m_rect[0] = rect;
}

RectangleList::~RectangleList()
{
  SetSize(0);
}

void RectangleList::SetSize(int nRects)
{
  if (nRects < 1)
  {
    if (m_rect) ClientFree(m_rect);
    m_rect = NULL;
  }
  else
  {
    m_rect = (Rect *)ClientRealloc(m_rect, nRects * sizeof(Rect));
    if (nRects > m_nRects) memset(m_rect + m_nRects, 0, (nRects - m_nRects) * sizeof(Rect));
  }
  m_nRects = nRects;
}

void RectangleList::Clean()
{
  int i, j = 0;
  for (i = 0; i < m_nRects; i++)
  {
    if (m_rect[i]) m_rect[j++] = m_rect[i];
  }
  if (j < m_nRects) SetSize(j);
}

RectangleList RectangleList::operator &(const Rect &rect)
{
  RectangleList RL(m_nRects);
  for (int i = 0; i < m_nRects; i++) RL.m_rect[i] = m_rect[i] & rect;
  RL.Clean();
  return RL;
}

RectangleList &RectangleList::operator &=(const Rect &rect)
{
  for (int i = 0; i < m_nRects; i++) m_rect[i] &= rect;
  Clean();
  return *this;
}

RectangleList RectangleList::operator &(const RectangleList &RL)
{
  RectangleList overlapRL(m_nRects * RL.m_nRects);
  for (int i = 0; i < RL.m_nRects; i++)
    for (int j = 0; j < m_nRects; j++)
      overlapRL.m_rect[i * m_nRects + j] = m_rect[j] & RL.m_rect[i];
  overlapRL.Clean();
  return overlapRL;
}

static void Exchange(RectangleList &RL1, RectangleList &RL2)
{
  int n = RL1.m_nRects;
  Rect *r = RL1.m_rect;
  RL1.m_nRects = RL2.m_nRects;
  RL1.m_rect = RL2.m_rect;
  RL2.m_nRects = n;
  RL2.m_rect = r;
}

RectangleList &RectangleList::operator &=(const RectangleList &RL)
{
  RectangleList overlapRL(m_nRects * RL.m_nRects);
  for (int i = 0; i < RL.m_nRects; i++)
    for (int j = 0; j < m_nRects; j++)
      overlapRL.m_rect[i * m_nRects + j] = m_rect[j] & RL.m_rect[i];
  overlapRL.Clean();
  Exchange(*this, overlapRL);
  return *this;
}

// The complement of a ractangle can be represented by four rectangles, each extending to infinity ... 

RectangleList operator~(const Rect &rect)
{
  if (!rect)
  {
    RectangleList RL(1);
    RL.m_rect[0].left = INT_MIN;  
    RL.m_rect[0].top = INT_MIN;
    RL.m_rect[0].right = INT_MAX;
    RL.m_rect[0].bottom = INT_MAX;
    return RL;
  }
  RectangleList RL(4);
  RL.m_rect[0].left = INT_MIN;  
  RL.m_rect[0].top = rect.top;
  RL.m_rect[0].right = rect.left;
  RL.m_rect[0].bottom = rect.bottom;
  RL.m_rect[1].left = rect.right;
  RL.m_rect[1].top = rect.top;
  RL.m_rect[1].right = INT_MAX;
  RL.m_rect[1].bottom = rect.bottom;
  RL.m_rect[2].left = INT_MIN;
  RL.m_rect[2].top = INT_MIN;
  RL.m_rect[2].right = INT_MAX;
  RL.m_rect[2].bottom = rect.top;
  RL.m_rect[3].left = INT_MIN;
  RL.m_rect[3].top = rect.bottom;
  RL.m_rect[3].right = INT_MAX;
  RL.m_rect[3].bottom = INT_MAX;
  return RL;
}

void MiniWindowContainer::Exposed(DrawingSurface &DS, const Rect &invRect)
{
  RectangleList RL = invRect;
  int i;
  for (i = 0; i < m_nMWs; i++)
  {
    m_MW[i]->Exposed(DS, invRect);
    RL &= ~m_MW[i]->m_rect;
  }
  DS.Set(Pen());
  DS.Set(m_backBrush);
  for (i = 0; i < RL.m_nRects; i++)
    DS.Rectangle(RL.m_rect[i]);
}

void MiniWindowContainer::KeyPressed(VirtualKeyCode vkCode, char ascii, unsigned short repeat)
{
  if (m_focalMW) m_focalMW->KeyPressed(vkCode, ascii, repeat);
}

void MiniWindowContainer::KeyReleased(VirtualKeyCode vkCode)
{
  if (m_focalMW) m_focalMW->KeyReleased(vkCode);
}

void MiniWindowContainer::MouseLButtonPressed(int x, int y, int flags)
{
  if (SetMouseMiniWindow(x, y)) m_mouseMW->MouseLButtonPressed(x, y, flags);
}

void MiniWindowContainer::MouseLButtonReleased(int x, int y, int flags)
{
  if (m_mouseMW) m_mouseMW->MouseLButtonReleased(x, y, flags);
}

void MiniWindowContainer::MouseLButtonDblClicked(int x, int y, int flags)
{
  if (SetMouseMiniWindow(x, y)) m_mouseMW->MouseLButtonDblClicked(x, y, flags);
}

void MiniWindowContainer::MouseRButtonPressed(int x, int y, int flags)
{
  if (SetMouseMiniWindow(x, y)) m_mouseMW->MouseRButtonPressed(x, y, flags);
}

void MiniWindowContainer::MouseRButtonReleased(int x, int y, int flags)
{
  if (SetMouseMiniWindow(x, y)) m_mouseMW->MouseRButtonReleased(x, y, flags);
}

void MiniWindowContainer::MouseRButtonDblClicked(int x, int y, int flags)
{
  if (SetMouseMiniWindow(x, y)) m_mouseMW->MouseRButtonDblClicked(x, y, flags);
}

void MiniWindowContainer::MouseLeft()
{
  if (!m_mouseMW) return;
  m_mouseMW->MouseLeft();
  m_mouseMW = NULL;
}

void MiniWindowContainer::MouseMoved(int x, int y)
{
  if (SetMouseMiniWindow(x, y)) m_mouseMW->MouseMoved(x, y);
//  else m_cursor->Set();
}

// MiniWindow

void MiniWindow::Draw()
{
  DrawingSurface DS(*m_parent);
  Exposed(DS, m_rect);
}

void MiniWindow::SetMouseCapture() const
{
  if (m_parent->m_mouseMW != this)
  {
    if (m_parent->m_mouseMW) m_parent->m_mouseMW->MouseLeft();
    m_parent->m_mouseMW = (MiniWindow *)this;
  }
  m_parent->SetMouseCapture();
}

void MiniWindow::ReleaseMouseCapture() const
{
  m_parent->ReleaseMouseCapture();
}

bool MiniWindow::IsMouseCaptured() const
{
  return m_parent->m_mouseMW == this && m_parent->IsMouseCaptured();
}

static MiniWindow *UnderMW(MiniWindow *MW, int x, int y)
{
  MiniWindowContainer *parent = MW->m_parent;
  int i;
  for (i = parent->m_nMWs - 1; i >= 0 && parent->m_MW[i] != MW; i--);
  Point pt(x, y);
  for (i--; i >= 0 && !(parent->m_MW[i]->m_rect & pt); i--);
  return i >= 0? parent->m_MW[i]: NULL;
}

/* In the default implementation of the mouse functions, the message is
   just passed on to the first mini-window underneath, if there is one ... */

void MiniWindow::MouseLButtonPressed(int x, int y, int flags)
{
  MiniWindow *underMW = UnderMW(this, x, y);
  if (underMW) underMW->MouseLButtonPressed(x, y, flags);
}

void MiniWindow::MouseLButtonReleased(int x, int y, int flags)
{
  MiniWindow *underMW = UnderMW(this, x, y);
  if (underMW) underMW->MouseLButtonReleased(x, y, flags);
}

void MiniWindow::MouseLButtonDblClicked(int x, int y, int flags)
{
  MiniWindow *underMW = UnderMW(this, x, y);
  if (underMW) underMW->MouseLButtonDblClicked(x, y, flags);
}

void MiniWindow::MouseRButtonPressed(int x, int y, int flags)
{
  MiniWindow *underMW = UnderMW(this, x, y);
  if (underMW) underMW->MouseRButtonPressed(x, y, flags);
}

void MiniWindow::MouseRButtonReleased(int x, int y, int flags)
{
  MiniWindow *underMW = UnderMW(this, x, y);
  if (underMW) underMW->MouseRButtonReleased(x, y, flags);
}

void MiniWindow::MouseRButtonDblClicked(int x, int y, int flags)
{
  MiniWindow *underMW = UnderMW(this, x, y);
  if (underMW) underMW->MouseRButtonDblClicked(x, y, flags);
}

void MiniWindow::MouseMoved(int x, int y)
{
  MiniWindow *underMW = UnderMW(this, x, y);
  if (underMW) underMW->MouseMoved(x, y);
//  else m_parent->m_cursor->Set();
}
