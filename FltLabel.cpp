#define WINDOBJ_MAIN

#include <WindObj/Private.h>

#include <malloc.h>

FloatingLabel *g_floatingLabel = NULL;

FloatingLabel::FloatingLabel() : Window(NULL, Rect(0, 0, 1, 1), WS_hidden) {}

void FloatingLabel::Exposed(DrawingSurface &DS, const Rect &invRect)
{
  DS.Rectangle();
  int n = GetText((char *)NULL);
  char *buff = (char *)alloca(n + 1);
  GetText(buff);
  DS.Text(buff, Point(2, 2));
}

void SetFloatingLabel(const Point &topLeft, const char *caption)
{
  if (!g_floatingLabel) g_floatingLabel = new FloatingLabel;
  if (caption)
  {
    g_floatingLabel->SetText(caption);
    DrawingSurface DS(*g_floatingLabel);
    Rect textRect = DS.TextRect(caption);
    textRect.right += 4;
    textRect.bottom += 4;
    g_floatingLabel->ClientRect(textRect);
  }
  g_floatingLabel->Move(topLeft.x, topLeft.y);
  ShowWindow(g_floatingLabel->m_windowData->hWnd, SW_SHOWNOACTIVATE);
}

void HideFloatingLabel()
{
  if (g_floatingLabel) g_floatingLabel->Hide();
}
