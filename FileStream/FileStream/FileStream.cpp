// FileStream.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <windows.h>
#include <fstream>
#include <stdio.h>
#include <cstdio>
#include <sys/stat.h>
#include <string>
#include <vector>


#include <wincrypt.h>
#include <WinTrust.h>
#include <Softpub.h>

#pragma comment (lib, "wintrust")
#pragma comment (lib, "crypt32.lib")
#define ENCODING (X509_ASN_ENCODING | PKCS_7_ASN_ENCODING)

#define ReadBufferSize 31
using namespace std;

std::string GetLastErrorAsString()
{
	//Get the error message, if any.
	DWORD errorMessageID = ::GetLastError();
	if (errorMessageID == 0)
		return std::string(); //No error message has been recorded

	LPSTR messageBuffer = nullptr;
	size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);

	std::string message(messageBuffer, size);

	//Free the buffer.
	LocalFree(messageBuffer);

	return message;
}

static bool CheckSignature(const std::wstring& dllPath, HANDLE& hfile, LPVOID pSIPClientData, std::wstring& reason) {
	bool isTrusted = false;
	std::vector<std::string> certificateChain;

	WINTRUST_FILE_INFO file_info = { 0 };
	file_info.cbStruct = sizeof(file_info);
	file_info.pcwszFilePath = dllPath.c_str();
	file_info.hFile = hfile;
	file_info.pgKnownSubject = NULL;

	WINTRUST_DATA wintrust_data = { 0 };
	wintrust_data.cbStruct = sizeof(wintrust_data);
	wintrust_data.pPolicyCallbackData = NULL;
	wintrust_data.pSIPClientData = NULL;
	wintrust_data.dwUIChoice = WTD_UI_NONE;
	wintrust_data.fdwRevocationChecks = WTD_REVOKE_NONE;
	wintrust_data.dwUnionChoice = WTD_CHOICE_FILE;
	wintrust_data.pFile = &file_info;
	wintrust_data.dwStateAction = WTD_STATEACTION_VERIFY;
	wintrust_data.hWVTStateData = NULL;
	wintrust_data.pwszURLReference = NULL;
	// Disallow revocation checks over the network.
	wintrust_data.dwProvFlags = WTD_CACHE_ONLY_URL_RETRIEVAL;
	wintrust_data.dwUIContext = WTD_UICONTEXT_EXECUTE;

	// The WINTRUST_ACTION_GENERIC_VERIFY_V2 policy verifies that the certificate
	// chains up to a trusted root CA, and that it has appropriate permission to
	// sign code.
	GUID policy_guid = WINTRUST_ACTION_GENERIC_VERIFY_V2;  //WINTRUST_ACTION_GENERIC_CERT_VERIFY  WINTRUST_ACTION_GENERIC_VERIFY_V2

	LONG result = WinVerifyTrust(static_cast<HWND>(INVALID_HANDLE_VALUE), &policy_guid, &wintrust_data);

	CRYPT_PROVIDER_DATA* prov_data = WTHelperProvDataFromStateData(wintrust_data.hWVTStateData);
	if (prov_data) {
		if (prov_data->csSigners > 0) {
			if (result == ERROR_SUCCESS) {
				isTrusted = true;
			}
		}
		// Free the provider data.
		wintrust_data.dwStateAction = WTD_STATEACTION_CLOSE;
		WinVerifyTrust(static_cast<HWND>(INVALID_HANDLE_VALUE), &policy_guid, &wintrust_data);
	}

	if (!isTrusted) {
		reason = dllPath + L" not trusted";
		return false;
	}
	return true;
}

int main()
{
	//ofstream myfile;
	/*string filePath = "C:\\Users\\jixiang.li\\AppData\\Local\\Chromium\\User Data\\Apps\\helloworld_folder\\dll\\ThxNativeLite.dll";*/
	string filePath = "D:\\downloadTest\\certTest2\\buffer.bin";
	
	std::ifstream input(filePath, std::ios::binary);
	// copies all data into buffer
	std::vector<char> buffer(std::istreambuf_iterator<char>(input), {});

	//FILE* memFile = open_memstream(buffer, buffer.size());
	

	HANDLE WINHandle = CreateFile(TEXT("D:\\downloadTest\\certTest2\\org.png"), (GENERIC_WRITE | GENERIC_READ), (FILE_SHARE_READ | FILE_SHARE_WRITE |
		FILE_SHARE_DELETE), 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0); // (FILE_ATTRIBUTE_TEMPORARY) | FILE_FLAG_DELETE_ON_CLOSE

	DWORD bytesWritten;
	char *str = reinterpret_cast<char*>(buffer.data());
	//WriteFile(WINHandle, str, buffer.size(), &bytesWritten, NULL);

	//DWORD  dwCurrentFilePosition = SetFilePointer(
	//	WINHandle, // must have GENERIC_READ and/or GENERIC_WRITE
	//	0, // do not move pointer
	//	NULL, // hFile is not large enough in needing this 
	//	FILE_BEGIN); // provides offset from current position

	//printf("Current file pointer position: %d\n", dwCurrentFilePosition);

	//char buff[ReadBufferSize];
	//DWORD dwBytesRead = 0;
	//bool isSuccess = ReadFile(WINHandle, buff, ReadBufferSize - 1, &dwBytesRead, NULL);
	//if (isSuccess) {
	//	buff[dwBytesRead] = '\0';
	//	printf("%s\n", buff);
	//}

	//string s = GetLastErrorAsString();
	//cout << s << endl;

	//HANDLE WINHandle2 = CreateFile(TEXT("D:\\downloadTest\\hello"), (GENERIC_WRITE | GENERIC_READ), (FILE_SHARE_READ | FILE_SHARE_WRITE |
	//	FILE_SHARE_DELETE), 0, OPEN_EXISTING, (FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE), 0); // (FILE_ATTRIBUTE_TEMPORARY) | FILE_FLAG_DELETE_ON_CLOSE


	//char buff[ReadBufferSize];
	//DWORD dwBytesRead = 0;
	//bool isSuccess = ReadFile(WINHandle2, buff, ReadBufferSize - 1, &dwBytesRead, NULL);
	//if (isSuccess) {
	//	buff[dwBytesRead] = '\0';
	//	printf("%s\n", buff);
	//}
	//string s = GetLastErrorAsString();
	//cout << s << endl;

	std::wstring reason;

	HANDLE try1 = NULL;
	//int* ptr = 0;
	//*ptr = 1;
	bool isRazerSigned = CheckSignature(TEXT("D:\\downloadTest\\certTest2\\org.png"), WINHandle, buffer.data(), reason);
	//bool isRazerSigned = CheckSignature(TEXT(""), WINHandle, NULL, reason);


	CloseHandle(WINHandle);
	//CloseHandle(WINHandle2);
	return 1;
}
