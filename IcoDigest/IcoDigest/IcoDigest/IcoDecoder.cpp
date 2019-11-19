/*
*http://vitiy.info/Code/ico.cpp
*	code by Victor Laskin (victor.laskin@gmail.com)
*  rev 2 - 1bit color was added, fixes for bit mask
*
*
*
*	THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS
*	OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
*	WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
*	ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY
*	DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
*	DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
*	GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
*	INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
*	IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
*	OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
*	IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
*/

#include "stdafx.h"
#include "IcoDecoder.h"

// These next two structs represent how the icon information is stored
// in an ICO file.
typedef struct
{
	unsigned char bWidth;               // Width of the image
	unsigned char bHeight;              // Height of the image (times 2)
	unsigned char bColorCount;          // Number of colors in image (0 if >=8bpp)
	unsigned char bReserved;            // Reserved
	unsigned short wPlanes;              // Color Planes
	unsigned short wBitCount;            // Bits per pixel
	unsigned long dwBytesInRes;         // how many bytes in this resource?
	unsigned long dwImageOffset;        // where in the file is this image
} ICONDIRENTRY, *LPICONDIRENTRY;

typedef struct
{
	unsigned short      idReserved;   // Reserved
	unsigned short      idType;       // resource type (1 for icons)
	unsigned short      idCount;      // how many images?
									  //ICONDIRENTRY  idEntries[1]; // the entries for each image
} ICONDIR, *LPICONDIR;


// size - 40 bytes
//typedef struct {
//	unsigned long biSize;
//	unsigned long  biWidth;
//	unsigned long  biHeight;				// Icon Height (added height of XOR-Bitmap and AND-Bitmap)
//	unsigned short  biPlanes;
//	unsigned short  biBitCount;
//	unsigned long biCompression;
//	long biSizeImage;
//	unsigned long  biXPelsPerMeter;
//	unsigned long  biYPelsPerMeter;
//	unsigned long biClrUsed;
//	unsigned long biClrImportant;
//} BITMAPINFOHEADER, *PBITMAPINFOHEADER;

// 46 bytes
typedef struct {
	BITMAPINFOHEADER   icHeader;      		// DIB header
	unsigned long		icColors[1];   		// Color table (short 4 bytes) //RGBQUAD
	unsigned char            icXOR[1];      // DIB bits for XOR mask
	unsigned char            icAND[1];      // DIB bits for AND mask
} ICONIMAGE, *LPICONIMAGE;


bool IcoDecoder::decode(unsigned char* buffer,							///< input buffer data
	int size, 										///< size of buffer
	unsigned int & width, 							///< output - width
	unsigned int & height, 							///< output - height
	std::vector<unsigned char> & image				///< output - image data
)
{
	LPICONDIR icoDir = (LPICONDIR)buffer;

	//LOG << "ICO:" <<  icoDir->idReserved << " "<<  icoDir->idType << " "<<  icoDir->idCount << NL;
	int iconsCount = icoDir->idCount;

	if (icoDir->idReserved != 0) return false;
	if (icoDir->idType != 1) return false;
	if (iconsCount == 0) return false;
	if (iconsCount > 20) return false;

	unsigned char* cursor = buffer;
	cursor += 6;
	ICONDIRENTRY* dirEntry = (ICONDIRENTRY*)(cursor);

	int maxSize = 0;
	int offset = 0;
	for (int i = 0; i < iconsCount; i++)
	{
		int w = dirEntry->bWidth;
		int h = dirEntry->bHeight;
		int colorCount = dirEntry->bColorCount;
		int bitCount = dirEntry->wBitCount;

		if (w * h > maxSize) // we choose icon with max resolution
		{
			width = w;
			height = h;
			offset = dirEntry->dwImageOffset;
			maxSize = w * h;
		}

		//LOG << "Size:" << w << "x" << h << " bits:" << bitCount  << " colors: " << colorCount << " offset: " << (int)(dirEntry->dwImageOffset)  << " bytes: " << (int)(dirEntry->dwBytesInRes) << NL;

		dirEntry++;
	}

	if (offset == 0) return false;

	//LOG << "Offset: " << offset << NL;
	cursor = buffer;
	cursor += offset;


	ICONIMAGE* icon = (ICONIMAGE*)(cursor);
	//LOG << "BitmapInfo: Struct: " << (int)(icon->icHeader.biSize) << " Size: " << (int)(icon->icHeader.biWidth) << "x" << (int)(icon->icHeader.biHeight) << " Bytes: " << (int)(icon->icHeader.biSizeImage) << NL;
	//LOG << "BitmapInfo: Planes: " << (int)icon->icHeader.biPlanes << " bits: " << (int)icon->icHeader.biBitCount << " Compression: " << (int)icon->icHeader.biCompression << NL;
	int realBitsCount = (int)icon->icHeader.biBitCount;
	bool hasAndMask = (realBitsCount < 32) && (height != icon->icHeader.biHeight);

	cursor += 40;
	int numBytes = width * height * 4;
	image.resize(numBytes);

	// rgba + vertical swap
	if (realBitsCount == 32)
	{
		int shift;
		int shift2;
		for (int x = 0; x < width; x++)
			for (int y = 0; y < height; y++)
			{
				shift = 4 * (x + y * width);
				shift2 = 4 * (x + (height - y - 1) * width);
				image[shift] = cursor[shift2 + 2];
				image[shift + 1] = cursor[shift2 + 1];
				image[shift + 2] = cursor[shift2];
				image[shift + 3] = cursor[shift2 + 3];
			}
	}

	if (realBitsCount == 24)
	{
		int shift;
		int shift2;
		for (int x = 0; x < width; x++)
			for (int y = 0; y < height; y++)
			{
				shift = 4 * (x + y * width);
				shift2 = 3 * (x + (height - y - 1) * width);
				image[shift] = cursor[shift2 + 2];
				image[shift + 1] = cursor[shift2 + 1];
				image[shift + 2] = cursor[shift2];
				image[shift + 3] = 255;
			}
	}

	if (realBitsCount == 8)  /// 256 colors
	{
		// 256 color table
		unsigned char * colors = (unsigned char *)cursor;
		cursor += 256 * 4;
		int shift;
		int shift2;
		int index;
		for (int x = 0; x < width; x++)
			for (int y = 0; y < height; y++)
			{
				shift = 4 * (x + y * width);
				shift2 = (x + (height - y - 1) * width);
				index = 4 * cursor[shift2];
				image[shift] = colors[index + 2];
				image[shift + 1] = colors[index + 1];
				image[shift + 2] = colors[index];
				image[shift + 3] = 255;
			}
	}

	if (realBitsCount == 4)  /// 16 colors
	{
		// 16 color table
		unsigned char * colors = (unsigned char *)cursor;
		cursor += 16 * 4;
		int shift;
		int shift2;
		unsigned char index;
		for (int x = 0; x < width; x++)
			for (int y = 0; y < height; y++)
			{
				shift = 4 * (x + y * width);
				shift2 = (x + (height - y - 1) * width);
				index = cursor[shift2 / 2];
				if (shift2 % 2 == 0)
					index = (index >> 4) & 0xF;
				else
					index = index & 0xF;
				index *= 4;

				image[shift] = colors[index + 2];
				image[shift + 1] = colors[index + 1];
				image[shift + 2] = colors[index];
				image[shift + 3] = 255;
			}
	}

	if (realBitsCount == 1)  /// 2 colors
	{
		// 2 color table
		unsigned char * colors = (unsigned char *)cursor;
		cursor += 2 * 4;
		int shift;
		int shift2;
		unsigned char index;
		unsigned char bit;

		int boundary = width; //!!! 32 bit boundary (http://www.daubnet.com/en/file-format-ico)
		while (boundary % 32 != 0) boundary++;

		for (int x = 0; x < width; x++)
			for (int y = 0; y < height; y++)
			{
				shift = 4 * (x + y * width);
				shift2 = (x + (height - y - 1) * boundary);
				index = cursor[shift2 / 8];

				// select 1 bit only
				bit = 7 - (x % 8);
				index = (index >> bit) & 0x01;
				index *= 4;

				image[shift] = colors[index + 2];
				image[shift + 1] = colors[index + 1];
				image[shift + 2] = colors[index];
				image[shift + 3] = 255;
			}
	}

	// Read AND mask after base color data - 1 BIT MASK
	if (hasAndMask)
	{
		int shift;
		int shift2;
		unsigned char bit;
		int mask;

		int boundary = width * realBitsCount; //!!! 32 bit boundary (http://www.daubnet.com/en/file-format-ico)
		while (boundary % 32 != 0) boundary++;
		cursor += boundary * height / 8;

		boundary = width;
		while (boundary % 32 != 0) boundary++;

		for (int y = 0; y < height; y++)
			for (int x = 0; x < width; x++)
			{
				shift = 4 * (x + y * width) + 3;
				bit = 7 - (x % 8);
				shift2 = (x + (height - y - 1) * boundary) / 8;
				mask = (0x01 & ((unsigned char)cursor[shift2] >> bit));
				//LOG << "Bit: " << bit << "Value: " << mask << " from byte: " << cursor[shift2] << " row: " << y << " index:" << shift2 << NL;
				image[shift] *= 1 - mask;

			}
	}

	return true;
}


