// FileWrite.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <string>
#include <string.h>
#include <Windows.h>
#include <fstream>
#include <stdio.h>
#include <vector>
#include <algorithm> 
#include <intrin.h>

#include "crc.h"
using namespace std;

//http://www.libpng.org/pub/png/book/chapter11.html#png.ch11.div.14
//International Text Annotations (iTXt)
//https://github.com/finnp/png-itxt/blob/master/index.js  (REf from javascript implementation)

 //79 + 5 + 0 + 0 + 0 + 10000
#define bufferSize 200
#define imageSize  0xF4240
#define BYTES_PER_DWORD 5

#define PNG_HEADER_SIZE 8
#define BUFFER_SIZE 0x1fff
#define PNG_CHUNK_HEADER_SIZE 8
#define PNG_SIG_CHUNK_TYPE "IHDR"

typedef struct ITXTDATA
{
	char* keyword;
	char* value;
} iTxt;

std::ifstream::pos_type filesize(const char* filename)
{
	std::ifstream in(filename, std::ifstream::ate | std::ifstream::binary);
	return in.tellg();
}

void DwordsToBytesArray(const DWORD* dwIn, BYTE* bOut, const UINT dwCount,
	const UINT bCount) {
	UINT i = 0, j = 0, k = 0, l = 0;
	BYTE bBuff[BYTES_PER_DWORD];

	for (i = 0; i < dwCount; i++) {
		*(DWORD*)bBuff = dwIn[i];
		for (k = BYTES_PER_DWORD; k > 0; k--) {
			if (j < bCount) {
				bOut[j++] = bBuff[k - 1];
			}
			else {
				break;
			}
		}
	}
}

LONG _stdcall PngGetOffset(HANDLE hFile, PBYTE chunkType, DWORD* error) 
{

	LONG offset = 8;
	//Skip first 8 bytes, because it is standard 
	//https://www.w3.org/TR/2003/REC-PNG-20031110/#5PNG-file-signature
	if (SetFilePointer(hFile, PNG_HEADER_SIZE, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
	{
		return -1;
	}
	DWORD bytesRead = 0, totalRead = 0;
	BYTE buffer[BUFFER_SIZE];

	for (;;) 
	{
		if (!ReadFile(hFile, &buffer[0], PNG_CHUNK_HEADER_SIZE, &bytesRead, NULL))
		{
			//Bad file format
			return -1;
	    }
		offset += PNG_CHUNK_HEADER_SIZE;
		if (bytesRead < PNG_CHUNK_HEADER_SIZE)
		{
			//Bad file format
			return -1;
		}
		const unsigned int size = buffer[3] | buffer[2] << 8 | buffer[1] << 16 | buffer[0] << 24;
		const unsigned char* tag = ((unsigned char*)&buffer[4]);
		if (memcmp(tag, PNG_SIG_CHUNK_TYPE, 4) != 0) {
			if (SetFilePointer(hFile, size + 4, NULL, FILE_CURRENT) == INVALID_SET_FILE_POINTER)
			{
				//cannot move file pointer properly / cannot find till the end.
				return -1;
			}
			offset += sizeof PNG_SIG_CHUNK_TYPE;
			continue;
		}
		else {
			return offset - sizeof(DWORD);
		}
	}
	return -1;
}

BOOL _stdcall PngPutITxt(HANDLE hFile, LONG offset, DWORD chunkSize, PBYTE chunkData, DWORD* error) 
{	
	//https://www.w3.org/TR/2003/REC-PNG-20031110/#5Chunk-layout
	if (SetFilePointer(hFile, offset, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER) 
	{
		return false;
	}
	DWORD dwBytesWritten;
	DWORD dwSignatureSizeBigEndian = _byteswap_ulong(chunkSize);

	//PUT [LENGTH] A four-byte unsigned integer (Table 5.1)
	if (!::WriteFile(hFile, &dwSignatureSizeBigEndian, sizeof(DWORD), &dwBytesWritten, NULL))
	{
		return false;
	}
	//PUT [CHUNK TYPE] A sequence of four bytes defining the chunk type. (eg. IHDR)
	if (!::WriteFile(hFile, PNG_SIG_CHUNK_TYPE, sizeof (PNG_SIG_CHUNK_TYPE) , &dwBytesWritten, NULL))
	{
		return false;
	}
	//PUT [CHUNK DATA] The data bytes appropriate to the chunk type, if any. This field can be of zero length.
	if (!::WriteFile(hFile, chunkData, chunkSize, &dwBytesWritten, NULL))
	{
		return false;
	}
	unsigned long checksum = crc_init();
	//check


}

//working version
void writeITXT(HANDLE fHandle, size_t fileSize, iTxt data) {
	
	int keyLen = min(79, strlen(data.keyword));
	DWORD bytesWritten = 0;
	DWORD bytesWrittenCurr;
	int totalLength = 0;
	DWORD hexValue = strlen(data.keyword) + strlen(data.value);
	char bOut[BYTES_PER_DWORD] = {0x00,0x00,0x00,(char)hexValue};
	char chunkType[BYTES_PER_DWORD] = {0x69,0x54,0x58,0x74};
	char hardCodeCRC[BYTES_PER_DWORD] = { 0x40, 0x75,0x49,0xF3 };
	
	
	string buffer = string(chunkType) + string(data.keyword) + string(data.value) + string(hardCodeCRC);
	totalLength =  sizeof(chunkType) - 1 + strlen(data.keyword) + strlen(data.value) + sizeof(hardCodeCRC) - 1;
	const char* str = buffer.c_str();
	const char* target = "IHDR";
	char* srcBuff = new char[imageSize];
	DWORD dwBytesRead = 0;
	bool isSuccess = ReadFile(fHandle, srcBuff, imageSize - 1, &dwBytesRead, NULL);

	int targetIdx = -1;
	for (DWORD i = 0; i < imageSize; i++) {
		if (srcBuff[i] == 0x49) {
			char checkStr[25] = { '\0' };
			memcpy_s(checkStr, sizeof target, srcBuff + i, sizeof target);
			string checkS(checkStr);
			string targetS(target);
			if (checkS == targetS) {
				targetIdx = i;
				break;
			}
			
		}
	}
	ptrdiff_t offset = (targetIdx) - 4;

	if (SetFilePointer(fHandle, offset, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
	{
		cout << "SetFilePointer error!" << endl;
	}

	DWORD dwSignatureSizeBigEndian = _byteswap_ulong(hexValue);
	WriteFile(fHandle, &dwSignatureSizeBigEndian, sizeof(DWORD), &bytesWrittenCurr, NULL);
	bytesWritten += bytesWrittenCurr;
	char* lastPartBuffs = new char[imageSize];
	memcpy_s(lastPartBuffs, imageSize - offset, srcBuff + offset, imageSize - offset);
	WriteFile(fHandle, str, totalLength, &bytesWrittenCurr, NULL);
	bytesWritten += bytesWrittenCurr;
	WriteFile(fHandle, lastPartBuffs, fileSize - offset, &bytesWrittenCurr, NULL);
	bytesWritten += bytesWrittenCurr;

	delete lastPartBuffs;
	delete srcBuff;
}

int main()
{
	wstring inputPath = L"D:\\Code\\C++\\FileOperationTest\\IcoDigest\\iconTest\\sample2.png";
	HANDLE hFile;
	//string filePath = "D:\\downloadTest\\sample.exe";
	//std::ifstream input(filePath, std::ios::binary);
	//// copies all data into buffer
	//std::vector<char> buffer(std::istreambuf_iterator<char>(input), {});

	hFile = CreateFile((LPCWSTR)inputPath.c_str(),                // name of the write
		GENERIC_READ | GENERIC_WRITE,          // open for writing
		(FILE_SHARE_READ | FILE_SHARE_WRITE |
			FILE_SHARE_DELETE),                      // do not share
		NULL,                   // default security
		OPEN_EXISTING,             // open existing file only
		FILE_ATTRIBUTE_NORMAL,  // normal file
		NULL);                  // no attr. template


	char keyword[100] = "keyword";
	char value[10000] = "value";
	iTxt data = {
		keyword,//123123
		value//123123
	};

	size_t fileSize = filesize("D:\\Code\\C++\\FileOperationTest\\IcoDigest\\iconTest\\sample2.png");
	writeITXT(hFile, fileSize, data);
	DWORD error;
	//LONG offset = PngGetOffset(hFile, (BYTE*)PNG_SIG_CHUNK_TYPE, &error);
	CloseHandle(hFile);


	const char* s = "123";
	const char* s1 = "456";

	char buffer[1000] = "";
	strcat_s(buffer, sizeof buffer,  s);
	strcat_s(buffer, sizeof buffer, s1);
	std::cout << sizeof buffer  << "Hello World!\n";
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
