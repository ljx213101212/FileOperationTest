#include "stdafx.h"
#include "DIB.h"
LPSTR FindDIBBits(LPSTR lpbi)
{
	return (lpbi + *(LPDWORD)lpbi + PaletteSize(lpbi));
}

WORD DIBNumColors(LPSTR lpbi)
{
	WORD wBitCount;
	DWORD dwClrUsed;

	dwClrUsed = ((LPBITMAPINFOHEADER)lpbi)->biClrUsed;

	if (dwClrUsed)
		return (WORD)dwClrUsed;

	wBitCount = ((LPBITMAPINFOHEADER)lpbi)->biBitCount;

	switch (wBitCount)
	{
	case 1: return 2;
	case 4: return 16;
	case 8:	return 256;
	default:return 0;
	}
	return 0;
}

WORD PaletteSize(LPSTR lpbi)
{
	return (DIBNumColors(lpbi) * sizeof(RGBQUAD));
}

DWORD BytesPerLine(LPBITMAPINFOHEADER lpBMIH)
{
	return WIDTHBYTES(lpBMIH->biWidth * lpBMIH->biPlanes * lpBMIH->biBitCount);
}