#define WINDOBJ_MAIN

#include <WindObj/Private.h>

#include <limits.h>
#include <malloc.h>
#include <stdio.h>

typedef void * (__cdecl *MallocFunc)(size_t size);
typedef void * (__cdecl *ReallocFunc)(void *ptr, size_t size);
typedef void (__cdecl *FreeFunc)(void *ptr);

MallocFunc g_mallocFunc = malloc;
ReallocFunc g_reallocFunc = realloc;
FreeFunc g_freeFunc = free;

void * __cdecl operator new(size_t size)
{
  void *ptr = (*g_mallocFunc)(size);
  if (!ptr) throw "Out of memory";
  return ptr;
}

void * __cdecl ClientMalloc(size_t size)
{
  void *ptr1 = (*g_mallocFunc)(size);
  if (!ptr1) throw "Out of memory";
  return ptr1;
}

void * __cdecl ClientRealloc(void *ptr, size_t size)
{
  void *ptr1 = (*g_reallocFunc)(ptr, size);
  if (!ptr1) throw "Out of memory";
  return ptr1;
}

void __cdecl operator delete(void *ptr)
{
  (*g_freeFunc)(ptr);
}

void __cdecl ClientFree(void *ptr)
{
  (*g_freeFunc)(ptr);
}

WO_EXPORT void WindObjSetMemFuncs(MallocFunc clientMalloc, 
                                              ReallocFunc clientRealloc, FreeFunc clientFree)
{
  g_mallocFunc = clientMalloc;
  g_reallocFunc = clientRealloc;
  g_freeFunc = clientFree;
}

void RestoreMemFuncs()
{
  g_mallocFunc = malloc;
  g_reallocFunc = realloc;
  g_freeFunc = free;
}

char *WindObjError(DWORD errCode)
{
  if (!errCode) errCode = GetLastError();
  static char s_emsg[256];
  memset(s_emsg, 0, 256);
  FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, errCode,
    /*MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_UK)*/ 0, s_emsg, 256, NULL);
  return s_emsg;
}

BOOL SetWindowText1(HWND hWnd, LPCTSTR text, int ntext)
{
  char *buff = (char *)_alloca(ntext + 1);
  strncpy(buff, text, ntext);
  buff[ntext] = '\0';
  return SetWindowText(hWnd, (LPCTSTR)buff);
}

UINT GetDlgItemText1(HWND hDlg, int id, LPTSTR str, int maxCount)
{
  char *buff = (char *)_alloca(maxCount + 1);
  UINT r = GetDlgItemText(hDlg, id, buff, maxCount + 1);
  memset(str, 0, maxCount);
  memcpy(str, buff, r);
  return r;
}

BOOL SetDlgItemText1(HWND hDlg, int id, LPCTSTR str, int maxCount)
{
  char *buff = (char *)_alloca(maxCount + 1);
  buff[maxCount] = '\0';
  strncpy(buff, str, maxCount);
  return SetDlgItemText(hDlg, id, buff);
}

HBRUSH CloneBrush(HBRUSH hBrush)
{
  HBRUSH hBrC = NULL;
  LOGBRUSH lb;
  if (!GetObject(hBrush, sizeof(LOGBRUSH), &lb) ||
      !(hBrC = CreateBrushIndirect(&lb))) throw WindObjError();
  return hBrC;
}

HFONT CloneFont(HFONT hFont)
{
  if (!hFont) return NULL;
  LOGFONT lf;
  if (!GetObject(hFont, sizeof(LOGFONT), &lf))
  {
    DWORD err = GetLastError();
    throw err? "CloneFont: GetObject non-zero error": "CloneFont: GetObject zero error";
  }
  HFONT hFontC = CreateFontIndirect(&lf);
  if (!hFontC) throw "CloneFont: CreateFontIndirect failed";
  return hFontC;
}

HPEN ClonePen(HPEN hPen)
{
  HPEN hPenC = NULL;
  int n = GetObject(hPen, 0, NULL);
  if (!n) throw WindObjError();
  char *buff = new char[n];
  GetObject(hPen, n, buff);
  if (n == sizeof(LOGPEN)) hPenC = CreatePenIndirect((const LOGPEN *)buff);
  else
  {
    EXTLOGPEN *elp = (EXTLOGPEN *)buff;
    LOGBRUSH lb = {elp->elpBrushStyle, elp->elpColor, elp->elpHatch};
    hPenC = ExtCreatePen(elp->elpPenStyle, elp->elpWidth, &lb, elp->elpNumEntries, 
      elp->elpNumEntries? elp->elpStyleEntry: NULL);
  }
  delete [] buff;
  if (!hPenC) throw WindObjError();
  return hPenC;
}

HICON CloneIcon(HICON hIcon)
{
  ICONINFO ii;
  if (!GetIconInfo(hIcon, &ii)) throw WindObjError();
  HICON hCloneIcon = CreateIconIndirect(&ii);
  DeleteObject(ii.hbmMask);
  DeleteObject(ii.hbmColor);
  if (!hCloneIcon) throw WindObjError();
  return hCloneIcon;
}

// Reads a packed DIB from a file, & puts it into a memory block which can be used in the clipboard ...

HGLOBAL ReadPackedDIB(WindObjStream &ifs)
{
  HGLOBAL h = NULL;
  int n = 0;
  try
  {
    BITMAPFILEHEADER bfh;
    ifs.Read(&bfh, sizeof(BITMAPFILEHEADER));
    if (bfh.bfSize == 0) return NULL;
    if (bfh.bfType != 0x4D42 || !(h = (BITMAPINFOHEADER *)GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE,
      n = bfh.bfSize - sizeof(BITMAPFILEHEADER)))) throw "Error reading bitmap file";
    ifs.Read(GlobalLock(h), n);
    GlobalUnlock(h);
    return h;
  }
  catch (char *)
  {
    if (n) GlobalUnlock(h);
    if (h) GlobalFree(h);
    throw;
  }
}

void WritePackedDIB(WindObjStream &ofs, HGLOBAL hDib)
{
  BITMAPFILEHEADER bfh;
  memset(&bfh, 0, sizeof(BITMAPFILEHEADER));
  if (!hDib) // Write a zeroed header if the bitmap is NULL
  {
    ofs.Write(&bfh, sizeof(BITMAPFILEHEADER));
    return;
  }
  BITMAPINFOHEADER *bmih = (BITMAPINFOHEADER *)GlobalLock(hDib);
  int bpp = bmih->biPlanes * bmih->biBitCount;
  DWORD biClrUsed = bmih->biClrUsed == 0 && bpp < 24? 1 << bpp: bmih->biClrUsed;
  if (bmih->biCompression == BI_BITFIELDS && biClrUsed == 0) biClrUsed = 3;
  LONG ht;
  if ((ht = bmih->biHeight) < 0) ht = -ht;
  DWORD biSizeImage = bmih->biSizeImage == 0? (((bmih->biWidth  * bpp + 31) >> 3) & ~3) * ht: bmih->biSizeImage;
  bfh.bfType = 0x4D42;
  bfh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + biClrUsed * sizeof(RGBQUAD);
  bfh.bfSize = bfh.bfOffBits + biSizeImage;
  try
  {
    ofs.Write(&bfh, sizeof(BITMAPFILEHEADER));
    ofs.Write(bmih, bfh.bfSize - sizeof(BITMAPFILEHEADER));
    GlobalUnlock(hDib);
  }
  catch (char *) {GlobalUnlock(hDib); throw;}
}

void WriteBitmap(WindObjStream &ofs, const Bitmap *bm)
{
  HGLOBAL hDib;

  if (!bm) hDib = NULL;
  else hDib = PackedDIBFromBitmap(bm->m_bitmapData->hBitmap, bm->m_bitmapData->hPalette);
  try
  {
    WritePackedDIB(ofs, hDib);
    if (hDib) GlobalFree(hDib);
  }
  catch (char *)
  {
    if (hDib) GlobalFree(hDib);
    throw;
  }
}

// converts a DIB into a BITMAP and a HPALETTE

HBITMAP BitmapFromPackedDIB(HPALETTE *hPal, HGLOBAL hDib)
{
  DWORD biClrUsed;
  BITMAPINFOHEADER *bmih, *bmih2;
  HBITMAP hBm;
  HPALETTE hPal2 = NULL;
  HDC hDCmem;

  if (hPal) *hPal = NULL;
  if (!hDib || !(bmih = (BITMAPINFOHEADER *)GlobalLock(hDib))) return NULL;
  if (bmih->biSize < sizeof(BITMAPINFOHEADER) - sizeof(DWORD)) biClrUsed = 0;
  else biClrUsed = bmih->biClrUsed;
  if (biClrUsed == 0 && bmih->biBitCount < 24) biClrUsed = 1 << bmih->biBitCount;
  if (biClrUsed > 0)
  {
    if (biClrUsed <= SHRT_MAX)
    {
      LOGPALETTE *pal = (LOGPALETTE *)ClientMalloc(sizeof(LOGPALETTE) +
        (biClrUsed - 1) * sizeof(PALETTEENTRY));
      if (!pal) {GlobalUnlock(hDib); throw "Out of memory";}
      pal->palVersion = 0x300;
      pal->palNumEntries = (WORD)biClrUsed;
      RGBQUAD *pRGB = (RGBQUAD *)((char *)bmih + bmih->biSize);
      for (DWORD i = 0; i < biClrUsed; i++)
      {
        pal->palPalEntry[i].peRed = pRGB[i].rgbRed;
        pal->palPalEntry[i].peGreen = pRGB[i].rgbGreen;
        pal->palPalEntry[i].peBlue = pRGB[i].rgbBlue;
        pal->palPalEntry[i].peFlags = 0;
      }
      hPal2 = CreatePalette(pal);
      ClientFree(pal);
      if (!hPal2) {GlobalUnlock(hDib); throw "Failed to create palette";}
    }
    else
    {
      hPal2 = (HPALETTE)GetStockObject(DEFAULT_PALETTE);
    }
    if (!(bmih2 = (BITMAPINFOHEADER *)ClientMalloc(sizeof(BITMAPINFOHEADER) + biClrUsed * sizeof(short))))
    {
      GlobalUnlock(hDib);
      DeleteObject(hPal2);
      throw "Out of memory";
    }
    *bmih2 = *bmih;
    bmih2->biSize = sizeof(BITMAPINFOHEADER);
    for (DWORD i = 0; i < biClrUsed; i++) ((short *)(bmih2 + 1))[i] = (short)i;
  }
  else
  {
    bmih2 = bmih;
    hPal2 = (HPALETTE)GetStockObject(DEFAULT_PALETTE);
  } 
  char *p = (char *)bmih + bmih->biSize + biClrUsed * sizeof(RGBQUAD);
  bmih->biBitCount = 16;
  DWORD d = !(hDCmem = CreateCompatibleDC(NULL)) ||
    !SelectPalette(hDCmem, hPal2, FALSE) ||
    RealizePalette(hDCmem) == GDI_ERROR ||
    !(hBm = CreateDIBitmap(hDCmem, bmih, CBM_INIT, p, (BITMAPINFO *)bmih2, DIB_PAL_COLORS));
  if (d) d = GetLastError();
  DeleteDC(hDCmem);
  GlobalUnlock(hDib);
  if (bmih2 != bmih) ClientFree(bmih2);
  if (d)
  {
    DeleteObject(hPal2);
    throw WindObjError(d);
  }
  if (hPal) *hPal = hPal2;
  return hBm;
} 

HBITMAP NewBitmapFromPackedDIB(HPALETTE *hPal, HGLOBAL hDib)
{
  DWORD biClrUsed;
  BITMAPINFOHEADER *bmih;

  if (!hDib || !(bmih = (BITMAPINFOHEADER *)GlobalLock(hDib))) return NULL;
  if (bmih->biSize < sizeof(BITMAPINFOHEADER) - sizeof(DWORD)) biClrUsed = 0;
  else biClrUsed = bmih->biClrUsed;
  *hPal = (HPALETTE)GetStockObject(DEFAULT_PALETTE);
/*  if (biClrUsed == 0 && bmih->biBitCount < 24) biClrUsed = 1 << bmih->biBitCount;
  if (hPal)
  {
    if (biClrUsed > 0)
    {
      LOGPALETTE *pal = (LOGPALETTE *)ClientMalloc(sizeof(LOGPALETTE) +
        (biClrUsed - 1) * sizeof(PALETTEENTRY));
      if (!pal) {GlobalUnlock(hDib); throw "Out of memory";}
      pal->palVersion = 0x300;
      pal->palNumEntries = (WORD)biClrUsed;
      RGBQUAD *pRGB = (RGBQUAD *)((char *)bmih + bmih->biSize);
      for (DWORD i = 0; i < biClrUsed; i++)
      {
        pal->palPalEntry[i].peRed = pRGB[i].rgbRed;
        pal->palPalEntry[i].peGreen = pRGB[i].rgbGreen;
        pal->palPalEntry[i].peBlue = pRGB[i].rgbBlue;
        pal->palPalEntry[i].peFlags = 0;
      }
      *hPal = CreatePalette(pal);
      ClientFree(pal);
    }
    else *hPal = NULL;
  } */
// bmih->biBitCount, (char *)bmih + bmih->biSize + biClrUsed * sizeof(RGBQUAD)
  HDC hDCscreen = CreateCompatibleDC(NULL);
//  SelectObject(hDCscreen, )
  bmih->biBitCount = 16;
  int bytesPerLine = (bmih->biWidth * bmih->biBitCount + 15) / 16 * 2;
  char *temp = (char *)ClientMalloc(bmih->biHeight * bytesPerLine);
  memset(temp, 0, bmih->biHeight * bytesPerLine);
  BITMAP BM = {0, bmih->biWidth, bmih->biHeight, bytesPerLine, 1, bmih->biBitCount, temp};
  HBITMAP hBM = CreateBitmapIndirect(&BM);
  ClientFree(temp);
  GlobalUnlock(hDib);
  return hBM;
} 

HGLOBAL PackedDIBFromBitmap(HBITMAP hBm, HPALETTE hPal)
{
  if (!hBm) return NULL;
  HDC hDC = CreateCompatibleDC(NULL);
  if (hPal)
  {
    SelectPalette(hDC, hPal, FALSE);
    RealizePalette(hDC);
  }
  BITMAPINFOHEADER bmih;
  memset(&bmih, 0, sizeof(BITMAPINFOHEADER));
  bmih.biSize = sizeof(BITMAPINFOHEADER);
  GetDIBits(hDC, hBm, 0, 0, NULL, (BITMAPINFO *)&bmih, DIB_RGB_COLORS); 
  int bpp = bmih.biPlanes * bmih.biBitCount;
  DWORD biClrUsed = bmih.biClrUsed == 0 && bpp < 24? 1 << bpp: bmih.biClrUsed;
  LONG ht;
  if ((ht = bmih.biHeight) < 0) ht = -ht;
  DWORD biSizeImage = bmih.biSizeImage? bmih.biSizeImage: (((bmih.biWidth * bpp + 31) >> 3) & ~3) * ht;
  if (bmih.biCompression == BI_BITFIELDS && biClrUsed == 0) biClrUsed = 3;
  HGLOBAL hDib = GlobalAlloc(GMEM_MOVEABLE, sizeof(BITMAPINFOHEADER) + biClrUsed * sizeof(RGBQUAD) + biSizeImage);
  if (!hDib) return NULL;
  BITMAPINFOHEADER *bmih2 = (BITMAPINFOHEADER *)GlobalLock(hDib);
  *bmih2 = bmih;
  DWORD *data = (DWORD *)((RGBQUAD *)(bmih2 + 1) + biClrUsed);
  BITMAPINFO *bmi = (BITMAPINFO *)bmih2;
  int i = GetDIBits(hDC, hBm, 0, ht, data, bmi, DIB_RGB_COLORS);
  DeleteDC(hDC);
  if (i != ht)
  {
    GlobalFree(hDib);
    return NULL;
  }
  GlobalUnlock(hDib);
  return hDib;
}

HBITMAP LoadBitmapFile(HPALETTE *hPal, const char *fileName) 
{
  WindObjFileStream WOFS(fileName);
  HGLOBAL hDib = NULL;
  HBITMAP hBm = NULL;
  *hPal = NULL;
  try {hBm = BitmapFromPackedDIB(hPal, hDib = ReadPackedDIB(WOFS));}
  catch (...) {GlobalFree(hDib); throw;}
  GlobalFree(hDib);
  return hBm;
} 

/* HBITMAP CloneBitmap(HBITMAP hBm)
{
   HDC hbmdc = CreateCompatibleDC(NULL), hclonedc = CreateCompatibleDC(NULL);
   BITMAP bm;
   HDC hDC = CreateCompatibleDC(NULL);
   GetObject(hBm, sizeof(BITMAP), &bm);
   HBITMAP hCloneBm = CreateCompatibleBitmap(hDC, bm.bmWidth, bm.bmHeight);
   SelectObject(hclonedc, hCloneBm);
   SelectObject(hbmdc, hBm);
   BitBlt(hclonedc, 0, 0, bm.bmWidth, bm.bmHeight, hbmdc, 0, 0, SRCCOPY);
   DeleteDC(hDC);
   DeleteDC(hbmdc);
   DeleteDC(hclonedc);
   return hCloneBm;
} */

HBITMAP CloneBitmap(HBITMAP hBm)
{
  BITMAP bm;
  long n;
  HBITMAP hCloneBm;

  if (!hBm) return NULL;
  GetObject(hBm, sizeof(BITMAP), &bm);
  bm.bmBits = new char[n = bm.bmWidthBytes * bm.bmHeight * bm.bmPlanes];
  if (!GetBitmapBits(hBm, n, bm.bmBits) ||
      !(hCloneBm = CreateBitmapIndirect(&bm)))
      throw WindObjError();
  delete bm.bmBits;
  return hCloneBm;
}   

HPALETTE ClonePalette(HPALETTE hPal)
{
  if (!hPal) return NULL;
  UINT n = GetPaletteEntries(hPal, 0, 0, NULL);
  if (n == 0) throw WindObjError();
  LOGPALETTE *pal = (LOGPALETTE *)ClientMalloc(sizeof(LOGPALETTE) + 
      (n - 1) * sizeof(PALETTEENTRY));
  if (!pal) throw "Out of memory";
  pal->palVersion = 0x300;             
  pal->palNumEntries = n;
  HPALETTE hPal2;
  if (!GetPaletteEntries(hPal, 0, n, pal->palPalEntry) ||
      !(hPal2 = CreatePalette(pal))) 
    {delete pal; throw WindObjError();}
  delete pal;
  return hPal2;
}

static void memmovebits(char *dest, char *src, int bitOffs, int nBits)
{
  int nbytes = (bitOffs + nBits + 7) / 8, n;
  memmove(dest, src, nbytes);
  if (!(n = bitOffs)) return;
  char c, c1 = 0;
  for (int i = nbytes - 1; i >= 0; i--, c1 = c)
  {
    c = ((unsigned char)dest[i]) >> 8 - n;
    dest[i] = dest[i] << n | c1;
  }
}

HBITMAP SubBitmap(HBITMAP hBm, int x, int y, int w, int h)
{
   BITMAP bm;
   long n;
   char *src, *dest;

   if (!hBm) return NULL;
   if (!GetObject(hBm, sizeof(BITMAP), &bm)) throw WindObjError();
   if (x >= bm.bmHeight || y >= bm.bmWidth) return NULL;
   if (x + w > bm.bmWidth) w = bm.bmWidth - x;
   if (y + h > bm.bmHeight) h = bm.bmHeight - y;
   if (!(bm.bmBits = new char[n = bm.bmWidthBytes * bm.bmHeight * bm.bmPlanes])) 
     throw "Out of memory";
   if (!GetBitmapBits(hBm, n, bm.bmBits)) {delete bm.bmBits; throw WindObjError();}
   int i, j, wb = (w * bm.bmBitsPixel + 15) / 16 * 2;
   dest = (char *)bm.bmBits;
   for (i = 0; i < bm.bmPlanes; i++)
   {
     src = (char *)bm.bmBits + bm.bmWidthBytes * (i * bm.bmHeight + y); // start of scan line
     int offs = x * bm.bmBitsPixel;
     src += offs / 8;
     offs %= 8;
     for (j = 0; j < h; j++, dest += wb, src += bm.bmWidthBytes)
       memmovebits(dest, src, offs, w * bm.bmBitsPixel);
   }
   bm.bmWidth = w;
   bm.bmHeight = h;
   bm.bmWidthBytes = wb;
   HBITMAP hCloneBm;
   if (!(hCloneBm = CreateBitmapIndirect(&bm))) {delete bm.bmBits; throw WindObjError();}
   delete bm.bmBits;
   return hCloneBm;
}   

int BitmapComp(HBITMAP hBm1, HBITMAP hBm2)
{
   if (hBm1 == hBm2) return 0;
   if (!hBm1 && hBm2) return -1;
   if (hBm1 && !hBm2) return 1;

   BITMAP bm1, bm2;
   GetObject(hBm1, sizeof(BITMAP), &bm1);
   GetObject(hBm2, sizeof(BITMAP), &bm2);
   long n;
   if (n = memcmp(&bm1, &bm2, sizeof(BITMAP))) return n;
   if (!(bm1.bmBits = ClientMalloc(n = bm1.bmWidthBytes * bm1.bmHeight * bm1.bmPlanes))) 
     return -1;
   if (!(bm2.bmBits = ClientMalloc(n))) {ClientFree(bm1.bmBits); return 1;}
   GetBitmapBits(hBm1, n, bm1.bmBits);
   GetBitmapBits(hBm2, n, bm2.bmBits);
   n = memcmp(bm1.bmBits, bm2.bmBits, n);
   ClientFree(bm1.bmBits);
   ClientFree(bm2.bmBits);
   return n;
}

int PaletteComp(HPALETTE hPal1, HPALETTE hPal2)
{
  if (hPal1 == hPal2) return 0;
  if (!hPal1 && hPal2) return -1;
  if (hPal1 && !hPal2) return 1;

  WORD n1, n2;
  GetObject(hPal1, sizeof(WORD), &n1);
  GetObject(hPal2, sizeof(WORD), &n2);
  if (n1 < n2) return -1;
  if (n1 > n2) return 1;

  LOGPALETTE *pal1 = (LOGPALETTE *)ClientMalloc(sizeof(LOGPALETTE) + 
      (n1 - 1) * sizeof(PALETTEENTRY));
  if (!pal1) return -1;
  LOGPALETTE *pal2 = (LOGPALETTE *)ClientMalloc(sizeof(LOGPALETTE) + 
      (n1 - 1) * sizeof(PALETTEENTRY));
  if (!pal2) {ClientFree(pal1); return 1;}
  GetPaletteEntries(hPal1, 0, n1, pal1->palPalEntry);
  GetPaletteEntries(hPal2, 0, n1, pal2->palPalEntry);
  n1 = memcmp(pal1->palPalEntry, pal2->palPalEntry, n1 * sizeof(PALETTEENTRY));
  ClientFree(pal1);
  ClientFree(pal2);
  return n1;
}

void DrawBitmap(HDC hDC, HBITMAP hBitmap, int x, int y)
{
  BITMAP bm;
  POINT sz, org;

  HDC hDCmem = CreateCompatibleDC(hDC);
  SelectObject(hDCmem, hBitmap);
  SetMapMode(hDCmem, GetMapMode(hDC));
  GetObject(hBitmap, sizeof(BITMAP), (void *)&bm);
  sz.x = bm.bmWidth;
  sz.y = bm.bmHeight;
  DPtoLP(hDC, &sz, 1);
  org.x = 0;
  org.y = 0;
  DPtoLP(hDCmem, &org, 1);
  BitBlt(hDC, x, y, sz.x, sz.y, hDCmem, org.x, org.y, SRCCOPY);
  DeleteDC(hDCmem);
}

/*
void DrawBitmapLite(HDC hDC, HBITMAP hBitmap, int x, int y, COLORREF fc, COLORREF bc)
{
  BITMAP bm;
  POINT sz;
  HGDIOBJ hobj;
  COLORREF fco, bco;

  fco = SetTextColor(hDC, fc);
  bco = SetBkColor(hDC, bc);
  hobj = SelectObject(hDC, CreatePatternBrush(hBitmap));
  GetObject(hBitmap, sizeof(BITMAP), (void *)&bm);
  sz.x = bm.bmWidth;
  sz.y = bm.bmHeight;
  DPtoLP(hDC, &sz, 1);
  PatBlt(hDC, x, y, sz.x, sz.y, PATCOPY);
  SetTextColor(hDC, fco);
  SetBkColor(hDC, bco);
  DeleteObject(SelectObject(hDC, hobj));
}
*/

COLORREF BlendColour(COLORREF clr1, COLORREF clr2)
{
  return (clr1 & 0xFEFEFE) + (clr2 & 0xFEFEFE) >> 1 | clr1 & clr2 & 0x10101;
}

BOOL GetWindowRect1(HWND hWnd, RECT *rect)
{
  if (!GetWindowRect(hWnd, rect)) return FALSE;
  if (GetWindowLong(hWnd, GWL_STYLE) & WS_CHILD)
    MapWindowPoints(NULL, GetParent(hWnd), (LPPOINT)rect, 2);
  return TRUE;
}

SIZE TextSize(HFONT hFont, char *str)
{
  SIZE siz;
  HDC hDC = CreateIC("DISPLAY", NULL, NULL, NULL);
  if (hFont) SelectObject(hDC, hFont);
  GetTextExtentPoint32(hDC, str, strlen(str), &siz);
  DeleteDC(hDC);
  return siz;
}

SIZE TextSize(HFONT hFont, wchar_t *wstr)
{
  SIZE siz;
  HDC hDC = CreateIC("DISPLAY", NULL, NULL, NULL);
  if (hFont) SelectObject(hDC, hFont);
  GetTextExtentPoint32W(hDC, wstr, wstr? wcslen(wstr): 0, &siz);
  DeleteDC(hDC);
  return siz;
}

int TextWidth(HFONT hFont, const char *str)
{
  return TextWidth(hFont, str, strlen(str));
}

int TextWidth(HFONT hFont, const wchar_t *wstr)
{
  return TextWidth(hFont, wstr, wcslen(wstr));
}

int TextWidth(HFONT hFont, const char *str, int nstr)
{
  if (!str) return 0;
  SIZE siz;
  HDC hDC = CreateIC("DISPLAY", NULL, NULL, NULL);
  SelectObject(hDC, hFont);
  GetTextExtentPoint32(hDC, str, nstr, &siz);
  DeleteDC(hDC);
  return siz.cx;
}

int TextWidth(HFONT hFont, const wchar_t *wstr, int nstr)
{
  if (!wstr) return 0;
  SIZE siz;
  HDC hDC = CreateIC("DISPLAY", NULL, NULL, NULL);
  SelectObject(hDC, hFont);
  GetTextExtentPoint32W(hDC, wstr, nstr, &siz);
  DeleteDC(hDC);
  return siz.cx;
}

int IntTextWidth(HFONT hFont, int i)
{
  char buff[20];
  sprintf(buff, "%d", i);
  return TextWidth(hFont, buff);
}

int TextWidth1(HDC hDC, const char *str)
{
  SIZE siz;
  GetTextExtentPoint32(hDC, str, strlen(str), &siz);
  return siz.cx;
}

int TextHeight1(HDC hDC, const char *str)
{
  SIZE siz;
  GetTextExtentPoint32(hDC, str, strlen(str), &siz);
  return siz.cy;
}

int TextHeight(HFONT hFont)
{
  SIZE siz;
  HDC hDC = CreateIC("DISPLAY", NULL, NULL, NULL);
  SelectObject(hDC, hFont);
  GetTextExtentPoint32(hDC, "M", 1, &siz);
  DeleteDC(hDC);
  return siz.cy;
}

int WindowX(HWND hWnd)
{
  RECT rect;

  if (!GetWindowRect1(hWnd, &rect)) return 0;
  return rect.left;
}

void SetWindowX(HWND hWnd, int X)
{
  RECT rect;

  if (!GetWindowRect1(hWnd, &rect)) return;
  MoveWindow(hWnd, X, rect.top, rect.right - rect.left, rect.bottom - rect.top, TRUE);
}

int WindowY(HWND hWnd)
{
  RECT rect;

  if (!GetWindowRect1(hWnd, &rect)) return 0;
  return rect.top;
}

void SetWindowY(HWND hWnd, int Y)
{
  RECT rect;

  if (!GetWindowRect1(hWnd, &rect)) return;
  MoveWindow(hWnd, rect.left, Y, rect.right - rect.left, rect.bottom - rect.top, TRUE);
}

void SetWindowPos(HWND hWnd, int X, int Y)
{
  RECT rect;

  if (!GetWindowRect1(hWnd, &rect)) return;
  MoveWindow(hWnd, X, Y, rect.right - rect.left, rect.bottom - rect.top, TRUE);
}

int WindowWidth(HWND hWnd)
{
  RECT rect;

  if (!GetWindowRect(hWnd, &rect)) return 0;
  return rect.right - rect.left;
}

int WindowClientWidth(HWND hWnd)
{
  RECT rect;

  if (!GetClientRect(hWnd, &rect)) return 0;
  return rect.right;
}

void SetWindowWidth(HWND hWnd, int width)
{
  RECT rect;

  if (!GetWindowRect1(hWnd, &rect)) return;
  MoveWindow(hWnd, rect.left, rect.top, width, rect.bottom - rect.top, TRUE);
}

void SetWindowClientWidth(HWND hWnd, int width)
{
  RECT rectc, rectw;

  GetClientRect(hWnd, &rectc);
  GetWindowRect(hWnd, &rectw);
  SetWindowWidth(hWnd, width + rectw.right - rectw.left - rectc.right);
}

int WindowHeight(HWND hWnd)
{
  RECT rect;

  if (!GetWindowRect(hWnd, &rect)) return 0;
  return rect.bottom - rect.top;
}

int WindowClientHeight(HWND hWnd)
{
  RECT rect;

  if (!GetClientRect(hWnd, &rect)) return 0;
  return rect.bottom;
}

void SetWindowHeight(HWND hWnd, int height)
{
  RECT rect;

  if (!GetWindowRect1(hWnd, &rect)) return;
  MoveWindow(hWnd, rect.left, rect.top, rect.right - rect.left, height, TRUE);
}

void SetWindowClientHeight(HWND hWnd, int height)
{
  RECT rectc, rectw;

  GetClientRect(hWnd, &rectc);
  GetWindowRect(hWnd, &rectw);
  SetWindowHeight(hWnd, height + rectw.bottom - rectw.top - rectc.bottom);
}

void SetWindowSize(HWND hWnd, int width, int height)
{
  RECT rect;

  if (!GetWindowRect1(hWnd, &rect)) return;
  MoveWindow(hWnd, rect.left, rect.top, width, height, TRUE);
}

static struct StdKey
{
  char tok[8];
  WORD vk;

} s_stdKey[] = 
{
  {"back",    VK_BACK},
  {"backspa", VK_BACK},
  {"clear",   VK_CLEAR},
  {"del",     VK_DELETE},
  {"delete",  VK_DELETE},
  {"down",    VK_DOWN},
  {"end",     VK_END},
  {"enter",   VK_RETURN},
  {"esc",     VK_ESCAPE},
  {"escape",  VK_ESCAPE},
  {"f1",      VK_F1},
  {"f2",      VK_F2},
  {"f3",      VK_F3},
  {"f4",      VK_F4},
  {"f5",      VK_F5},
  {"f6",      VK_F6},
  {"f7",      VK_F7},
  {"f8",      VK_F8},
  {"f9",      VK_F9},
  {"f10",     VK_F10},
  {"f11",     VK_F11},
  {"f12",     VK_F12},
  {"f13",     VK_F13},
  {"f14",     VK_F14},
  {"f15",     VK_F15},
  {"f16",     VK_F16},
  {"f17",     VK_F17},
  {"f18",     VK_F18},
  {"f19",     VK_F19},
  {"f20",     VK_F20},
  {"f21",     VK_F21},
  {"f22",     VK_F22},
  {"f23",     VK_F23},
  {"f24",     VK_F24},
  {"home",    VK_HOME},
  {"ins",     VK_INSERT},
  {"insert",  VK_INSERT},
  {"left",    VK_LEFT},
  {"menu",    VK_MENU},
  {"num*",    VK_MULTIPLY},
  {"num+",    VK_ADD},
  {"num,",    VK_SEPARATOR},
  {"num-",    VK_SUBTRACT},
  {"num.",    VK_DECIMAL},
  {"num/",    VK_DIVIDE},
  {"num0",    VK_NUMPAD0},
  {"num1",    VK_NUMPAD1},
  {"num2",    VK_NUMPAD2},
  {"num3",    VK_NUMPAD3},
  {"num4",    VK_NUMPAD4},
  {"num5",    VK_NUMPAD5},
  {"num6",    VK_NUMPAD6},
  {"num7",    VK_NUMPAD7},
  {"num8",    VK_NUMPAD8},
  {"num9",    VK_NUMPAD9},
  {"numpad*", VK_MULTIPLY},
  {"numpad+", VK_ADD},
  {"numpad,", VK_SEPARATOR},
  {"numpad-", VK_SUBTRACT},
  {"numpad.", VK_DECIMAL},
  {"numpad/", VK_DIVIDE},
  {"numpad0", VK_NUMPAD0},
  {"numpad1", VK_NUMPAD1},
  {"numpad2", VK_NUMPAD2},
  {"numpad3", VK_NUMPAD3},
  {"numpad4", VK_NUMPAD4},
  {"numpad5", VK_NUMPAD5},
  {"numpad6", VK_NUMPAD6},
  {"numpad7", VK_NUMPAD7},
  {"numpad8", VK_NUMPAD8},
  {"numpad9", VK_NUMPAD9},
  {"pagedow", VK_NEXT},
  {"pageup",  VK_PRIOR},
  {"pause",   VK_PRINT},
  {"pgdn",    VK_NEXT},
  {"pgdown",  VK_NEXT},
  {"pgup",    VK_PRIOR},
  {"print",   VK_PRINT},
  {"return",  VK_RETURN},
  {"right",   VK_RIGHT},
  {"select",  VK_SELECT},
  {"space",   VK_SPACE},
  {"spaceba", VK_SPACE},
  {"tab",     VK_TAB},
  {"up",      VK_UP}
};

#define c_nStdKeys (sizeof(s_stdKey) / sizeof(StdKey))

inline int _isspace(int c) {return c >= '\x09' && c <= '\x0D' || c == ' ';}

/* This function takes strings of the form "Ctrl+Shift+F4" and fills in an ACCEL structure
   appropriately. The cmd member is set to one if the string was understood & zero 
   if not ... */

ACCEL ParseAccelString(char *str)
{
  ACCEL a = {FVIRTKEY, 0, 0};
  char tok[8];
  int i;
  char *ptr = str;

  for (;;)
  {
    while (_isspace(*ptr)) ptr++;
    i = 0;
    memset(tok, 0, 8);
    while (isalnum((unsigned char)*ptr))
    {
PAS0:
      if (i < 7) tok[i++] = tolower(*ptr);
      ptr++;
      if (!strcmp(tok, "pg") || !strcmp(tok, "page") || !strcmp(tok, "num"))
        while (_isspace(*ptr)) ptr++;
      if (strchr("*+,-./", *ptr) && (!strcmp(tok, "num") || !strcmp(tok, "numpad"))) goto PAS0;
    }
    if (!strcmp(tok, "control") || !strcmp(tok, "ctrl") || !strcmp(tok, "ctl"))
      a.fVirt |= FCONTROL;
    else if (!strcmp(tok, "shift")) a.fVirt |= FSHIFT;
    else if (!strcmp(tok, "alt")) a.fVirt |= FALT;
    else
    {
      if (i == 1) // single alphanumeric character
      {
        a.key = toupper(tok[0]);
        break;
      }
      if (i == 0)
      {
// Non-alphanumeric key is marked as being ASCII, not virtual -- to cope with different international kb layouts
        a.key = *ptr;
        a.fVirt ^= FVIRTKEY;
        a.cmd = !!*ptr;
        return a;
      }
      for (i = 0; i < c_nStdKeys; i++)
        if (!strcmp(tok, s_stdKey[i].tok))
        {
          a.key = s_stdKey[i].vk;
          break;
        }
      return a;
    }
    while (_isspace(*ptr)) ptr++;
    if (*ptr == '+') ptr++;
  }
  a.cmd = 1;
  return a;
}

int FindInPath(char *buff, int nPath, const char *name)
{
  char *ptr;
  return SearchPath(NULL, name, NULL, nPath, buff, &ptr);
}

void OutputDebug(const char *fmt, ...)
{
	enum { BuffLen = 1024 };

  char buff[BuffLen];
  va_list ap;
  va_start(ap, fmt);
  _vsnprintf(buff, BuffLen, fmt, ap);
	buff[BuffLen-1] = '\0';
  va_end(ap);
  OutputDebugString(buff);
}

struct WOCritSec
{
  RTL_CRITICAL_SECTION m_CS;

  WOCritSec() {InitializeCriticalSection(&m_CS);}
  ~WOCritSec() {DeleteCriticalSection(&m_CS);}

} g_timerCS;

struct WOCritSecLock
{
  WOCritSec *m_WOCS;

  WOCritSecLock(WOCritSec *WOCS) {EnterCriticalSection(&(m_WOCS = WOCS)->m_CS);}
  ~WOCritSecLock() {LeaveCriticalSection(&m_WOCS->m_CS);}
};

struct ActiveTimer
{
  Timer *timer;
  unsigned int idEvent;

} *g_activeTimer = NULL;

int g_nActiveTimers = 0;

bool ActiveTimerIdx(int &idx, unsigned int idEvent)
{
  int cmp, rangeMin = 0, rangeMid, rangeMax;
  if (g_nActiveTimers < 1)
  {
    idx = 0;
    return false;
  }
  if ((cmp = (int)idEvent - (int)g_activeTimer[0].idEvent) <= 0)
  {
    idx = 0;
    return cmp == 0;
  }
  if ((cmp = (int)idEvent - (int)g_activeTimer[rangeMax = g_nActiveTimers - 1].idEvent) > 0)
  {
    idx = g_nActiveTimers;
    return false;
  }
  if (cmp == 0)
  {
    idx = rangeMax;
    return true;
  }
  while (rangeMax > rangeMin + 1)
  {
    cmp = (int)idEvent - (int)g_activeTimer[rangeMid = (rangeMin + rangeMax) / 2].idEvent;
    if (cmp > 0) rangeMin = rangeMid;
    else if (cmp < 0) rangeMax = rangeMid;
    else {idx = rangeMid; return true;}
  }
  idx = rangeMax;
  return false;
}

void __stdcall TimerCallback1(HWND, UINT, UINT idEvent, DWORD)
{
  Timer *activeTimer;
  {
    WOCritSecLock CSLock(&g_timerCS);
    int idx;
    if (!ActiveTimerIdx(idx, idEvent))
    {
      OutputDebug("Error: tick received for non-existent timer\n");
      return;
    }
    activeTimer = g_activeTimer[idx].timer;
  }
  activeTimer->Tick();
}

Timer::Timer(unsigned int interval)
{
  unsigned int idEvent = SetTimer(NULL, 0, interval, TimerCallback1);
  WOCritSecLock CSLock(&g_timerCS);
  int idx;
  if (ActiveTimerIdx(idx, idEvent)) throw "Duplicate timer ID";
  if (!(g_nActiveTimers % 10))
  {
    g_activeTimer = (ActiveTimer *)ClientRealloc(g_activeTimer, (g_nActiveTimers + 10) * sizeof(ActiveTimer));
  }
  memmove(g_activeTimer + idx + 1, g_activeTimer + idx, (g_nActiveTimers++ - idx) * sizeof(ActiveTimer));
  m_timerData = (TimerData *)(g_activeTimer[idx].idEvent = idEvent);
  g_activeTimer[idx].timer = this;
}

Timer::~Timer()
{
  int idx;
  if (ActiveTimerIdx(idx, (UINT)m_timerData))
  {
    memmove(g_activeTimer + idx, g_activeTimer + idx + 1, (--g_nActiveTimers - idx) * sizeof(ActiveTimer));
  }
  else throw "Error: non-existent timer record";
  if (g_nActiveTimers < 1)
  {
    delete [] g_activeTimer;
    g_activeTimer = NULL;
  }
  KillTimer(NULL, (UINT)m_timerData);
}

void Timer::Tick() {}

WindObjFileStream::WindObjFileStream(const char *fileName, const char *mode)
{
  m_file = fopen(fileName, mode);
  if (!m_file)
  {
    if (tolower(mode[0]) == 'r') throw "File not found";
    else throw "Error accessing file";
  }
}

WindObjFileStream::~WindObjFileStream() {fclose(m_file);}

void WindObjFileStream::Open(const char *fileName, const char *mode, bool throwError)
{
  if (m_file) fclose(m_file);
  m_file = fopen(fileName, mode);
  if (!m_file && throwError)
  {
    if (tolower(mode[0]) == 'r') throw "File not found";
    else throw "Error accessing file";
  }
}

void WindObjFileStream::Read(void *buff, int nBytes)
{
  if (nBytes < 1) return;
  if (fread(buff, nBytes, 1, m_file) < 1) throw "Error reading file";
}

void WindObjFileStream::Write(const void *buff, int nBytes)
{
  if (nBytes < 1) return;
  if (fwrite(buff, nBytes, 1, m_file) < 1) throw "Error writing to file";
}

void WindObjFileStream::Seek(long offset, SeekOrigin origin)
{
  if (fseek(m_file, offset, (int)origin)) throw "Error in file seek";
}

bool WindObjFileStream::EndOfFile() const
{
  return feof(m_file) != 0;
}
