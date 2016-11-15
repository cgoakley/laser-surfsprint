#define WINDOBJ_MAIN

#include <WindObj/TextSplashWindow.h>

#include <string.h>

TextSplashWindow::TextSplashWindow(Window *parent, int nLines, const Font &font) : Window(parent, Rect(20, 30, 31, 130), WS_thinBorder)
{
  m_font = font;
  m_fontHeight = m_font.TextHeight();
  int height = (m_nLines = nLines) * m_fontHeight;
  Point p = parent->ClientOrigin();
  Move(p.x + 30, p.y + 20);
  Size(100, height);
  int n = nLines * sizeof(char *);
  m_data = (char **)malloc(n);
  memset(m_data, 0, n);
  m_mouseDown = false;
}

TextSplashWindow::~TextSplashWindow()
{
  for (int i = 0; i < m_nLines; i++)
    delete [] m_data[i];
  free(m_data);
}

void TextSplashWindow::Exposed(DrawingSurface &DS, const Rect &invRect)
{
  DS.SetClip(invRect);
  DS.Rectangle(invRect, Pen(), Brush(Colour::window));
  for (int i = invRect.top / m_fontHeight * 0; i <= m_nLines - 1 + 0 * invRect.bottom / m_fontHeight; i++)
  {
    if (m_data[i]) DS.Text(m_data[i], Point(0, i * m_fontHeight), m_font);
  }
}

void TextSplashWindow::AddLine(const char *text)
{
  delete [] m_data[m_nLines - 1];
  memmove(m_data + 1, m_data, (m_nLines - 1) * sizeof(char *));
  if (!text || !text[0]) m_data[0] = NULL;
  else strcpy(m_data[0] = new char[strlen(text) + 1], text);
  int w = 100;
  for (int i = 0; i < m_nLines; i++)
  {
    if (!m_data[i]) continue;
    int w1 = m_font.TextWidth(m_data[i]);
    if (w1 > w) w = w1;
  }
  Rect clientRect = ClientRect();
  if (w > clientRect.right) ClientRect(Rect(0, 0, w, clientRect.bottom));
  Draw();
}

void TextSplashWindow::MouseLButtonPressed(int x, int y, int flags)
{
  g_dragPos = Point(x, y);
  m_mouseDown = true;
}

void TextSplashWindow::MouseLButtonReleased(int x, int y, int flags)
{
  m_mouseDown = false;
  Point p = ClientOrigin() + Point(x, y) - g_dragPos;
  Move(p.x, p.y);
}

void TextSplashWindow::MouseMoved(int x, int y)
{
  if (!m_mouseDown) return;
  Point p = ClientOrigin() + Point(x, y) - g_dragPos;
  Move(p.x, p.y);
}

void TextSplashWindow::MouseLeft()
{
  m_mouseDown = false;
}

