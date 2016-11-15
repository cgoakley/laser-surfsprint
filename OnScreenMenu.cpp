#define WINDOBJ_MAIN

#include <WindObj/OnScreenMenu.h>
#include <WindObj/Private.h>

#include <string.h>

struct OnScreenMenuPopup : PopupMenu
{
  OnScreenMenuPopup(Window *parent) : PopupMenu(parent) {}

  void SelectionMade(int index, int keyMods);
};

OnScreenMenu::OnScreenMenu(Window *parent, const Rect &rect, bool hasBorder, int index,
   Colour foreColour, Colour backColour) :  
  Window(parent, rect), m_hasBorder(hasBorder), m_index(-1), m_foreColour(foreColour), 
    m_backColour(backColour)
{
  m_popup = new OnScreenMenuPopup(this);
}

void OnScreenMenu::Exposed(DrawingSurface &DS, const Rect &invRect)
{
  Pen pen;
  Rect clientRect = ClientRect();
  if (m_hasBorder)
  {
    pen = Pen(m_foreColour);
    clientRect.left++;
    clientRect.top++;
    clientRect.right--;
    clientRect.bottom--;
  }
  DS.Rectangle(DefRect, pen, Brush(m_backColour));
  if (m_index < 0 || m_index >= m_popup->m_nMWs) return;
  TextMenuItemMW *MW = (TextMenuItemMW *)m_popup->m_MW[m_index];
  Rect &r = MW->m_rect;
  Point textOffs(5, (ClientRect().bottom - (r.bottom - r.top)) / 2);
  Point origShift = -r.TopLeft() + textOffs;
  DS.SetClip(clientRect);
  DS.Origin(origShift.x, origShift.y);
  MW->Exposed(DS, clientRect - origShift);
  DS.Origin(0, 0);
  if (IsFocal() && MW->IsTextMenuItemMW())
  {
    DS.Set(*MW->m_font);
    Rect textRect = DS.TextRect(MW->m_text);
    DS.Rectangle(textRect + textOffs + Point(MW->m_bitmapSpace, 0), 
      Pen(Colour(m_foreColour, m_backColour), 1, Pen::dotted), Brush());
  }
}

void OnScreenMenu::GotFocus() {Draw();}
void OnScreenMenu::LostFocus() {Draw();}

void OnScreenMenu::KeyPressed(VirtualKeyCode vkCode, char ascii, unsigned short repeat)
{
  if (vkCode == VKC_return) Popup(false);
}

void OnScreenMenu::MouseLButtonPressed(int x, int y, int flags)
{
  Popup(true);
}

void OnScreenMenu::Popup(bool mouseActivated)
{
  if (m_popup->m_nMWs < 1) return;
  Point p = BoundingRect().TopLeft();
  if (m_index >= 0 && m_index < m_popup->m_nMWs)
  {
    const Rect &r = m_popup->m_MW[m_index]->m_rect;
    p += Point(4, (ClientRect().bottom - (r.bottom - r.top)) / 2 - 1);
  }
  m_popup->Activate(p.x, p.y, m_index, mouseActivated);
}

void OnScreenMenuPopup::SelectionMade(int index, int keyMods)
{
  OnScreenMenu *parent = (OnScreenMenu *)m_parent;
  bool newValue = index >= 0 && index != parent->m_index;
  if (newValue) parent->m_index = index;
  parent->SetFocus();
  parent->Draw();
  if (newValue) parent->SelectionMade();
}
