#define WINDOBJ_MAIN

#include <WindObj/MessageScroll.h>
#include <WindObj/Private.h>

MessageScroll::MessageScroll(Window *parent, int nMessageMax, MessageScroll::Type type, bool newestAtTop) : m_type(type),
  Window(parent,  Rect(0, 0, 1, 1)), m_nMessages(0), m_nMessageMax(nMessageMax), m_message(NULL), m_focalRow(-1), 
  m_anchorRow(-1), m_newestAtTop(newestAtTop)
{
  m_textHeight = Font().TextHeight();
}

MessageScroll::~MessageScroll()
{
  if (!m_message) return;
  for (int i = 0; i < m_nMessages; i++)
    m_message[i].Clear();
  ClientFree(m_message);
}

void MessageScroll::AddMessage(const char *text, const Colour &colour, const Colour &backColour)
{
  AddMessage(text, text? strlen(text): 0, colour, backColour);
}

void MessageScroll::AddMessage(const char *text, int nText, const Colour &colour, const Colour &backColour)
{
  Rect canvas = Canvas();
  int canvasW = canvas.right - canvas.left, tw;
  Font font;
  if (m_nMessageMax > 0 && m_nMessages == m_nMessageMax)
  {
// Max no. messages reached. Delete one off the end.
    tw = font.TextWidth(m_message[m_newestAtTop? m_nMessages - 1: 0].message);
    if (tw == canvasW)
    {
      canvasW = 0;
      for (int i = (m_newestAtTop? 0: 1); i < (m_newestAtTop? m_nMessages - 1: m_nMessages); i++)
      {
        int tw1 = font.TextWidth(m_message[i].message);
        if (tw1 > canvasW) canvasW = tw1;
      }
    }
    m_message[m_newestAtTop? m_nMessages - 1: 0].Clear();
    memmove(m_message + (m_newestAtTop? 1: 0), m_message + (m_newestAtTop? 0: 1), (m_nMessages - 1) * sizeof(MSMessage));
  }
  else
  {
    if (!(m_nMessages % 100))
    {
      m_message = (MSMessage *)ClientRealloc(m_message, (m_nMessages + 100) * sizeof(MSMessage));
    }
    if (m_newestAtTop) memmove(m_message + 1, m_message, m_nMessages * sizeof(MSMessage));
    m_nMessages++;
    canvas.bottom += m_textHeight;
  }
  int newIdx = m_newestAtTop? 0: m_nMessages - 1;
  MSMessage &msg = m_message[newIdx];
  memcpy(msg.message = new char[nText + 1], text, nText);
  msg.message[nText] = '\0';
  msg.colour = colour;
  msg.backColour = backColour;
  msg.selected = false;
  tw = font.TextWidth(msg.message);
  if (tw > canvasW) canvasW = tw;
/* If the top item is visible then the list scrolls down below it when the new item is added. If it is not visible
   then the scrolls are adjusted so that the visible portion of the list is unchanged ... */
  if (m_newestAtTop)
  {
    if (canvas.top > m_textHeight)
    {
/*  canvas.top -= m_textHeight;
    Rect clientRect = ClientRect();
    int clientHeight = clientRect.bottom - clientRect.top;
    if ((canvas.bottom -= m_textHeight) > clientHeight)
    {
      int h = canvas.bottom - canvas.top;
      canvas.top = (canvas.bottom = clientHeight) - h;
    }
    */
      canvas.top += m_textHeight;
      canvas.bottom += m_textHeight;
    }
  }
  else
  {
    Rect clientRect = ClientRect();
    int d = canvas.bottom - (canvas.top + clientRect.bottom);
    if (d > 0 && d < 2 * m_textHeight)
    {
      canvas.top += d;
      canvas.bottom += d;
    }
  }
  SetCanvas(0, canvas.top, canvasW, canvas.bottom - canvas.top);
  Draw();
}

void MessageScroll::SetMessageMax(int nMessageMax)
{
  m_nMessageMax = nMessageMax;
  if (m_nMessages <= nMessageMax) return;
  for (int i = nMessageMax; i < m_nMessages; i++)
    m_message[i].Clear();
  m_nMessages = nMessageMax;
  Draw();
}

void MessageScroll::Exposed(DrawingSurface &DS, const Rect &invRect)
{
  DS.Rectangle(invRect, Pen(), Brush(Colour::window));
  int i = max(invRect.top / m_textHeight, 0),
    n = (invRect.bottom + m_textHeight - 1) / m_textHeight;
  if (n > m_nMessages) n = m_nMessages;
  for (; i < n; i++)
  {
    Colour textColour = m_message[i].selected? m_message[i].backColour: m_message[i].colour,
      backColour = m_message[i].selected? m_message[i].colour: m_message[i].backColour;
    DS.Rectangle(Rect(0, i * m_textHeight, invRect.right, (i + 1) * m_textHeight), Pen(), Brush(backColour));
    DS.Text(m_message[i].message, Point(0, i * m_textHeight), DefFont, textColour);
  }
}

void MessageScroll::Sized() {Draw();}

void MessageScroll::MouseLButtonReleased(int x, int y, int flags)
{
  if (m_type == MST_readOnly) return;
  int idx = y / m_textHeight;
  if (idx < 0 || idx >= m_nMessages) return;
  if (flags & MM_control)
  {
    m_message[idx].selected = !m_message[idx].selected;
    m_focalRow = m_anchorRow = idx;
  }
  else if (flags & MM_shift)
  {
    if (m_anchorRow == -1) m_anchorRow = idx;
    m_focalRow = idx;
    int range1 = min(m_anchorRow, m_focalRow), range2 = max(m_anchorRow, m_focalRow);
    for (int i = 0; i < m_nMessages; i++)
      m_message[i].selected = i >= range1 && i <= range2;
  }
  else
  {
    for (int i = 0; i < m_nMessages; i++)
      m_message[i].selected = i == idx;
    m_focalRow = m_anchorRow = idx;
  }
  Draw();
}

void MessageScroll::KeyPressed(VirtualKeyCode VKC, char, unsigned short repeat)
{
  if (m_type != MST_deleteRows || m_focalRow == -1 || VKC != VKC_delete && VKC != VKC_backspace) return;
  m_anchorRow = m_focalRow = -1;
  int j = 0;
  for (int i = 0; i < m_nMessages; i++)
  {
    if (!m_message[i].selected)
    {
      if (i > j) m_message[j] = m_message[i];
      j++;
    }
  }
  m_nMessages = j;
  Draw();
}

MSMessage::MSMessage() : colour(Colour::highlightText), backColour(Colour::highlight), selected(false), message(NULL)
{
}

MSMessage::MSMessage(const MSMessage &msg) : colour(msg.colour), backColour(msg.backColour), selected(msg.selected)
{
  int n = msg.message? strlen(msg.message): 0;
  if (n == 0) message = NULL;
  else strcpy(message = new char[n + 1], msg.message);
}

MSMessage::~MSMessage() {delete [] message;}

MSMessage &MSMessage::operator=(const MSMessage &msg)
{
  colour = msg.colour;
  backColour = msg.backColour;
  selected = msg.selected;
  delete [] message;
  int n = msg.message? strlen(msg.message): 0;
  if (n == 0) message = NULL;
  else strcpy(message = new char[n + 1], msg.message);
  return *this;
}

void MSMessage::Clear()
{
  delete [] message;
  message = NULL;
}
