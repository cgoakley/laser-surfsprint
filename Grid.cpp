#define WINDOBJ_MAIN

#include <WindObj/Grid.h>
#include <WindObj/PopupMenu.h>
#include <WindObj/Private.h>

#include <limits.h>
#include <malloc.h>
#include <stdio.h>

Grid::Grid(Window *parent, Rect &rect, int nRows, int nColumns, int cellAlloc, Font *defFont, int flags,
           bool allowUserResize, bool rightMouseFormatMenu) : 
  Window(parent, rect, flags), m_gridLinePen(Pen(Colour::greyText)), m_nFrozenRows(0), m_nFrozenColumns(0),
  m_allowUserResize(allowUserResize), m_rightMouseFormatMenu(rightMouseFormatMenu)

{
  m_selRow = m_selColumn = -1;
  m_nFonts = 1;
  m_font = (Font **)ClientMalloc(sizeof(Font *));
  m_font[0] = defFont? new Font(*defFont): new Font(Font::variable);
  m_defHeight = 3 * m_font[0]->TextHeight() / 2;
  m_defWidth = 3 * m_defHeight;
  m_defForeground = Colour(Colour::windowText);
  m_defBackground = Colour(Colour::window);
  m_x = (int *)ClientMalloc(((m_nColumns = nColumns) + 1) * sizeof(int));
  m_y = (int *)ClientMalloc(((m_nRows = nRows) + 1) * sizeof(int));
  int nCells = nColumns * nRows;
  m_cellAlloc = max(nCells, cellAlloc);
  m_cell = (Cell *)ClientMalloc(m_cellAlloc * sizeof(Cell));
  int i;
  for (i = 0; i <= nColumns; i++) m_x[i] = i * m_defWidth;
  for (i = 0; i <= nRows; i++) m_y[i] = i * m_defHeight;
  for (i = 0; i < nCells; i++)
  {
    m_cell[i].m_fontNo = 0;
    m_cell[i].m_backColour = m_defBackground;
    m_cell[i].m_textColour = m_defForeground;
    m_cell[i].m_text = NULL;
    m_cell[i].m_flags = '\0';
  }
  SetCanvas();
}

Grid::~Grid()
{
  if (m_x) ClientFree(m_x);
  if (m_y) ClientFree(m_y);
  int i, n = m_nRows * m_nColumns;
  for (i = 0; i < n; i++)
		delete [] m_cell[i].m_text;
  ClientFree(m_cell);
  for (i = 0; i < m_nFonts; i++)
    delete m_font[i];
  if (m_font) ClientFree(m_font);
}

static int SearchInt(int x, int *list, int n)
{
  if (x < list[0]) return -1;
  if (x > list[n]) return n;
  int i1 = 0, i2 = n, i3;
  while (i2 > i1 + 1)
  {
    i3 = (i1 + i2) / 2;
    if (x > list[i3]) i1 = i3;
    else i2 = i3;
  }
  return i1;
}

Cell &Grid::operator ()(int row, int column)
{
  if (row < 0 || row >= m_nRows || column < 0 || column >= m_nColumns) throw "Invalid cell reference";
  return m_cell[row * m_nColumns + column];
}

Cell &Grid::operator ()()
{
  if (m_selColumn < 0) throw "No cell selected";
  return m_cell[m_selRow * m_nColumns + m_selColumn];
}

const char *Cell::operator=(const char *text)
{
	delete [] m_text;
  if (text) strcpy(m_text = new char[strlen(text) + 1], text);
  else m_text = NULL;
  return text;
}

Cell::operator char *()
{
  return m_text;
}

char *Cell::AllocText(int nText)
{
  delete [] m_text;
  m_text = new char[nText + 1];
  memset(m_text, 0, nText + 1);
  return m_text;
}  

void Grid::AllocCells(int nCells) // to allocate space for cells in advance
{
  if (m_cellAlloc >= nCells) return;
  Cell *cell = (Cell *)ClientRealloc(m_cell, nCells * sizeof(Cell));
  if (!cell) throw "Out of memory";
  m_cellAlloc = nCells;
  m_cell = cell;
}

void Grid::SetCanvas()
{
  Window::SetCanvas(m_x[m_nColumns], m_y[m_nRows]);
}

Cell *Grid::InsertRows(int rowNo, int nRows)
{
  AllocCells((m_nRows + nRows) * m_nColumns);
  if (rowNo < 0) rowNo = 0;
  else if (rowNo > m_nRows) rowNo = m_nRows;
  Cell *cell = m_cell + rowNo * m_nColumns;
  memmove(cell + nRows * m_nColumns, cell, (m_nRows - rowNo) * m_nColumns * sizeof(Cell));
  int i;
  for (i = 0; i < m_nColumns * nRows; i++)
  {
    cell[i].m_fontNo = 0;
    cell[i].m_backColour = m_defBackground;
    cell[i].m_textColour = m_defForeground;
    cell[i].m_text = NULL;
    cell[i].m_flags = '\0';
  }
  if (rowNo > 0 && m_nRows > rowNo) // Continue "linked above" links through inserted rows
  {
    Cell *cell1 = cell + m_nColumns * nRows;
    for (i = 0; i < m_nColumns; i++)
    {
      if (!(cell1[i].m_flags & Cell_linkedAbove)) continue;
      for (int j = 0; j < nRows; j++)
        cell[i + j * m_nColumns].m_flags = Cell_linkedAbove;
    }
  }
  m_y = (int *)ClientRealloc(m_y, (m_nRows + 1 + nRows) * sizeof(int));
  memmove(m_y + rowNo + 1 + nRows, m_y + rowNo + 1, (m_nRows - rowNo) * sizeof(int));
  m_nRows += nRows;
  for (i = 1; i <= nRows; i++) m_y[rowNo + i] = m_y[rowNo] + i * m_defHeight;
  for (i = rowNo + nRows + 1; i <= m_nRows; i++)
    m_y[i] += nRows * m_defHeight;
  SetCanvas();
  if (m_selColumn >= 0 && m_selRow >= rowNo) m_selRow += nRows;
  return cell;
}

void Grid::DeleteRows(int rowNo, int nRows)
{
  if (rowNo < 0) rowNo = 0;
  nRows = min(nRows, m_nRows - rowNo);
  if (nRows <= 0) return;
  Cell *cell = m_cell + rowNo * m_nColumns;
  int i;
  if (rowNo + nRows < m_nRows)
  {
    for (i = 0; i < m_nColumns; i++)
      if (!(cell[i].m_flags & Cell_linkedAbove))
        cell[nRows * m_nColumns + i].m_flags &= ~Cell_linkedAbove;
  }
  for (i = 0; i < nRows * m_nColumns; i++)
    delete [] cell[i].m_text;
  memmove(cell, cell + nRows * m_nColumns, ((m_nRows -= nRows) - rowNo) * m_nColumns * sizeof(Cell));
  int rh = m_y[rowNo + nRows] - m_y[rowNo];
  memmove(m_y + rowNo + 1, m_y + rowNo + 1 + nRows, (m_nRows - rowNo) * sizeof(int));
  for (i = rowNo + 1; i <= m_nRows; i++) m_y[i] -= rh;
  SetCanvas();
  if (m_selColumn >= 0 && m_selRow >= rowNo + nRows) m_selRow -= nRows;
  else if (m_selRow >= rowNo && m_selRow < rowNo + nRows) Deselect();
}

Cell *Grid::InsertColumns(int columnNo, int nColumns)
{
  int i, j;
  if (columnNo < 0) columnNo = 0;
  else if (columnNo > m_nColumns) columnNo = m_nColumns;
  if (columnNo < m_nColumns)
  {
    for (i = 0; i < m_nRows; i++)
      (*this)(i, columnNo).m_flags &= ~Cell_tempLinkedLeft;
  }
  AllocCells(m_nRows * (m_nColumns + nColumns));
  Cell *cell = m_cell + columnNo;
  memmove(cell + (m_nColumns + nColumns) * (m_nRows - 1) + nColumns,
    cell + m_nColumns * (m_nRows - 1), (m_nColumns - columnNo) * sizeof(Cell));
  for (i = m_nRows - 2; i >= 0; i--)
    memmove(cell + i * (m_nColumns + nColumns) + nColumns, 
      cell + i * m_nColumns, m_nColumns * sizeof(Cell));
  m_nColumns += nColumns;
  for (i = 0; i < m_nRows; i++)
  {
    Cell *cell1 = cell + i * m_nColumns,
      *prevCell = columnNo > 0? cell1 - 1: NULL, *nextCell = columnNo < m_nColumns - nColumns? cell1 + nColumns: NULL;
    short refFontNo = 0;
    Colour refBackColour = m_defBackground, refTextColour = m_defForeground;
    char refFlags = '\0';
    Cell *refCell = !prevCell || nextCell && nextCell->m_flags & Cell_linkedLeft? nextCell: prevCell;
    CheckTextSpill(i, columnNo - 1);
    if (refCell)
    {
      refFontNo = refCell->m_fontNo;
      refBackColour = refCell->m_backColour;
      refTextColour = refCell->m_textColour;
      refFlags = refCell->m_flags;
      if (refCell == prevCell) refFlags &= ~Cell_linkedLeft;
    }
    for (j = 0; j < nColumns; j++)
    {
      Cell &tgtCell = cell1[j];
      tgtCell.m_fontNo = refFontNo;
      tgtCell.m_backColour = refBackColour;
      tgtCell.m_flags = refFlags;
      tgtCell.m_text = NULL;
      tgtCell.m_textColour = refTextColour;
    }
  }
  m_x = (int *)ClientRealloc(m_x, (m_nColumns + 1) * sizeof(int));
  memmove(m_x + columnNo + 1 + nColumns, m_x + columnNo + 1, (m_nColumns - columnNo - nColumns) * sizeof(int));
  for (i = 1; i <= nColumns; i++) m_x[columnNo + i] = m_x[columnNo] + i * m_defWidth;
  for (i = columnNo + nColumns + 1; i <= m_nColumns; i++)
    m_x[i] += nColumns * m_defWidth;
  SetCanvas();
  if (m_selColumn >= columnNo) m_selColumn += nColumns;
  return cell;
}

void Grid::DeleteColumns(int columnNo, int nColumns)
{
  if (columnNo < 0) columnNo = 0;
  if (columnNo >= m_nColumns) return;
  nColumns = min(nColumns, m_nColumns - columnNo);
  if (nColumns <= 0) return;
  int i, j;
  for (i = 0; i < m_nRows; i++)
    for (j = columnNo; j < columnNo + nColumns; j++)
      delete [] (char *)((*this)(i, j));
  int newNumColumns = m_nColumns - nColumns;
  for (i = 0; i < m_nRows - 1; i++)
    memmove(m_cell + columnNo + i * newNumColumns, 
      m_cell + columnNo + nColumns + i * m_nColumns, newNumColumns * sizeof(Cell));
  memmove(m_cell + columnNo + (m_nRows - 1) * newNumColumns, 
    m_cell + columnNo + nColumns + (m_nRows - 1) * m_nColumns, 
      (newNumColumns - columnNo) * sizeof(Cell));
  int cw = m_x[columnNo + nColumns] - m_x[columnNo];
  memmove(m_x + columnNo + 1, m_x + columnNo + 1 + nColumns, (newNumColumns - columnNo) * sizeof(int));
  for (i = columnNo + 1; i <= m_nColumns; i++) m_x[i] -= cw;
  m_nColumns = newNumColumns;
  SetCanvas();
  if (m_selColumn > columnNo + nColumns) m_selColumn -= nColumns;
  else if (m_selColumn >= columnNo) Deselect();
  if (columnNo < m_nColumns - 1)
  {
    for (i = 0; i < m_nRows; i++)
    {
      (*this)(i, columnNo).m_flags &= ~Cell_linkedLeft; // remove links to left
      CheckTextSpill(i, columnNo);
    }
  }
}

int Grid::ColWidth(int col)
{
  return m_x[col + 1] - m_x[col];
}

static int RemapCoord(int *coord, int changedCoord, int change, int value)
{
  if (!change) return value;
  int x1 = coord[changedCoord], x2p = coord[changedCoord + 1], x2 = x2p - change;
  if (value <= x1) return value;
  if (value >= x2) return value + change;
  return x1 + (value - x1) * (x2p - x1) / (x2 - x1);
}

/* When column widths or row heights are changed, child windows appearing on the sheet need to
   be resized to follow these changes ... */

static void RealignGridChildWindows(Grid *grid, int changedCol, int dWidth, int changedRow, int dHeight)
{
  for (int i = 0; i < grid->NumChildren(); i++)
  {
    Rect rect = grid->Child(i)->BoundingRect() - grid->ClientOrigin();
    rect.left = RemapCoord(grid->m_x, changedCol, dWidth, rect.left);
    rect.top = RemapCoord(grid->m_y, changedRow, dHeight, rect.top);
    rect.right = RemapCoord(grid->m_x, changedCol, dWidth, rect.right);
    rect.bottom = RemapCoord(grid->m_y, changedRow, dHeight, rect.bottom);
    grid->Child(i)->BoundingRect(rect + grid->ClientOrigin());
    grid->Child(i)->Draw();
  }
}

void Grid::ColWidth(int col, int width)
{
  int i, d = width - (m_x[col + 1] - m_x[col]);
  for (i = col + 1; i <= m_nColumns; i++)
    m_x[i] += d;
  for (i = 0; i < m_nRows; i++) CheckTextSpill(i, col);
  RealignGridChildWindows(this, col, d, 0, 0);
  SetCanvas();
}

int Grid::RowHeight(int row)
{
  return m_y[row + 1] - m_y[row];
}

void Grid::RowHeight(int row, int height)
{
  int i, d = height - (m_y[row + 1] - m_y[row]);
  for (i = row + 1; i <= m_nRows; i++)
    m_y[i] += d;
  RealignGridChildWindows(this, 0, 0, row, d);
  SetCanvas();
}

static int CellTextWidth(Grid *grid, int row, int column)
{
  Cell &cell = (*grid)(row, column);
  if (!cell.m_text || !cell.m_text[0]) return 0;
  Font *font = grid->m_font[cell.m_fontNo];
  return font->TextWidth(cell) + font->TextWidth("M") * 2 / 3;
}

static int CellTextHeight(Grid *grid, int row, int column)
{
  Cell &cell = (*grid)(row, column);
  return 3 * grid->m_font[cell.m_fontNo]->TextHeight() / 2;
}

void Grid::AutoFitText()
{
  memset(m_x, 0, (m_nColumns + 1) * sizeof(int));
  memset(m_y, 0, (m_nRows + 1) * sizeof(int));
  int row, column, i;
  bool hasLinkedRows = false, hasLinkedColumns = false;
  for (row = 0; row < m_nRows; row++)
  {
    for (column = 0; column < m_nColumns; column++)
    {
      Cell &cell = (*this)(row, column);
      cell.m_flags &= ~Cell_tempLinkedLeft;
      if (!(cell.m_flags & Cell_linkedLeft) &&
        (column == m_nColumns - 1 || !((*this)(row, column + 1).m_flags & Cell_linkedLeft)))
      {
        int w = CellTextWidth(this, row, column);
        if (w > m_x[column]) m_x[column] = w;
      }
      else hasLinkedColumns = true;
      if (!(cell.m_flags & Cell_linkedAbove) &&
        (row == m_nRows - 1 || !((*this)(row + 1, column).m_flags & Cell_linkedAbove)))
      {
        int h = CellTextHeight(this, row, column);
        if (h > m_y[row]) m_y[row] = h;
      }
      else hasLinkedRows = true;
    }
  }
  if (hasLinkedRows || hasLinkedColumns)
  {
    for (row = 0; row < m_nRows; row++)
    {
      for (column = 0; column < m_nColumns; column++)
      {
        Cell &cell = (*this)(row, column);
        if (!(cell.m_flags & Cell_linkedLeft))
        {
          int column1 = column + 1;
          for (; column1 < m_nColumns && (*this)(row, column1).m_flags & Cell_linkedLeft; column1++);
          int numCols = column1 - column;
          if (numCols > 1)
          {
            int w = 0;
            for (i = column; i < column1; i++) w += CellTextWidth(this, row, i) - m_x[i];
            if (w > 0)
            {
              w = (w + numCols - 1) / numCols;
              for (i = column; i < column1; i++) m_x[i] += w;
            }
          }
        }
        if (!(cell.m_flags & Cell_linkedAbove))
        {
          int row1 = row + 1;
          for (; row1 < m_nRows && (*this)(row1, column).m_flags & Cell_linkedAbove; row1++);
          int numRows = row1 - row;
          if (numRows > 1)
          {
            int h = 0;
            for (i = row; i < row1; i++) h += CellTextHeight(this, i, column) - m_y[i];
            if (h > 0)
            {
              h = (h + numRows - 1) / numRows;
              for (i = row; i < row1; i++) m_y[i] += h;
            }
          }
        }
      }
    }
  }
  for (i = 0; i < m_nColumns; i++)
    if (m_x[i] == 0) m_x[i] = m_defWidth;
  for (i = 0; i < m_nColumns; i++)
    m_x[i + 1] += m_x[i];
  memmove(m_x + 1, m_x, m_nColumns * sizeof(int));
  m_x[0] = 0;
  for (i = 0; i < m_nRows; i++)
    if (m_y[i] == 0) m_y[i] = m_defHeight;
  for (i = 0; i < m_nRows; i++)
    m_y[i + 1] += m_y[i];
  memmove(m_y + 1, m_y, m_nRows * sizeof(int));
  m_y[0] = 0;
  SetCanvas();
}

void Grid::AutoFitWidth(int column)
{
  int row, d = INT_MIN, row1, column1, row2, column2;
  for (row = 0; row < m_nRows; row = row2 + 1)
  {
    CellGroup(row1, column1, row2, column2, row, column);
    int textWidth = CellTextWidth(this, row1, column1);
    if (textWidth)
    {
      int d1 = textWidth - (m_x[column2 + 1] - m_x[column1]);
      if (d1 > d) d = d1;
    }
  }
  if (d > INT_MIN) ColWidth(column, d + m_x[column + 1] - m_x[column]);
  SetCanvas();
}

void Grid::AutoFitHeight(int row)
{
  int column, d = INT_MIN, row1, column1, row2, column2;
  for (column = 0; column < m_nColumns; column = column2 + 1)
  {
    CellGroup(row1, column1, row2, column2, row, column);
    int textHeight = CellTextHeight(this, row1, column1);
    if (textHeight)
    {
      int d1 = textHeight - (m_y[row2 + 1] - m_y[row1]);
      if (d1 > d) d = d1;
    }
  }
  if (d > INT_MIN) RowHeight(row, d + m_y[row + 1] - m_y[row]);
  SetCanvas();
}

void Grid::AutoFitText(int row, int column)
{
  int row1, column1, row2, column2;
  CellGroup(row1, column1, row2, column2, row, column);
  int w = CellTextWidth(this, row1, column1), colWidth = m_x[column2 + 1] - m_x[column1];
  if (w != colWidth) ColWidth(column, w);
  int h = CellTextHeight(this, row1, column1), rowHeight = m_y[row2 + 1] - m_y[row1];
  if (h != rowHeight) RowHeight(row, h);
  SetCanvas();
}

void Grid::AutoFitText(int row, int column, const char *text)
{
  (*this)(row, column) = text;
  AutoFitText(row, column);
}

Rect Grid::CellRect(int row, int column)
{
  int row1, column1, row2, column2;
  if (!CellGroup(row1, column1, row2, column2, row, column))
    return Rect(0, 0, 0, 0);
  return Rect(m_x[column1], m_y[row1], m_x[column2 + 1] + 1, m_y[row2 + 1] + 1);
}

Rect TwoPixBorder(const Rect &rect)
{
  return Rect(rect.left - 2, rect.top - 2, rect.right + 2, rect.bottom + 2);
}

void Grid::Select(int row, int column)
{
  int row1, column1, row2, column2;
  if (!CellGroup(row1, column1, row2, column2, row, column))
  {
    row1 = column1 = -1;
  }
  if (column1 != m_selColumn || row1 != m_selRow && m_selColumn != -1)
  {
    if (m_selRow >= 0 && m_selColumn >= 0)
    {
      int oldSelRow = m_selRow, oldSelColumn = m_selColumn;
      m_selRow = m_selColumn = -1;
      Draw(TwoPixBorder(CellRect(oldSelRow, oldSelColumn)));
    }
    DrawCell(DrawingSurface(*this), m_selRow = row1, m_selColumn = column1);
  }
}

void Grid::Deselect()
{
  m_selRow = m_selColumn = -1;
}

void Grid::CellCoords(int &row, int &column, Point pt)
{
  row = SearchInt(pt.y, m_y, m_nRows);
  column = SearchInt(pt.x, m_x, m_nColumns);
}

bool Grid::IsLinked(short row, short column)
{
  return (*this)(row, column).m_flags & (Cell_linkedLeft | Cell_linkedAbove) ||
    row < m_nRows - 1 && (*this)(row + 1, column).m_flags & Cell_linkedAbove ||
    column < m_nColumns - 1 && (*this)(row, column + 1).m_flags & Cell_linkedLeft;
}

bool Grid::CellGroup(int &row1, int &column1, int &row2, int &column2, int row, int column,
                     bool includeTextSpill)
{
  if (row < 0 || column < 0 || row >= m_nRows || column >= m_nColumns) return false;
  char leftLink = Cell_linkedLeft;
  if (includeTextSpill) leftLink |= Cell_tempLinkedLeft;
  row1 = row2 = row;
  column1 = column2 = column;
  while ((*this)(row1, column1).m_flags & leftLink) column1--;
  while ((*this)(row1, column1).m_flags & Cell_linkedAbove) row1--;
  while (column2 + 1 < m_nColumns && (*this)(row2, column2 + 1).m_flags & leftLink) column2++;
  while (row2 + 1 < m_nRows && (*this)(row2 + 1, column2).m_flags & Cell_linkedAbove) row2++;
  return true;
}

int Grid::GroupWidth(int row, int column)
{
  int row1, column1, row2, column2;
  CellGroup(row1, column1, row2, column2, row, column);
  return column2 - column1 + 1;
}

int Grid::GroupHeight(int row, int column)
{
  int row1, column1, row2, column2;
  CellGroup(row1, column1, row2, column2, row, column);
  return row2 - row1 + 1;
}

static void ClearTextSpill(Grid *grid, int row, int column)
{
  int i;
// Clear existing text spill flags ...
  for (i = column; i > 0 && (*grid)(row, i).m_flags & Cell_tempLinkedLeft; i--)
    (*grid)(row, i).m_flags &= ~Cell_tempLinkedLeft;
  for (i = column + 1; i < grid->m_nColumns && (*grid)(row, i).m_flags & Cell_tempLinkedLeft; i++)
    (*grid)(row, i).m_flags &= ~Cell_tempLinkedLeft;
}

static void SpillText(Grid *grid, int row, int column)
{
  if (grid->IsLinked(row, column)) return;
  Cell &cell = (*grid)(row, column);
  if (!cell.m_text || !cell.m_text[0]) return;
  Font &font = *grid->m_font[cell.m_fontNo];
  int textWidth = font.TextWidth(cell.m_text) + 2 * font.TextWidth("M") / 3;
  int column1 = column;
  while ((textWidth -= grid->ColWidth(column1)) > 0)
  {
    if (++column1 == grid->m_nColumns || grid->IsLinked(row, column1)) break;
    Cell &cell1 = (*grid)(row, column1);
    if (cell1.m_backColour != cell.m_backColour || cell1.m_textColour != cell.m_textColour || 
      cell1.m_fontNo != cell.m_fontNo || cell1.m_flags != cell.m_flags ||
      cell1.m_text && cell1.m_text[0]) break;
    cell1.m_flags |= Cell_tempLinkedLeft;
  }
}

void Grid::CheckTextSpill(int row, int column)
{
  if (row < 0 || row >= m_nRows || column < 0 || column >= m_nColumns) return;
  if (IsLinked(row, column)) return;
  int column1 = column;
  for (; column1 > 0 && (*this)(row, column1).m_flags & Cell_tempLinkedLeft; column1--)
    (*this)(row, column1).m_flags &= ~Cell_tempLinkedLeft;
  int column2 = column + 1;
  for (; column2 < m_nColumns && (*this)(row, column2).m_flags & Cell_tempLinkedLeft; column2++)
    (*this)(row, column2).m_flags &= ~Cell_tempLinkedLeft;
  SpillText(this, row, column1);
  if (column1 < column && (*this)(row, column).m_text && (*this)(row, column).m_text[0])
    SpillText(this, row, column);
}

void Grid::DrawCell(DrawingSurface &DS, int row, int column, bool groupDraw)
{
  int row1, column1, row2, column2;
  if (!CellGroup(row1, column1, row2, column2, row, column, true)) return;
  Rect cellRect(m_x[column1], m_y[row1], m_x[column2 + 1] + 1, m_y[row2 + 1] + 1);
  Cell &cell = (*this)(row1, column1);
  DS.Rectangle(cellRect, m_gridLinePen, Brush(cell.m_backColour));
  if (cell.m_text)
  {
    Font &font = *m_font[cell.m_fontNo];
    int textWidth = font.TextWidth(cell.m_text), textHeight = font.TextHeight(),
      cellWidth = cellRect.right - cellRect.left;
    TextAlignment TA;
/* If text is spilling into neighbouring cell without the cells being explicitly linked, then
   text alignment is always left-justified ... */
    if ((*this)(row1, column2).m_flags & Cell_tempLinkedLeft) TA = TA_left;
    else TA = (TextAlignment)(cell.m_flags & Cell_alignMask);
    Point offset;
    switch (TA)
    {
    case TA_left: offset.x = font.TextWidth("M") / 3; break;
    case TA_right: offset.x = cellRect.right - cellRect.left - textWidth - 
                     font.TextWidth("M") / 3; break;
    case TA_centre: offset.x = (cellRect.right - cellRect.left - textWidth) / 2; break;
    }
    TextVAlignment TVA;
    TVA = (TextVAlignment)((cell.m_flags & Cell_vertAlignMask) >> 2);
    switch (TVA)
    {
    case TA_top: offset.y = textHeight / 4; break;
    case TA_bottom: offset.y = cellRect.bottom - cellRect.top - 5 * textHeight / 4; break;
    case TA_vertCentre: offset.y = (cellRect.bottom - cellRect.top - textHeight) / 2; break;
    }
    DS.Set(cellRect.TopLeft() + offset);
    DS.SetClip(cellRect);
    DS.Text(cell.m_text, DefPoint, font, cell.m_textColour);
    DS.SetClip();
  }
// Draw selection rectangle if it is likely to impinge ...
  int selRow1, selColumn1, selRow2, selColumn2;
  if (CellGroup(selRow1, selColumn1, selRow2, selColumn2, m_selRow, m_selColumn) &&
    !!(Rect(selColumn1, selRow1, selColumn2 + 1, selRow2 + 1) & 
       Rect(column1 - 1, row1 - 1, column2 + 2, row2 + 2)))
  {
    DS.Rectangle(Rect(m_x[selColumn1], m_y[selRow1],
      m_x[selColumn2 + 1] + 2, m_y[selRow2 + 1] + 2), Pen(m_defForeground, 2), Brush());
  }
}

void Grid::Exposed(DrawingSurface &DS, const Rect &invRect)
{
  Rect clientRect = ClientRect();
  int row1, row2, column1, column2;
  CellCoords(row1, column1, invRect.TopLeft()); 
  CellCoords(row2, column2, invRect.BottomRight()); 
  if (row1 < 0) row1 = 0;
  if (row2 < 0) row2 = 0;
  if (column1 < 0) column1 = 0;
  if (column2 < 0) column2 = 0;
  if (column2 >= m_nColumns)
  {
    column2 = m_nColumns - 1;
    Rect rightRect = Rect(m_x[m_nColumns], 0, INT_MAX, INT_MAX) & invRect;
    if (rightRect) DS.Rectangle(rightRect, Pen(), m_defBackground);
  }
  if (row2 >= m_nRows)
  {
    row2 = m_nRows - 1;
    Rect bottomRect = Rect(0, m_y[m_nRows], INT_MAX, INT_MAX) & invRect;
    if (bottomRect) DS.Rectangle(bottomRect, Pen(), m_defBackground);
  }
  if (column1 >= m_nColumns || row1 >= m_nRows) return;
  for (int column = column1; column <= column2; column++)
    for (int row = row1; row <= row2; row++)
    {
      unsigned char flags = (*this)(row, column).m_flags;
      if ((column == column1 || !(flags & (Cell_linkedLeft | Cell_tempLinkedLeft))) &&
        (row == row1 || !(flags & Cell_linkedAbove)))
          DrawCell(DS, row, column, true);
    }
}

struct CellEditBox : EditBox
{
  int m_ignoreKillFocus;

  CellEditBox(Grid *parent, int row, int column) : 
    EditBox(parent, parent->CellRect(row, column) - parent->ClientRect().TopLeft(),
      parent->m_font[(*parent)(row, column).m_fontNo], -1, wantReturn | autoHScroll, 
      (*parent)(row, column), (*parent)(row, column).m_backColour, (*parent)(row, column).m_textColour), m_ignoreKillFocus(0)
  {
    SetSelection(0, -1); // select text in edit box
  }

  void KeyPressed(VirtualKeyCode vkcode, char ascii, unsigned short repeat)
  {
    if (vkcode == VKC_return) LostFocus();
    else if (vkcode == VKC_escape) delete this;
  }

  void LostFocus()
  {
    if (m_ignoreKillFocus)
    {
      m_ignoreKillFocus--;
      return;
    }
    Grid *grid = (Grid *)m_parent;
    char *newText = (char *)alloca(GetText((char *)NULL) + 1);
    GetText(newText);
    try
    {
      grid->ValidateText(newText);
      grid->CellEdited(grid->m_selRow, grid->m_selColumn, newText);
      delete this;
      grid->Draw();
    }
    catch (char *emsg)
    {
// Putting up this error box will trigger another WM_KILLFOCUS, which we need to ignore
      m_ignoreKillFocus++;
      grid->ErrorMessageBox(emsg);
      SetFocus();
      SetSelection(0, -1); // select text
    }
  }
};

static void Select1(Grid *grid, int row, int column)
{
  int row1 = row < 0? 0: row >= grid->m_nRows? grid->m_nRows - 1: row;
  int column1 = column < 0? 0: column >= grid->m_nColumns? grid->m_nColumns - 1: column;
  grid->Select(row1, column1);
  Rect clientRect = grid->ClientRect();
  Rect selRect = grid->CellRect(row1, column1);
  Point offset;
  offset = selRect.TopLeft() - clientRect.TopLeft();
  if (offset.x >= 0)
  {
    offset.x = selRect.right - clientRect.right;
    if (offset.x <= 0) offset.x = 0;
  }
  if (offset.y >= 0)
  {
    offset.y = selRect.bottom - clientRect.bottom;
    if (offset.y <= 0) offset.y = 0;
  }
  if (offset != Point(0, 0)) grid->SetCanvasOrg(offset);
}

void Grid::KeyPressed(VirtualKeyCode vkcode, char ascii, unsigned short repeat)
{
  switch (vkcode)
  {
  case VKC_up:    Select1(this, m_selRow - GroupHeight(m_selRow, m_selColumn), m_selColumn); break;
  case VKC_down:  Select1(this, m_selRow + 1, m_selColumn); break;
  case VKC_left:  Select1(this, m_selRow, m_selColumn - GroupWidth(m_selRow, m_selColumn - 1)); break;
  case VKC_right: Select1(this, m_selRow, m_selColumn + 1); break;
  case VKC_return:
    if (m_selColumn >= 0) EditCell();
    return;
  default: return;
  }
  CellSelected();
}

void Grid::EditCell()
{
  if ((*this)().m_flags & Cell_isReadOnly) return;
  int row1, row2, column1, column2;
  CellGroup(row1, column1, row2, column2, m_selRow, m_selColumn);
  CellEditBox *cellEditBox = new CellEditBox(this, row1, column1);
  cellEditBox->SetFocus();
}

static bool MouseInSizePosition(int &rowNo, int &colNo, Grid *grid, int x, int y)
{
  if (!grid->m_allowUserResize) return false;
  grid->CellCoords(rowNo, colNo, Point(x, y));
  if (colNo < 0 || colNo >= grid->m_nColumns || rowNo < 0 || rowNo >= grid->m_nRows) return false;
  if (x > grid->m_x[colNo + 1] - 2) colNo++;
  else if (x >= grid->m_x[colNo] + 2 || !colNo) colNo = -1;
  if (y > grid->m_y[rowNo + 1] - 2) rowNo++;
  else if (y >= grid->m_y[rowNo] + 2 || !rowNo) rowNo = -1;
  return rowNo != -1 || colNo != -1;
}

static int g_dragRow = -1, g_dragColumn = -1, g_dragX = INT_MIN, g_dragY = INT_MIN; 

static void DrawDragLines(Grid *grid, int x, int y)
{
  DrawingSurface DS(*grid);
  if (x > INT_MIN) DS.Line(Point(x, 0), Point(x, grid->m_y[grid->m_nRows]), 
    Pen(c_white), DefColour, DrawingSurface::opaque, DrawingSurface::xorPen);
  if (y > INT_MIN) DS.Line(Point(0, y), Point(grid->m_x[grid->m_nColumns], y), 
    Pen(c_white), DefColour, DrawingSurface::opaque, DrawingSurface::xorPen);
}

void Grid::MouseLButtonPressed(int x, int y, int flags)
{
  if (!MouseInSizePosition(g_dragRow, g_dragColumn, this, x, y)) return;
  DrawDragLines(this, g_dragX = g_dragColumn == -1? INT_MIN: x, g_dragY = g_dragRow == -1? INT_MIN: y);
  SetMouseCapture();
}

void Grid::MouseLButtonReleased(int x, int y, int flags)
{
  if (g_dragX > INT_MIN || g_dragY > INT_MIN)
  {
    DrawDragLines(this, g_dragX, g_dragY);
    ReleaseMouseCapture();
    int i, deltaX = 0, deltaY = 0;
    if (g_dragX > INT_MIN)
    {
      deltaX = g_dragX - m_x[g_dragColumn];
      for (i = g_dragColumn; i <= m_nColumns; i++)
        m_x[i] += deltaX;
      for (i = 0; i < m_nRows; i++)
        CheckTextSpill(i, g_dragColumn - 1);
      ColumnResized(g_dragColumn);
    }
    if (g_dragY > INT_MIN)
    {
      deltaY = g_dragY - m_y[g_dragRow];
      for (i = g_dragRow; i <= m_nRows; i++)
        m_y[i] += deltaY;
      RowResized(g_dragRow);
    }
    RealignGridChildWindows(this, g_dragColumn - 1, deltaX, g_dragRow - 1, deltaY);
    g_dragX = g_dragY = INT_MIN;
    SetCanvas();
    Draw();
  }
  else
  {
    SetFocus();
    int row, column;
    CellCoords(row, column, Point(x, y));
    Select(row, column);
    Draw();
    CellClicked();
    CellSelected();
  }
}

void Grid::MouseLButtonDblClicked(int x, int y, int flags)
{
  int row, column;
  if (MouseInSizePosition(row, column, this, x, y))
  {
    if (row > 0) AutoFitHeight(row - 1);
    if (column > 0) AutoFitWidth(column - 1);
  }
  else if (m_selRow >= 0 && m_selColumn >= 0) CellDblClicked();
}

void Grid::MouseMoved(int x, int y)
{
  if (g_dragX == INT_MIN && g_dragY == INT_MIN) return;
  DrawDragLines(this, g_dragX, g_dragY);
  if (g_dragX > INT_MIN) g_dragX = max(x, m_x[g_dragColumn - 1] + 5);
  if (g_dragY > INT_MIN) g_dragY = max(y, m_y[g_dragRow - 1] + 5);
  DrawDragLines(this, g_dragX, g_dragY);
}

Cursor g_sizeHCursor(Cursor::arrowEW), g_sizeVCursor(Cursor::arrowNS),
  g_diagCursor(Cursor::arrowNWSE);

void Grid::SetCursor(Cursor *&cursor)
{
  int rowNo, colNo;
  Point p = PointerPosition() - ClientOrigin();
  if (MouseInSizePosition(rowNo, colNo, this, p.x, p.y))
  {
    if (rowNo == -1 && colNo >= 0) cursor = &g_sizeHCursor;
    else if (rowNo >= 0 && colNo == -1) cursor = &g_sizeVCursor;
    else cursor = &g_diagCursor;
  }
}

void Grid::CellClicked() {}

void Grid::CellDblClicked()
{
  EditCell();
}

void Grid::CellRClicked(int, int) {}

void Grid::CellRDblClicked(int, int) {}

void Grid::CellSelected() {}

void Grid::CellEdited(int row, int column, const char *newText)
{
  int row1, column1, row2, column2;
  CellGroup(row1, column1, row2, column2, row, column);
  (*this)(row1, column1) = newText;
  CheckTextSpill(row1, column1);
}

// Take the grouped area which includes given cell and enlarge it if necessary to make it rectangular ...

static void MakeLinkRectangle(Grid *grid, int row, int column)
{
  int i, j, left = column, top = row, right = column, bottom = row;
  for (;;)
  {
    bool enlargeL = false, enlargeT = false, enlargeR = false, enlargeB = false;
    if (left > 0)
    {
      for (i = top; i <= bottom; i++)
      {
        if (!((*grid)(i, left).m_flags & Cell_linkedLeft)) continue;
        enlargeL = true;
        break;
      }
    }
    if (top > 0)
    {
      for (i = left; i <= right; i++)
      {
        if (!((*grid)(top, i).m_flags & Cell_linkedAbove)) continue;
        enlargeT = true;
        break;
      }
    }
    if (right < grid->m_nColumns - 1)
    {
      for (i = top; i <= bottom; i++)
      {
        if (!((*grid)(i, right + 1).m_flags & Cell_linkedLeft)) continue;
        enlargeR = true;
        break;
      }
    }
    if (bottom < grid->m_nRows - 1)
    {
      for (i = left; i <= right; i++)
      {
        if (!((*grid)(bottom + 1, i).m_flags & Cell_linkedAbove)) continue;
        enlargeB = true;
        break;
      }
    }
    if (enlargeL) left--;
    if (enlargeT) top--;
    if (enlargeR) right++;
    if (enlargeB) bottom++;
    if (!enlargeL && !enlargeT && !enlargeR && !enlargeB) break;
  }
  Cell &cell = (*grid)(top, left);
  char flags = cell.m_flags & ~Cell_linkedAbove & ~Cell_linkedLeft;
  Colour backColour = cell.m_backColour, textColour = cell.m_textColour;
  short fontNo = cell.m_fontNo;
  for (i = top; i <= bottom; i++)
    for (j = left; j <= right; j++)
    {
      (*grid)(i, j).m_backColour = backColour;
      (*grid)(i, j).m_textColour = textColour;
      (*grid)(i, j).m_fontNo = fontNo;
      (*grid)(i, j).m_flags = flags | (i > top? Cell_linkedAbove: 0) | (j > left? Cell_linkedLeft: 0);
    }
}

struct CellConfigurePopup : PopupMenu
{
  int m_row, m_column;
  Bitmap m_backgroundBitmap, m_foregroundBitmap, m_menuCheck;

  CellConfigurePopup(Grid *parent, int row, int column) : 
  m_backgroundBitmap(15, 15), m_foregroundBitmap(15, 15), m_menuCheck(Bitmap::menuCheck),
    PopupMenu(parent), m_row(row), m_column(column)
  {
    Cell &cell = (*parent)(row, column);
    SetSize(23 + parent->m_nFonts);
    {
      DrawingSurface DS(m_backgroundBitmap);
      DS.Rectangle(Rect(0, 0, 14, 14), Pen(cell.m_textColour), Brush(cell.m_backColour));
    }
    SetMenuItem(0, "Background Colour...", true, &m_backgroundBitmap);
    {
      DrawingSurface DS(m_foregroundBitmap);
      DS.Rectangle(Rect(0, 0, 14, 14), Pen(cell.m_backColour), Brush(cell.m_textColour));
    }
    SetMenuItem(1, "Text Colour...", true, &m_foregroundBitmap);
    SetMenuItem(2, "Read Only", true, cell.m_flags & Cell_isReadOnly? &m_menuCheck: NULL);
    SetMenuItem(3, "Linked Left", column > 0, cell.m_flags & Cell_linkedLeft? &m_menuCheck: NULL);
    SetMenuItem(4, "Linked Above", row > 0, cell.m_flags & Cell_linkedAbove? &m_menuCheck: NULL);
    SetMenuItem(5);
    for (int i = 0; i < parent->m_nFonts; i++)
    {
      char buff[256];
      Font *font = NewFont(*parent->m_font[i]);
      font->Description(buff);
      SetMenuItem(6 + i, buff, true, i == cell.m_fontNo? &m_menuCheck: NULL, 18, font);
    }
    int n = 6 + parent->m_nFonts;
    SetMenuItem(n++, "Other font ...");
    SetMenuItem(n++);
    SetMenuItem(n++, "Left Align", true, (cell.m_flags & Cell_alignMask) == Cell_leftAlign? &m_menuCheck: NULL);
    SetMenuItem(n++, "Right Align", true, (cell.m_flags & Cell_alignMask) == Cell_rightAlign? &m_menuCheck: NULL);
    SetMenuItem(n++, "Centre Horizontally", true, (cell.m_flags & Cell_alignMask) == Cell_centreAlign? &m_menuCheck: NULL);
    SetMenuItem(n++);
    SetMenuItem(n++, "Top Align", true, (cell.m_flags & Cell_vertAlignMask) == Cell_topVAlign? &m_menuCheck: NULL);
    SetMenuItem(n++, "Bottom Align", true, (cell.m_flags & Cell_vertAlignMask) == Cell_bottomVAlign? &m_menuCheck: NULL);
    SetMenuItem(n++, "Centre Vertically", true, (cell.m_flags & Cell_vertAlignMask) == Cell_centreVAlign? &m_menuCheck: NULL);
    SetMenuItem(n++);
    SetMenuItem(n++, "Insert Row");
    SetMenuItem(n++, "Add Row");
    SetMenuItem(n++, "Delete Row");
    SetMenuItem(n++);
    SetMenuItem(n++, "Insert Column");
    SetMenuItem(n++, "Add Column");
    SetMenuItem(n++, "Delete Column");
    Configure();
  }

  void SelectionMade(int index, int keyMods)
  {
    Grid *grid = (Grid *)m_parent;
    Cell &cell = (*grid)(m_row, m_column);
    int row1, column1, row2, column2;
    grid->CellGroup(row1, column1, row2, column2, m_row, m_column);
    int i, j;
    if (index == 0) // Configure background colour
    {
      Colour backColour = cell.m_backColour;
      if (ConfigureColour(backColour, m_parent))
      {
        for (i = row1; i <= row2; i++)
          for (j = column1; j <= column2; j++)
            (*grid)(i, j).m_backColour = backColour;
      }
    }
    else if (index == 1) // Configure foreground colour
    {
      Colour textColour = cell.m_textColour;
      if (ConfigureColour(textColour, m_parent))
      {
        for (i = row1; i <= row2; i++)
          for (j = column1; j <= column2; j++)
            (*grid)(i, j).m_textColour = textColour;
      }
    }
    else if (index == 2) // read only
    {
      bool isReadOnly = !(cell.m_flags & Cell_isReadOnly);
      for (i = row1; i <= row2; i++)
        for (j = column1; j <= column2; j++)
          (*grid)(i, j).m_flags = (*grid)(i, j).m_flags & ~Cell_isReadOnly | (isReadOnly? Cell_isReadOnly: '\0');
    }
    else if (index == 3) // linked left
    {
      if (cell.m_flags & Cell_linkedLeft) // unlinking
      {
        for (i = m_row; i >= 0; i--)
        {
          (*grid)(i, m_column).m_flags &= ~Cell_linkedLeft;
          if (!((*grid)(i, m_column).m_flags & Cell_linkedAbove)) break;
        }
        for (i = m_row + 1; i < grid->m_nRows && (*grid)(i, m_column).m_flags & Cell_linkedAbove; i++)
          (*grid)(i, m_column).m_flags &= ~Cell_linkedLeft;
      }
      else // linking
      {
        cell.m_flags |= Cell_linkedLeft;
        MakeLinkRectangle(grid, m_row, m_column);
      }
    }
    else if (index == 4) // linked above
    {
      if (cell.m_flags & Cell_linkedAbove) // unlinking
      {
        for (i = m_column; i >= 0; i--)
        {
          (*grid)(m_row, i).m_flags &= ~Cell_linkedAbove;
          if (!((*grid)(m_row, i).m_flags & Cell_linkedLeft)) break;
        }
        for (i = m_column + 1; i < grid->m_nColumns && (*grid)(m_row, i).m_flags & Cell_linkedLeft; i++)
          (*grid)(m_row, i).m_flags &= ~Cell_linkedAbove;
      }
      else // linking
      {
        cell.m_flags |= Cell_linkedAbove;
        MakeLinkRectangle(grid, m_row, m_column);
      }
    }
    else if (index >= 6 && index < 6 + grid->m_nFonts) // font from exg list
    {
      short fontNo = index - 6;
      for (i = row1; i <= row2; i++)
        for (j = column1; j <= column2; j++)
          (*grid)(i, j).m_fontNo = fontNo;
    }
    else if (index == 6 + grid->m_nFonts) // font selection dialogue box
    {
      Font *newFont = new Font(*grid->m_font[cell.m_fontNo]);
      if (ConfigureFont(*newFont, cell.m_textColour, m_parent))
      {
        int fontNo;
        for (fontNo = 0;; fontNo++)
        {
          if (fontNo >= grid->m_nFonts)
          {
            grid->m_font = (Font **)ClientRealloc(grid->m_font, ++grid->m_nFonts * sizeof(Font *));
            grid->m_font[fontNo] = newFont;
            newFont = NULL;
            break;
          }
          if (*newFont == *grid->m_font[fontNo]) break;
        }
        for (i = row1; i <= row2; i++)
          for (j = column1; j <= column2; j++)
            (*grid)(i, j).m_fontNo = fontNo;
      }
      delete newFont;
    }
    else if (index >= 8 + grid->m_nFonts && index <= 10 + grid->m_nFonts) // left align
    {
      char align = (char)(index - 8 - grid->m_nFonts);
      for (i = row1; i <= row2; i++)
        for (j = column1; j <= column2; j++)
          (*grid)(i, j).m_flags = (*grid)(i, j).m_flags & ~Cell_alignMask | align;
    }
    else if (index >= 12 + grid->m_nFonts && index <= 14 + grid->m_nFonts) // left align
    {
      char vertAlign = (char)((index - 12 - grid->m_nFonts) << 2);
      for (i = row1; i <= row2; i++)
        for (j = column1; j <= column2; j++)
          (*grid)(i, j).m_flags = (*grid)(i, j).m_flags & ~Cell_vertAlignMask | vertAlign;
    }
    else if (index == 16 + grid->m_nFonts)
      grid->InsertRows(m_row);
    else if (index == 17 + grid->m_nFonts)
      grid->InsertRows(grid->m_nRows);
    else if (index == 18 + grid->m_nFonts)
      grid->DeleteRows(m_row);
    else if (index == 20 + grid->m_nFonts)
      grid->InsertColumns(m_column);
    else if (index == 21 + grid->m_nFonts)
      grid->InsertColumns(grid->m_nColumns);
    else if (index == 22 + grid->m_nFonts)
      grid->DeleteColumns(m_column);
    delete this;
    grid->UserFormatChange(row1, column1);
    grid->Draw();
  }

} *g_cellConfigurePopup = NULL;

void Grid::MouseRButtonPressed(int x, int y, int flags)
{
  int row, column;
  CellCoords(row, column, Point(x, y));
  if (column < 0 || column >= m_nColumns || row < 0 || row >= m_nRows) return;
  if (m_rightMouseFormatMenu)
  {
    g_cellConfigurePopup = new CellConfigurePopup(this, row, column);
    Point p = BoundingRect().TopLeft() - Canvas().TopLeft();
    g_cellConfigurePopup->Activate(p.x + x, p.y + y);
  }
  else CellRClicked(row, column);
}

void Grid::MouseRButtonDblClicked(int x, int y, int flags)
{
  if (m_rightMouseFormatMenu) return;
  int row, column;
  CellCoords(row, column, Point(x, y));
  if (column >= 0 && column < m_nColumns && row >= 0 && row < m_nRows)
    CellRDblClicked(row, column);
}
