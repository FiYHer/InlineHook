#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <process.h>
#include "D3DHook.h"

d3dhook::D3DHook g_Hook;

HRESULT _stdcall MyReset(IDirect3DDevice9* pDirect3DDevice, D3DPRESENT_PARAMETERS* pPresentationParameters)
{
	g_Hook.SetGameDirect3DDevicePoint(pDirect3DDevice);
	g_Hook.ReduceAddress(d3dhook::D3dClass::Class_IDirect3DDevice9, d3dhook::Direct3DDevice_Function::F_Reset);

	static int nIndex = 0;
	std::cout << nIndex++ << std::endl;
	HRESULT hRet = pDirect3DDevice->Reset(pPresentationParameters);

	g_Hook.ModifyAddress(d3dhook::D3dClass::Class_IDirect3DDevice9, d3dhook::Direct3DDevice_Function::F_Reset);
	return hRet;
}

HRESULT _stdcall MyPresent(IDirect3DDevice9* pDirect3DDevice, CONST RECT* pSourceRect, CONST RECT* pDestRect, HWND hDestWindowOverride, CONST RGNDATA* pDirtyRegion)
{
	g_Hook.SetGameDirect3DDevicePoint(pDirect3DDevice);
	g_Hook.ReduceAddress(d3dhook::D3dClass::Class_IDirect3DDevice9, d3dhook::Direct3DDevice_Function::F_Present);

	static int nIndex = 0;
	std::cout << nIndex++ << std::endl;
	HRESULT hRet = pDirect3DDevice->Present(pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion);

	g_Hook.ModifyAddress(d3dhook::D3dClass::Class_IDirect3DDevice9, d3dhook::Direct3DDevice_Function::F_Present);
	return hRet;
}

void InitializeHook(void* pBuf)
{
	if (d3dhook::InitializeD3DClass(&g_Hook))
		MessageBoxA(0, "Init成功", 0, 0);

	if (g_Hook.InitializeAndModifyAddress(d3dhook::D3dClass::Class_IDirect3DDevice9, d3dhook::Direct3DDevice_Function::F_Present, reinterpret_cast<int>(MyPresent)))
		MessageBoxA(0, "MyPresent成功", 0, 0);

	if (g_Hook.InitializeAndModifyAddress(d3dhook::D3dClass::Class_IDirect3DDevice9, d3dhook::Direct3DDevice_Function::F_Reset,
		reinterpret_cast<int>(MyReset)))
		MessageBoxA(0, "MyReset成功", 0, 0);

}

void CreateDebugConsole()
{
	AllocConsole();
	SetConsoleTitleA("Test Log");
	freopen("CON", "w", stdout);
}

BOOL WINAPI DllMain(
	_In_ void* _DllHandle,
	_In_ unsigned long _Reason,
	_In_opt_ void* _Reserved)
{
	if (_Reason == DLL_PROCESS_ATTACH)
	{
		DisableThreadLibraryCalls(static_cast<HMODULE>(_DllHandle));
		CreateDebugConsole();
		_beginthread(InitializeHook, 0, 0);
	}
	if (_Reason == DLL_PROCESS_DETACH)
	{
		g_Hook.ReduceAllAddress();
	}
	return TRUE;
}