// IcoDigest.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "IcoDecoder.h"
#include "Icon.h"
#include "DIB.h"

//typedef struct
//{
//	WORD           idReserved;   // Reserved (must be 0)
//	WORD           idType;       // Resource Type (1 for icons)
//	WORD           idCount;      // How many images?
//	ICONDIRENTRY   idEntries[1]; // An entry for each image (idCount of 'em)
//} ICONDIR, *LPICONDIR;
//
//
//typedef struct
//{
//	BYTE        bWidth;          // Width, in pixels, of the image
//	BYTE        bHeight;         // Height, in pixels, of the image
//	BYTE        bColorCount;     // Number of colors in image (0 if >=8bpp)
//	BYTE        bReserved;       // Reserved ( must be 0)
//	WORD        wPlanes;         // Color Planes
//	WORD        wBitCount;       // Bits per pixel
//	DWORD       dwBytesInRes;    // How many bytes in this resource?
//	DWORD       dwImageOffset;   // Where in the file is this image?
//} ICONDIRENTRY, *LPICONDIRENTRY;

//typedef struct
//{
//	BITMAPINFOHEADER   icHeader;      // DIB header
//	RGBQUAD         icColors[1];   // Color table
//	BYTE            icXOR[1];      // DIB bits for XOR mask
//	BYTE            icAND[1];      // DIB bits for AND mask
//} ICONIMAGE, *LPICONIMAGE;

/****************************************************************************/
// Structs used locally (file scope)
// Resource Position info - size and offset of a resource in a file

#define BUFFER_SIZE 0x20000
#define PNG_SIG_SIZE 8

typedef struct ICO_FILE_INFO_
{
	LONG beforePNGStartPosition;
	LONG PNGStartPosition;
	LONG afterPNGStartPosition;
	LONG fileEndPosition;
} ICO_FILE_INFO;

typedef struct
{
	DWORD	dwBytes;
	DWORD	dwOffset;
} RESOURCEPOSINFO, *LPRESOURCEPOSINFO;

// External Globals
//extern HWND        	hWndMain;

// Prototypes for local functions
UINT ReadICOHeader(HANDLE hFile);
BOOL AdjustIconImagePointers(LPICONIMAGE lpImage);
DWORD  GetFilePointer(HANDLE hFile) {
	return SetFilePointer(hFile, 0, NULL, FILE_CURRENT);
}

BOOL CheckPNGSignature(LPBYTE chunkData) {
	BYTE bufferSIG[PNG_SIG_SIZE];
	const BYTE PNGSIG[PNG_SIG_SIZE] = { 0x89,0x50,0x4E,0x47,0x0D,0x0A,0x1A,0x0A };
	memcpy_s(&bufferSIG, PNG_SIG_SIZE, chunkData, PNG_SIG_SIZE);
	return (0 == memcmp(bufferSIG, PNGSIG, PNG_SIG_SIZE));
}



LPICONRESOURCE ReadIconFromICOFile(LPCTSTR szFileName)
{
	LPICONRESOURCE    	lpIR = NULL, lpNew = NULL;
	HANDLE            	hFile = NULL;
	LPRESOURCEPOSINFO	lpRPI = NULL;
	UINT                i;
	DWORD            	dwBytesRead;
	LPICONDIRENTRY    	lpIDE = NULL;

	ICO_FILE_INFO info;
	// Open the file
	if ((hFile = CreateFile(szFileName, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE)
	{
		//MessageBox(hWndMain, TEXT("Error Opening File for Reading"), szFileName, MB_OK);
		return NULL;
	}
	info.beforePNGStartPosition = GetFilePointer(hFile);
	// Allocate memory for the resource structure
	if ((lpIR = (LPICONRESOURCE)malloc(sizeof(ICONRESOURCE))) == NULL)
	{
		//MessageBox(hWndMain, TEXT("Error Allocating Memory"), szFileName, MB_OK);
		CloseHandle(hFile);
		return NULL;
	}
	// Read in the header
	if ((lpIR->nNumImages = ReadICOHeader(hFile)) == (UINT)-1)
	{
		//MessageBox(hWndMain, TEXT("Error Reading File Header"), szFileName, MB_OK);
		CloseHandle(hFile);
		free(lpIR);
		return NULL;
	}
	// Adjust the size of the struct to account for the images
	if ((lpNew = (LPICONRESOURCE)realloc(lpIR, sizeof(ICONRESOURCE) + ((lpIR->nNumImages - 1) * sizeof(ICONIMAGE)))) == NULL)
	{
		//MessageBox(hWndMain, TEXT("Error Allocating Memory"), szFileName, MB_OK);
		CloseHandle(hFile);
		free(lpIR);
		return NULL;
	}
	lpIR = lpNew;
	// Store the original name
	lstrcpy(lpIR->szOriginalICOFileName, szFileName);
	lstrcpy(lpIR->szOriginalDLLFileName, TEXT(""));
	// Allocate enough memory for the icon directory entries
	if ((lpIDE = (LPICONDIRENTRY)malloc(lpIR->nNumImages * sizeof(ICONDIRENTRY))) == NULL)
	{
		//MessageBox(hWndMain, TEXT("Error Allocating Memory"), szFileName, MB_OK);
		CloseHandle(hFile);
		free(lpIR);
		return NULL;
	}
	// Read in the icon directory entries
	if (!ReadFile(hFile, lpIDE, lpIR->nNumImages * sizeof(ICONDIRENTRY), &dwBytesRead, NULL))
	{
		//MessageBox(hWndMain, TEXT("Error Reading File"), szFileName, MB_OK);
		CloseHandle(hFile);
		free(lpIR);
		return NULL;
	}
	if (dwBytesRead != lpIR->nNumImages * sizeof(ICONDIRENTRY))
	{
		//MessageBox(hWndMain, TEXT("Error Reading File"), szFileName, MB_OK);
		CloseHandle(hFile);
		free(lpIR);
		return NULL;
	}

	// Loop through and read in each image
	for (i = 0; i < lpIR->nNumImages; i++)
	{
		// Allocate memory for the resource
		if ((lpIR->IconImages[i].lpBits = (LPBYTE)malloc(lpIDE[i].dwBytesInRes)) == NULL)
		{
			//MessageBox(hWndMain, TEXT("Error Allocating Memory"), szFileName, MB_OK);
			CloseHandle(hFile);
			free(lpIR);
			free(lpIDE);
			return NULL;
		}
	
		lpIR->IconImages[i].dwNumBytes = lpIDE[i].dwBytesInRes;
		// Seek to beginning of this image
		if (SetFilePointer(hFile, lpIDE[i].dwImageOffset, NULL, FILE_BEGIN) == 0xFFFFFFFF)
		{
			//MessageBox(hWndMain, TEXT("Error Seeking in File"), szFileName, MB_OK);
			CloseHandle(hFile);
			free(lpIR);
			free(lpIDE);
			return NULL;
		}
	
		// Read it in
		if (!ReadFile(hFile, lpIR->IconImages[i].lpBits, lpIDE[i].dwBytesInRes, &dwBytesRead, NULL))
		{
			//MessageBox(hWndMain, TEXT("Error Reading File"), szFileName, MB_OK);
			CloseHandle(hFile);
			free(lpIR);
			free(lpIDE);
			return NULL;
		}

		//check PNG SIG
		if (CheckPNGSignature(lpIR->IconImages[i].lpBits)) {
			info.afterPNGStartPosition = GetFilePointer(hFile);
			info.PNGStartPosition = info.afterPNGStartPosition - dwBytesRead;
		}
		if (dwBytesRead != lpIDE[i].dwBytesInRes)
		{
			//MessageBox(hWndMain, TEXT("Error Reading File"), szFileName, MB_OK);
			CloseHandle(hFile);
			free(lpIDE);
			free(lpIR);
			return NULL;
		}
		// Set the internal pointers appropriately
		if (!AdjustIconImagePointers(&(lpIR->IconImages[i])))
		{
			//MessageBox(hWndMain, TEXT("Error Converting to Internal Format"), szFileName, MB_OK);
			CloseHandle(hFile);
			free(lpIDE);
			free(lpIR);
			return NULL;
		}
	}
	info.fileEndPosition = GetFilePointer(hFile);

	SetFilePointer(hFile, 0, NULL, FILE_BEGIN);
	BYTE testData[BUFFER_SIZE];
	ReadFile(hFile, &testData, info.PNGStartPosition - info.beforePNGStartPosition, &dwBytesRead, NULL);
	ReadFile(hFile, &testData, info.afterPNGStartPosition - info.PNGStartPosition, &dwBytesRead, NULL);
	ReadFile(hFile, &testData, info.fileEndPosition - info.afterPNGStartPosition, &dwBytesRead, NULL);
	// Clean up	
	free(lpIDE);
	free(lpRPI);
	CloseHandle(hFile);
	return lpIR;
}
/* End ReadIconFromICOFile() **********************************************/




UINT ReadICOHeader(HANDLE hFile)
{
	WORD    Input;
	DWORD	dwBytesRead;

	// Read the 'reserved' WORD
	if (!ReadFile(hFile, &Input, sizeof(WORD), &dwBytesRead, NULL))
		return (UINT)-1;
	// Did we get a WORD?
	if (dwBytesRead != sizeof(WORD))
		return (UINT)-1;
	// Was it 'reserved' ?   (ie 0)
	if (Input != 0)
		return (UINT)-1;
	// Read the type WORD
	if (!ReadFile(hFile, &Input, sizeof(WORD), &dwBytesRead, NULL))
		return (UINT)-1;
	// Did we get a WORD?
	if (dwBytesRead != sizeof(WORD))
		return (UINT)-1;
	// Was it type 1?
	if (Input != 1)
		return (UINT)-1;
	// Get the count of images
	if (!ReadFile(hFile, &Input, sizeof(WORD), &dwBytesRead, NULL))
		return (UINT)-1;
	// Did we get a WORD?
	if (dwBytesRead != sizeof(WORD))
		return (UINT)-1;
	// Return the count
	return Input;
}

BOOL AdjustIconImagePointers(LPICONIMAGE lpImage)
{
	// Sanity check
	if (lpImage == NULL)
		return FALSE;
	// BITMAPINFO is at beginning of bits
	lpImage->lpbi = (LPBITMAPINFO)lpImage->lpBits;
	// Width - simple enough
	lpImage->Width = lpImage->lpbi->bmiHeader.biWidth;
	// Icons are stored in funky format where height is doubled - account for it
	lpImage->Height = (lpImage->lpbi->bmiHeader.biHeight) / 2;
	// How many colors?
	lpImage->Colors = lpImage->lpbi->bmiHeader.biPlanes * lpImage->lpbi->bmiHeader.biBitCount;
	// XOR bits follow the header and color table
	lpImage->lpXOR = (LPBYTE)FindDIBBits((LPSTR)lpImage->lpbi);
	// AND bits follow the XOR bits
	lpImage->lpAND = lpImage->lpXOR + (lpImage->Height*BytesPerLine((LPBITMAPINFOHEADER)(lpImage->lpbi)));
	return TRUE;
}



int main()
{
	/*LPICONDIR pIconDir;
	pIconDir = (LPICONDIR)malloc(sizeof(ICONDIR));*/

	/*HANDLE WINHandle = CreateFile(TEXT("D:\\downloadTest\\hello"), (GENERIC_WRITE | GENERIC_READ), (FILE_SHARE_READ | FILE_SHARE_WRITE |
		FILE_SHARE_DELETE), 0, CREATE_ALWAYS, (FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE), 0);*/ // (FILE_ATTRIBUTE_TEMPORARY) | FILE_FLAG_DELETE_ON_CLOSE

	LPICONRESOURCE source = ReadIconFromICOFile(TEXT("D:\\downloadTest\\icoDigest\\rzappengine.ico"));
	/*IcoDecoder ico = IcoDecoder();
	ico.decode();*/

    return 0;
}

