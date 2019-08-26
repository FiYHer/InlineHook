#pragma once

#include <iostream>
#include <map>

#include <d3d9.h>
#pragma comment(lib,"d3d9.lib")

#include "InlineHook.h"

namespace d3dhook
{
	enum D3dClass
	{
		Class_IDirect3D9,			//��ʾIDirect3D9��
		Class_IDirect3DDevice9		//��ʾIDirect3DDevice9��
	};

	enum Direct3D_Function				//IDirect3D9��ĺ�������
	{
		f_RegisterSoftwareDevice = 3,
		f_GetAdapterCount,
		f_GetAdapterIdentifier,
		f_GetAdapterModeCount,
		f_EnumAdapterModes,
		f_GetAdapterDisplayMode,
		f_CheckDeviceType,
		f_CheckDeviceFormat,
		f_CheckDeviceMultiSampleType,
		f_CheckDepthStencilMatch,
		f_CheckDeviceFormatConversion,
		f_GetDeviceCaps,
		f_GetAdapterMonitor,
		f_CreateDevice
	};

	enum Direct3DDevice_Function		//IDirect3DDevice9��ĺ�������
	{
		F_TestCooperativeLevel = 3,
		F_GetAvailableTextureMem,
		F_EvictManagedResources,
		F_GetDirect3D,
		F_GetDeviceCaps,
		F_GetDisplayMode,
		F_GetCreationParameters,
		F_SetCursorProperties,
		F_SetCursorPosition,
		F_ShowCursor,
		F_CreateAdditionalSwapChain,
		F_GetSwapChain,
		F_GetNumberOfSwapChains,
		F_Reset,
		F_Present,
		F_GetBackBuffer,
		F_GetRasterStatus,
		F_SetDialogBoxMode,
		F_SetGammaRamp,
		F_GetGammaRamp,
		F_CreateTexture,
		F_CreateVolumeTexture,
		F_CreateCubeTexture,
		F_CreateVertexBuffer,
		F_CreateIndexBuffer,
		F_CreateRenderTarget,
		F_CreateDepthStencilSurface,
		F_UpdateSurface,
		F_UpdateTexture,
		F_GetRenderTargetData,
		F_GetFrontBufferData,
		F_StretchRect,
		F_ColorFill,
		F_CreateOffscreenPlainSurface,
		F_SetRenderTarget,
		F_GetRenderTarget,
		F_SetDepthStencilSurface,
		F_GetDepthStencilSurface,
		F_BeginScene,
		F_EndScene,
		F_Clear,
		F_SetTransform,
		F_GetTransform,
		F_MultiplyTransform,
		F_SetViewport,
		F_GetViewport,
		F_SetMaterial,
		F_GetMaterial,
		F_SetLight,
		F_GetLight,
		F_LightEnable,
		F_GetLightEnable,
		F_SetClipPlane,
		F_GetClipPlane,
		F_SetRenderState,
		F_GetRenderState,
		F_CreateStateBlock,
		F_BeginStateBlock,
		F_EndStateBlock,
		F_SetClipStatus,
		F_GetClipStatus,
		F_GetTexture,
		F_SetTexture,
		F_GetTextureStageState,
		F_SetTextureStageState,
		F_GetSamplerState,
		F_SetSamplerState,
		F_ValidateDevice,
		F_SetPaletteEntries,
		F_GetPaletteEntries,
		F_SetCurrentTexturePalette,
		F_GetCurrentTexturePalette,
		F_SetScissorRect,
		F_GetScissorRect,
		F_SetSoftwareVertexProcessing,
		F_GetSoftwareVertexProcessing,
		F_SetNPatchMode,
		F_GetNPatchMode,
		F_DrawPrimitive,
		F_DrawIndexedPrimitive,
		F_DrawPrimitiveUP,
		F_DrawIndexedPrimitiveUP,
		F_ProcessVertices,
		F_CreateVertexDeclaration,
		F_SetVertexDeclaration,
		F_GetVertexDeclaration,
		F_SetFVF,
		F_GetFVF,
		F_CreateVertexShader,
		F_SetVertexShader,
		F_GetVertexShader,
		F_SetVertexShaderConstantF,
		F_GetVertexShaderConstantF,
		F_SetVertexShaderConstantI,
		F_GetVertexShaderConstantI,
		F_SetVertexShaderConstantB,
		F_GetVertexShaderConstantB,
		F_SetStreamSource,
		F_GetStreamSource,
		F_SetStreamSourceFreq,
		F_GetStreamSourceFreq,
		F_SetIndices,
		F_GetIndices,
		F_CreatePixelShader,
		F_SetPixelShader,
		F_GetPixelShader,
		F_SetPixelShaderConstantF,
		F_GetPixelShaderConstantF,
		F_SetPixelShaderConstantI,
		F_GetPixelShaderConstantI,
		F_SetPixelShaderConstantB,
		F_GetPixelShaderConstantB,
		F_DrawRectPatch,
		F_DrawTriPatch,
		F_DeletePatch,
		F_CreateQuery,
	};

	class D3DHook
	{
	private:
		IDirect3D9* m_pOwnDirect3D;												//���Ǵ�����IDirect3D9
		IDirect3DDevice9* m_pOwnDirect3DDevice;									//���Ǵ�����IDirect3DDevice9

		int* m_pDirect3DTable;													//IDirect3D9��ĺ�������ָ��
		int* m_pDirect3DDeviceTable;											//IDirect3DDevice9��ĺ�������ָ��

		IDirect3D9* m_pGameDirect3D;											//ԭ����Ϸ�����IDirect3D9
		IDirect3DDevice9* m_pGameDirect3DDevice;								//ԭ����Ϸ�����IDirect3DDevice9

		std::map<Direct3D_Function, InlineHook> m_cDirect3DAddress;				//����Hookס��IDirect3D9����
		std::map<Direct3DDevice_Function, InlineHook> m_cDirect3DDeviceAddress;	//����Hookס��IDirect3DDevice9����

		bool m_bInitialize;														//��ʾ��ǰʵ���Ƿ��ʼ�����

	public:
		void SetOwnDirect3DPoint(IDirect3D9* pDirect3D);
		void SetOwnDirect3DDevicePoint(IDirect3DDevice9* pDirect3DDevice);
		inline void SetInitialize(bool bState) { m_bInitialize = bState; }

	public:
		void SetGameDirect3DPoint(IDirect3D9* pDirect3D);
		void SetGameDirect3DDevicePoint(IDirect3DDevice9* pDirect3DDevice);

	public:
		D3DHook();
		~D3DHook();

		///Note ��ʼ�����ҶԺ�������Hook����
		///eClass ָ��ҪHook�����D3D������ַ
		///nFunctionIndex ָ��ҪHook��һ��D3D����
		///nNewAddress �����Լ����º�����ַ
		///ִ�гɹ�����true�����򷵻�false
		bool InitializeAndModifyAddress(D3dClass eClass, int nFunctionIndex,int nNewAddress);

		///Note ��ָ����������Hook����
		///eClass ָ��ҪHook�����D3D������ַ
		///nFunctionIndex ָ��ҪHook��һ��D3D����
		///ִ�гɹ�����true�����򷵻�false
		bool ModifyAddress(D3dClass eClass,int nFunctionIndex);

		///Note ��ԭD3D�����ĵ�ַ
		///eClass ָ��Ҫ��ԭ�����D3D������ַ
		///nFunctionIndex ָ��Ҫ��ԭ��һ��D3D����
		///ִ�гɹ�����true�����򷵻�false
		bool ReduceAddress(D3dClass eClass, int nFunctionIndex);

		///Note ��ԭ���Ե�D3D�����ĵ�ַ
		///ִ�гɹ�����true�����򷵻�false
		bool ReduceAllAddress();
	};

	///Note �ศ��������Ĭ�ϴ��ڹ���
	LRESULT CALLBACK D3DHookProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	///Note �ศ����������ʼ��D3DHook��
	bool InitializeD3DClass(D3DHook* pThis);
}

