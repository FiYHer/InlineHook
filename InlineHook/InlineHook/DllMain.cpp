
#include "InlineHook.h"

InlineHook g_Hook;

int
WINAPI
MyMessageBoxA(
	_In_opt_ HWND hWnd,
	_In_opt_ LPCSTR lpText,
	_In_opt_ LPCSTR lpCaption,
	_In_ UINT uType)
{
	g_Hook.ReduceAddress();
	int nRet = MessageBoxA(0, "你被劫持了", 0, 0);
	g_Hook.ModifyAddress();
	return nRet;
}

BOOL WINAPI DllMain(
	_In_ void* _DllHandle,
	_In_ unsigned long _Reason,
	_In_opt_ void* _Reserved)
{
	if (_Reason == DLL_PROCESS_ATTACH)
	{
		if (g_Hook.Initialize(reinterpret_cast<int>(MessageBoxA), reinterpret_cast<int>(MyMessageBoxA)))
			if (g_Hook.ModifyAddress())
				MessageBoxW(0, L"成功Hook", 0, 0);
	}
	if(_Reason == DLL_PROCESS_DETACH)
	{
		g_Hook.ReduceAddress();
	}
	return TRUE;
}