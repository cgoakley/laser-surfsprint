#define WINDOBJ_MAIN

#include <WindObj/ScrollBar.h>
#include <WindObj/Private.h>

static void DrawShadowRect(DrawingSurface &DS, Rect &rect)
{
  DS.Set(Pen(Colour::light3D));
  DS.Line(rect.TopLeft(), rect.BottomLeft() - Point(0, 2));
  DS.Line(rect.TopRight());

  DS.Set(Pen(Colour::darkShadow3D));
  DS.Line(rect.BottomRight() - Point(1, 1), rect.TopRight() - Point(1, 0));
  DS.Line(rect.BottomLeft() - Point(0, 1));

  DS.Set(Pen(Colour::buttonShadow));
  DS.Line(rect.BottomRight() - Point(2, 2), rect.TopRight() - Point(2, 0));
  DS.Line(rect.BottomLeft() - Point(0, 2));

  DS.Rectangle(Rect(rect.TopLeft() + Point(1, 1), rect.BottomRight() - Point(2, 2)), Pen(),
    Brush(Colour::buttonFace));
}

struct ScrollArrowMW : MiniWindow
{
  char m_direction;
  bool m_buttonPressed;

  ScrollArrowMW(MiniWindowContainer *parent, Rect rect, char direction) : 
    MiniWindow(parent, rect.left, rect.top, rect.right, rect.bottom), m_direction(direction),
    m_buttonPressed(false) {}

  void Exposed(DrawingSurface &DS, const Rect &)
  {
    Point pressedOffset(0, 0);
    if (m_buttonPressed)
    {
      DS.Rectangle(m_rect, Pen(Colour::buttonShadow), Brush(Colour::buttonFace));
      pressedOffset = Point(1, 1);
    }
    else DrawShadowRect(DS, m_rect);
    int w = m_rect.right - m_rect.left, h = m_rect.bottom - m_rect.top;
    Point p[3];
    if (m_direction == 0) // pointing up
    {
      p[0] = Point(w / 2, h / 3);
      p[1] = Point(w / 4, h - p[0].y);
      p[2] = Point(2 * p[0].x - p[1].x, p[1].y);
    }
    else if (m_direction == 1) // pointing right
    {
      p[0] = Point(2 * w / 3, h / 2);
      p[1] = Point(w - p[0].x, h / 4);
      p[2] = Point(p[1].x, 2 * p[0].y - p[1].y);
    }
    if (m_direction == 2) // pointing down
    {
      p[0] = Point(w / 2, 2 * h / 3);
      p[1] = Point(w / 4, h - p[0].y);
      p[2] = Point(2 * p[0].x - p[1].x, p[1].y);
    }
    else if (m_direction == 3) // pointing left
    {
      p[0] = Point(w / 3, h / 2);
      p[1] = Point(w - p[0].x, h / 4);
      p[2] = Point(p[1].x, p[0].y - p[1].y);
    }
    for (int i = 0; i < 3; i++) p[i] += m_rect.TopLeft() + pressedOffset;
    DS.Polygon(p, 3, Pen(), Brush(Colour::buttonText));
  }

  void MouseLButtonPressed(int x, int y, int flags)
  {
    SetMouseCapture();
    m_buttonPressed = true;
    Draw();
  }

  void MouseLButtonReleased(int x, int y, int flags)
  {
//    OutputDebug("ScrollArrowMW::MouseLButtonReleased\n");
    ReleaseMouseCapture();
    m_buttonPressed = false;
    Draw();
  }

  void MouseMoved(int x, int y)
  {
    if (!IsMouseCaptured() || (m_rect & Point(x, y)) == m_buttonPressed) return;
    m_buttonPressed = !m_buttonPressed;
    Draw();
  }
};

struct ScrollTrackMW : MiniWindow
{
  ScrollTrackMW(MiniWindowContainer *parent, Rect rect) : 
    MiniWindow(parent, rect.left, rect.top, rect.right, rect.bottom) {}

  void Exposed(DrawingSurface &DS, const Rect &)
  {
    DS.Rectangle(m_rect, Pen(), 
      Brush(Colour(Colour(Colour::buttonFace), Colour(Colour::buttonHighlight))));
  }
};

struct ScrollThumbMW : MiniWindow
{
  ScrollThumbMW(MiniWindowContainer *parent, Rect rect) : 
    MiniWindow(parent, rect.left, rect.top, rect.right, rect.bottom) {}

  void Exposed(DrawingSurface &DS, const Rect &)
  {
    DrawShadowRect(DS, m_rect);
  }
};

ScrollBar::ScrollBar(Window *parent, Rect rect, Orientation orientation, 
  int posMax, int thumbSize, int pos) :  
  MiniWindowContainer(parent, rect, WS_containedByParent), m_orientation(orientation), 
    m_posMax(posMax), m_thumbSize(thumbSize), m_pos(pos)
{
  int w = rect.right - rect.left, h = rect.bottom - rect.top;
  SetSize(4);
  m_MW[0] = new ScrollArrowMW(this, Rect(0, 0, w, w), 0);
  m_MW[1] = new ScrollArrowMW(this, Rect(0, h - w, w, h), 2);
  m_MW[2] = new ScrollTrackMW(this, Rect(0, w, w, h - w));
  int thumbSizePix = thumbSize * h / posMax;
  int posPix = pos * (h - thumbSizePix) / posMax;
  m_MW[3] = new ScrollThumbMW(this, Rect(0, w + posPix, w, w + posPix + thumbSizePix));
}


/*void ScrollBar::Exposed(DrawingSurface &DS, const Rect &)
{
  Rect m_rect = ClientRect();
//  if (m_orientation ==  ScrollBar::Vertical)
//  {
    int w = m_rect.right;
    DrawShadowRect(DS, Rect(0, 0, w, w));
    Point triangle[3];
    triangle[0] = Point(w / 4, 2 * w / 3);
    triangle[1] = Point(w / 2, w / 3);
    triangle[2] = Point(3 * w / 4, 2 * w / 3);
    DS.Polygon(triangle, 3, Pen(), Brush(Colour::buttonText));
    int trackSize = m_rect.bottom - m_rect.top - 2 * w;
    int thumbSizePix = m_thumbSize * trackSize / m_posMax;
    int posPix = m_pos * (trackSize - thumbSizePix) / m_posMax;
    Pen nullPen;
    Brush scrollBrush(Colour(Colour(Colour::buttonFace), Colour(Colour::buttonHighlight)));
    DS.Rectangle(Rect(0, w, w, w + posPix), nullPen, scrollBrush);
    DrawShadowRect(DS, Rect(0, w + posPix, w, w + posPix + thumbSizePix));
    DS.Rectangle(Rect(0, w + posPix + thumbSizePix, w, m_rect.bottom - w), nullPen, scrollBrush);
    DrawShadowRect(DS, Rect(0, m_rect.bottom - w, w, m_rect.bottom));
    triangle[0] = Point(w / 4, m_rect.bottom - w + w / 3);
    triangle[1] = Point(w / 2, m_rect.bottom - w + 2 * w / 3);
    triangle[2] = Point(3 * w / 4, m_rect.bottom - w + w / 3);
    DS.Polygon(triangle, 3, Pen(), Brush(Colour::buttonText));
//  }
} */
