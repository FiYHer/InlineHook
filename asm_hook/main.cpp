#include "asm_hook.hpp"

asm_hook* g_test = nullptr;

CONTEXT g_context;
LPVOID g_new_address = nullptr;

/* 我们的操作函数 */
__declspec(naked) void my_func()
{
	GetThreadContext(GetCurrentThread(), &g_context);
	MessageBoxA(0, "写我们的作弊代码", 0, 0);
	SetThreadContext(GetCurrentThread(), &g_context);

	_asm jmp g_new_address
}

BOOL WINAPI DllMain(_In_ void* handle, _In_ unsigned long type, _In_opt_ void*)
{
	if (type == 1)
	{
		HMODULE main_mod = GetModuleHandleA("WindowsProject1.exe");
		DWORD addr = (DWORD)main_mod + 0x11C2B;

		g_test = new asm_hook((DWORD64)addr, (DWORD64)my_func);
		g_new_address = g_test->get_jmp_address();
	}
	if (type == 0)
	{
		g_test->backup();
	}
	return TRUE;
}