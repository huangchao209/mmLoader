// demo-mmloader-static.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <windows.h>
#include <strsafe.h>
#include "..\..\output\include\mmLoader\mmLoader.h"

int main()
{
	// Return value
	int iRet = -1;

	// Initialize function table
	NTFUNCPTRSTABLE sNtFuncPtrsTable;
	sNtFuncPtrsTable.pfnGetModuleHandleA = ::GetModuleHandleA;
	sNtFuncPtrsTable.pfnLoadLibraryA = ::LoadLibraryA;
	sNtFuncPtrsTable.pfnGetProcAddress = ::GetProcAddress;
	sNtFuncPtrsTable.pfnVirtualAlloc = ::VirtualAlloc;
	sNtFuncPtrsTable.pfnVirtualFree = ::VirtualFree;
	sNtFuncPtrsTable.pfnVirtualProtect = ::VirtualProtect;

	// Initialize MEM_MODULE
	MEM_MODULE sMemModule;
	sMemModule.pNtFuncptrsTable = &sNtFuncPtrsTable;

	// Load the module
#ifdef _DEBUG
	WCHAR wszDllPath[] = L"demo-moduled.dll";
#else
	WCHAR wszDllPath[] = L"demo-module.dll";
#endif

	// Open the module and read it into memory buffer
	BOOL br = FALSE;
	HANDLE hFile = ::CreateFileW(wszDllPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL);
	if (INVALID_HANDLE_VALUE == hFile || NULL == hFile)
	{
		wprintf(L"Failed to open the file: %s\r\n", wszDllPath);
		return iRet;
	}

	// Check file size
	DWORD dwFileSize = ::GetFileSize(hFile, NULL);
	if (INVALID_FILE_SIZE == dwFileSize || dwFileSize < sizeof(IMAGE_DOS_HEADER))
	{
		::CloseHandle(hFile);
		_tprintf(_T("Invalid file size: %d\r\n"), dwFileSize);
		return iRet;
	}

	HANDLE hFileMapping = ::CreateFileMappingW(hFile, 0, PAGE_READONLY, 0, 0, NULL);
	if (NULL == hFileMapping)
	{
		::CloseHandle(hFile);
		_tprintf(_T("Failed to create file mapping.\r\n"));
		return iRet;
	}

	LPVOID pBuffer = ::MapViewOfFile(hFileMapping, FILE_MAP_READ, 0, 0, 0);
	if (NULL == pBuffer)
	{
		::CloseHandle(hFileMapping);
		::CloseHandle(hFile);
		_tprintf(_T("Failed to map view of the file.\r\n"));
		return iRet;
	}

	if (MemModuleHelper(&sMemModule, MHM_BOOL_LOAD, pBuffer, NULL, TRUE))
	{
		::UnmapViewOfFile(pBuffer);
		::CloseHandle(hFileMapping);
		::CloseHandle(hFile);

		_tprintf(_T("Module was loaded successfully. Module Base: 0x%p!\r\n"), sMemModule.lpBase);

		// Get address of function demoFunction
		LPVOID lpAddr = (LPVOID)MemModuleHelper(&sMemModule, MHM_FARPROC_GETPROC, NULL, "demoFunction", FALSE);
		if (lpAddr)
		{
			_tprintf(_T("Get address of demoFunction successfully. Address: 0x%p!\r\n"), lpAddr);

			// Function pointer type of demoFunction
			typedef BOOL (_stdcall * Type_TargetFunction)(unsigned char*, unsigned int);

			// Call the demoFunction
			Type_TargetFunction pfnFunction = (Type_TargetFunction)lpAddr;

			unsigned char buf[MAX_PATH] = { 0 };
			if (pfnFunction(buf, MAX_PATH))
			{
				char* p = "{f56fee02-16d1-44a3-b191-4d7535f92ca5}";
				iRet = ::memcmp(buf, p, strlen(p));
				if (0 == iRet)
					_tprintf(_T("Called target function demoFunction successfully with correct return value!\r\n"));
				else
					_tprintf(_T("Called target function demoFunction successfully, but returned unexpected value!\r\n"));
			}
		}
		else
			_tprintf(_T("Failed to get address of demoFunction from memory module.\r\n"));

		// Free the module
		MemModuleHelper(&sMemModule, MHM_VOID_FREE, NULL, NULL, FALSE);
	}
	else
		_tprintf(_T("Failed to load the module!\r\n"));

	return iRet;
}

