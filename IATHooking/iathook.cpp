#include <Windows.h>
#include <stdio.h>

void do_iat_hooking(HMODULE hModule, LPCSTR lpcTargetModule, LPVOID NewFunction);
int __cdecl new_strcmp(const char* s1, const char* s2);
PIMAGE_IMPORT_DESCRIPTOR get_import_table(HMODULE hModule);
void hook_address(PIMAGE_THUNK_DATA pThunk, PVOID NewFunc);

#define TARGET "strcmp"

DWORD sourceAddr;

BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		do_iat_hooking(GetModuleHandleA(NULL), TARGET, new_strcmp);
		break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		//do_iat_hooking(GetModuleHandleA(NULL), TARGET, (LPVOID)sourceAddr);
		break;
	}
	return TRUE;
}

int __cdecl new_strcmp(const char* s1, const char* s2)
{
	_asm {
		xor eax, eax;
	}
}

//_declspec(naked) int new_strcmp()
//{
//	_asm {
//		push ebp;
//		mov ebp, esp;
//		xor eax, eax;
//		pop ebp;
//		ret 5;
//	}
//}


void do_iat_hooking(HMODULE hModule, LPCSTR lpcTargetModule, LPVOID NewFunction)
{
	PIMAGE_IMPORT_DESCRIPTOR ImportTable = get_import_table(hModule);

	PIMAGE_THUNK_DATA pFirstThunk, pOriginakFirstThunk;
	PIMAGE_IMPORT_BY_NAME pFuncData;

	while (*(WORD*)ImportTable != 0)
	{

		pFirstThunk = (PIMAGE_THUNK_DATA)((PBYTE)hModule + ImportTable->FirstThunk);
		pOriginakFirstThunk = (PIMAGE_THUNK_DATA)((PBYTE)hModule + ImportTable->OriginalFirstThunk);
		pFuncData = (PIMAGE_IMPORT_BY_NAME)((PBYTE)hModule + pOriginakFirstThunk->u1.AddressOfData);

		while (*((WORD*)pOriginakFirstThunk) != 0)
		{

			if (strcmp(lpcTargetModule, (char*)pFuncData->Name) == 0)
			{
				hook_address(pFirstThunk, NewFunction);
				printf("\n* Hooked! *\n");
			}

			pOriginakFirstThunk++;
			pFuncData = (PIMAGE_IMPORT_BY_NAME)((PBYTE)hModule + pOriginakFirstThunk->u1.AddressOfData);
			pFirstThunk++;
		}

		ImportTable++;
	}

}

PIMAGE_IMPORT_DESCRIPTOR get_import_table(HMODULE hModule)
{
	PIMAGE_DOS_HEADER dos_header = (PIMAGE_DOS_HEADER)hModule;
	PIMAGE_NT_HEADERS nt_headers = (PIMAGE_NT_HEADERS)((DWORD)dos_header + dos_header->e_lfanew);
	IMAGE_DATA_DIRECTORY data_directory = (IMAGE_DATA_DIRECTORY)(nt_headers->OptionalHeader.DataDirectory[1]);

	return (PIMAGE_IMPORT_DESCRIPTOR)((DWORD)hModule + data_directory.VirtualAddress);
}

void hook_address(PIMAGE_THUNK_DATA pThunk, PVOID NewFunc)
{
	DWORD CurrentProtect;
	DWORD junk;

	VirtualProtect(pThunk, 4096, PAGE_READWRITE, &CurrentProtect);
	sourceAddr = pThunk->u1.Function;
	pThunk->u1.Function = (DWORD)NewFunc;
	VirtualProtect(pThunk, 4096, PAGE_READWRITE, &junk);
}