#define WINDOBJ_MAIN

#include <WindObj/Private.h>

#include <limits.h>

MenuTable::MenuTable()
{
  hMenu = NULL;
  nItems = 0;
  item = NULL;
}

MenuTable::~MenuTable()
{
  ClientFree(item);
}

int MenuTable::InsertEntry(Menu *menu)
{
  int i = 0;
  for (;; i++)
  {
    if (i == nItems)
    {
      if (nItems % 20 == 0) item = (Menu **)ClientRealloc(item, (nItems + 20) * sizeof(Menu *));
      nItems++;
      break;
    }
    if (!item[i]) break;
  }
  item[i] = menu;
  return i + 3;
}

void MenuTable::DeleteEntry(int id)
{
  item[id - 3] = NULL;
}

int MenuTable::GetMenuID(Menu *menu)
{
  int i;
  for (i = 0; i < nItems; i++)
    if (item[i] == menu) return i + 3;
  return -1;
}

MenuData::MenuData(bool greyed, bool checked, UINT flags)
{
  prev = next = sub = parent = NULL;
  id = -1;
  type = (greyed? MF_GRAYED: 0) | (checked? MF_CHECKED: 0) | flags;
  data = NULL;
}

Menu::Menu(const char *text, bool greyed, bool checked)
{
  if (strcmp(text, "-"))
  {
    m_menuData = new MenuData(greyed, checked, MF_STRING);
    m_menuData->data = new char[strlen(text) + 1];
    strcpy(m_menuData->data, text);
  }
  else m_menuData = new MenuData(greyed, checked, MF_SEPARATOR);
}

Menu::Menu(const Bitmap *bitmap, bool greyed, bool checked)
{
  m_menuData = new MenuData(greyed, checked, MF_BITMAP);
  m_menuData->data = (char *)bitmap->m_bitmapData;
}

Menu::Menu(bool greyed, bool checked)
{
  m_menuData = new MenuData(greyed, checked, MF_OWNERDRAW);
  m_menuData->data = NULL;
}

Menu::~Menu()
{
  Menu *m, *m1;
  MenuTable *mt = (MenuTable *)Top()->m_menuData->data;
  for (m = m_menuData->sub; m; m = m1)
  {
    m1 = m->m_menuData->next;
    delete m;
  }
  if (mt)
  {
    if (m_menuData->type & MF_POPUP)
    {
      DestroyMenu((HMENU)m_menuData->id);
      mt->DeleteEntry(mt->GetMenuID(this));
    }
    else 
    {
      DeleteMenu((HMENU)m_menuData->parent->m_menuData->id, 
        (UINT)m_menuData->id, MF_BYCOMMAND);
      mt->DeleteEntry(m_menuData->id);
    }
  }
  if (m_menuData->parent && m_menuData->parent->m_menuData->sub == this)
    m_menuData->parent->m_menuData->sub = 
      m_menuData->prev? m_menuData->prev:
      m_menuData->next? m_menuData->next: m_menuData->sub;
  if (m_menuData->prev) m_menuData->prev->m_menuData->next = m_menuData->next;
  if (m_menuData->next) m_menuData->next->m_menuData->prev = m_menuData->prev;
  if (!(m_menuData->type & (MF_BITMAP | MF_OWNERDRAW))) delete [] m_menuData->data; // string
  if (!m_menuData->parent) delete (MenuTable *)m_menuData->data;
  delete m_menuData;
}

Menu *Menu::Add(const char *text, bool greyed, bool checked)
{
  return Add(new Menu(text, greyed, checked));
}

Menu *Menu::Add(const char *text, SelectFunc proc, bool greyed, bool checked)
{
  return Add(new SimpleMenu(text, proc, greyed, checked));
}

Menu *Menu::Add(const Bitmap *bitmap, bool greyed, bool checked)
{
  return Add(new Menu(bitmap, greyed, checked));
}

Menu *Menu::Add(const Bitmap *bitmap, SelectFunc proc, bool greyed, bool checked)
{
  return Add(new SimpleMenu(bitmap, proc, greyed, checked));
}

Menu *Menu::Add(Menu *menu, Menu *before)
{
  MenuData *md = menu->m_menuData;
  Menu *m;
  MenuTable *mt;
  
// only attach menu if not already attached to something. Force tree topology

  if (md->id != -1 || md->prev || md->next || md->parent) return menu;

  bool firstSubMenu = !(m = m_menuData->sub);
  if (firstSubMenu) m_menuData->sub = menu;
  else // add to already exg list of submenus
  {
    if (before)
    {
      md->next = before;
      if (md->prev = before->m_menuData->prev)
        md->prev->m_menuData->next = menu;
      else m_menuData->sub = menu;
      before->m_menuData->prev = menu;
    }
    else
    {
      for (; m->m_menuData->next; m = m->m_menuData->next);
      m->m_menuData->next = menu;
      md->prev = m;
    }
  }
  md->parent = this;
  if (mt = (MenuTable *)Top()->m_menuData->data)
  {
    if (firstSubMenu) // parent item needs to have a popup attached
    {
      HMENU hPopup = CreatePopupMenu();
      m_menuData->type |= MF_POPUP;
      ModifyMenu(mt->hMenu, m_menuData->id, m_menuData->type, (UINT)hPopup, m_menuData->data);
      m_menuData->id = (int)hPopup;
    }
    menu->Activate();
  }
  return menu;
}

Menu *Menu::Parent() const {return m_menuData->parent;}

Menu *Menu::Top() const
{
  const Menu *m = this;
  for (; m->Parent(); m = m->Parent());
  return (Menu *)m;
}

Menu *Menu::Sub() const {return m_menuData->sub;}

Menu *Menu::Prev() const {return m_menuData->prev;}

Menu *Menu::Next() const {return m_menuData->next;}

Window *Menu::Owner() const
{
  MENUINFO menuInfo = {sizeof(MENUINFO), MIM_MENUDATA};
  BOOL b = GetMenuInfo((HMENU)Top()->m_menuData->id, &menuInfo);
  return (Window *)menuInfo.dwMenuData;
}

void Menu::Activate()
{
  MenuTable *mt;

  if (!m_menuData->parent)
  {
    m_menuData->data = (char *)new MenuTable;
    m_menuData->id = (int)(((MenuTable *)m_menuData->data)->hMenu = CreateMenu());
    m_menuData->type = MF_POPUP;
  }
  else
  {
    mt = (MenuTable *)Top()->m_menuData->data;
    m_menuData->id = mt->InsertEntry(this);
    if (m_menuData->sub)
    {
      m_menuData->id = (int)CreatePopupMenu();
      m_menuData->type |= MF_POPUP;
    }
    unsigned int pos = 0;
    for (Menu *menu = Prev(); menu; pos++, menu = menu->Prev());
    InsertMenu((HMENU)Parent()->m_menuData->id, pos,
      MF_BYPOSITION | m_menuData->type, (UINT)m_menuData->id, m_menuData->data);
  }
  for (Menu *m = Sub(); m; m = m->Next()) m->Activate();
}

WO_EXPORT void DrawMenuBar2(Window *w) {DrawMenuBar(w->m_windowData->hWnd);} //TEMP

void Menu::Enable(bool state)
{
  if (m_menuData->type & MF_POPUP || !(m_menuData->type & MF_GRAYED) == !!state) return;
  m_menuData->type ^= MF_GRAYED;
  MenuTable *mt = (MenuTable *)Top()->m_menuData->data;
  if (mt) EnableMenuItem(mt->hMenu, mt->GetMenuID(this), m_menuData->type & MF_GRAYED);
}

bool Menu::IsEnabled() const
{
  return !(m_menuData->type & MF_GRAYED);
}

void Menu::Disable() {Enable(false);}

void Menu::Enable() {Enable(true);}

bool Menu::Check(bool state)
{
  if (!(m_menuData->type & MF_POPUP) && !(m_menuData->type & MF_CHECKED) == state)
  {
    m_menuData->type ^= MF_CHECKED;
    MenuTable *mt = (MenuTable *)Top()->m_menuData->data;
    if (mt) CheckMenuItem(mt->hMenu, mt->GetMenuID(this), m_menuData->type & MF_CHECKED);
  }
  return state;
}

bool Menu::IsChecked() const
{
  return !!(m_menuData->type & MF_CHECKED);
}

const char *Menu::operator=(const char *text)
{
  MenuData *md = m_menuData;
  if (!md->parent) return text;
  if (!(md->type & (MF_BITMAP | MF_OWNERDRAW))) delete [] md->data; // string
  md->type &= ~(MF_BITMAP | MF_OWNERDRAW);
  if (text) strcpy(md->data = new char[strlen(text) + 1], text);
  else text = NULL;
  MenuTable *mt = (MenuTable *)Top()->m_menuData->data;
  UINT id = mt->GetMenuID(this);
  ModifyMenu((HMENU)Parent()->m_menuData->id, id, md->type, id, text);
  return text;
}

int Menu::GetText(char *buff) const
{
  MenuData *md = m_menuData;
  const char *menuText;
  if (md->type & MF_BITMAP) menuText = "[bitmap]";
  else if (md->type & MF_OWNERDRAW) menuText = "[owner draw]";
  else menuText = m_menuData->data;
  const char *ptr = strchr(menuText, '\t');
  int n;
  if (ptr) n = ptr - menuText;
  else n = strlen(menuText);
  if (buff)
  {
    memcpy(buff, menuText, n);
    buff[n] = '\0';
  }
  return n;
}

Menu::CaptionType Menu::Type() const
{
  if (m_menuData->type & MF_BITMAP) return Menu::bitmap;
  if (m_menuData->type & MF_OWNERDRAW) return Menu::ownerDraw;
  return Menu::text;
}

void Menu::Selected() {}

Point Menu::GetSize() const {return Point(0, 0);}

void Menu::Exposed(DrawingSurface &, const Rect &, bool) {}

SimpleMenu::SimpleMenu(const char *text, SelectFunc ma, bool greyed, bool checked) : Menu(text, greyed, checked)
{
  m_ma = ma;
}

SimpleMenu::SimpleMenu(const Bitmap *bitmap, SelectFunc ma, bool greyed, bool checked) : Menu(bitmap, greyed, checked)
{
  m_ma = ma;
}

SimpleMenu::SimpleMenu(SelectFunc ma, bool greyed, bool checked) : Menu(greyed, checked)
{
  m_ma = ma;
}

void SimpleMenu::Selected()
{
  if (m_ma) (*m_ma)(this);
}

bool ProcessMenuMessage(LRESULT &lRes, WindowData *windowData, UINT message, WPARAM wParam, LPARAM lParam)
{
  if (!windowData->menu) return false;
  MenuTable *mt = (MenuTable *)windowData->menu->m_menuData->data;
  if (!mt) return false;
  if (message == WM_COMMAND)
  {
    if (wParam & 0xFFFE0000 || lParam || LOWORD(wParam) < 3 ||
      LOWORD(wParam) > mt->nItems + 2) return false;
    mt->item[LOWORD(wParam) - 3]->Selected(); // menu selection
    lRes = 0;
    return true;
  }
  if (wParam) return false;
  if (message == WM_DRAWITEM)
  {
    DRAWITEMSTRUCT *dis = (DRAWITEMSTRUCT *)lParam;
    if (dis->CtlType != ODT_MENU || dis->itemID < 3 || (int)dis->itemID > mt->nItems + 2)
      return false;
    SaveDC(dis->hDC);
    SetBkMode(dis->hDC, TRANSPARENT);
    DSData DSD = {1, dis->hDC, NULL}, *pDSD = &DSD;
    mt->item[dis->itemID - 3]->Exposed(*(DrawingSurface *)&pDSD,
      Rect(dis->rcItem.left, dis->rcItem.top, dis->rcItem.right, dis->rcItem.bottom),
      !!(dis->itemState & ODS_SELECTED));
    RestoreDC(dis->hDC, -1);
    lRes = TRUE;
    return true;
  }
  if (message != WM_MEASUREITEM) return false;
  MEASUREITEMSTRUCT *mis = (MEASUREITEMSTRUCT *)lParam;
  if (mis->CtlType != ODT_MENU || mis->itemID < 3 || (int)mis->itemID > mt->nItems + 2)
    return false;
  Point pt = mt->item[mis->itemID - 3]->GetSize();
  mis->itemWidth = pt.x;
  mis->itemHeight = pt.y;
  lRes = TRUE;
  return true;
}

