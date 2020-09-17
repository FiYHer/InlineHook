#include <string>

#include "asm_hook.hpp"

asm_hook* g_test = nullptr;

/* 我们的操作函数 */
__declspec(naked) void my_func()
{
	g_test->begin();
	MessageBoxA(0, "写我们的作弊代码", 0, 0);
	g_test->end();

	g_test->jmp_original();
}

BOOL WINAPI DllMain(_In_ void* handle, _In_ unsigned long type, _In_opt_ void*)
{
	if (type == 1)
	{
		HMODULE main_mod = GetModuleHandleA("target.exe");
		DWORD addr = (DWORD)main_mod + 0x1508B;
		g_test = new asm_hook((DWORD64)addr, (DWORD64)my_func);

		MessageBoxA(0, std::to_string(addr).c_str(), "hook", 0);
	}
	if (type == 0)
	{
		g_test->backup();
		MessageBoxA(0, "backup", "un hook", 0);
	}
	return TRUE;
}