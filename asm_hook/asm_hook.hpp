#pragma once

/*
fun : Hook指定地址的汇编代码
time : 2020.9.15
by : fyh
bus : 不支持Hook跳转类型的汇编代码 如jmp jz call... 因为没有处理跳转的相对地址
*/

/*将这些宏定义复制到预处理器中
#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NONSTDC_NO_DEPRECATE
#define STRICT
#define MAINPROG                       // Place all unique variables here
*/

#pragma warning(disable : 4005 4244 4838)
#include <Windows.h>
#include "disasm.h"

class asm_hook
{
private:
	DWORD64 m_original;		// 原始地址
	DWORD64 m_customize;	// 自定义地址

	DWORD64 m_asm_length;		// 原始地址的指令长度

	unsigned char m_original_byte[5];		// 原始地址的字节数据

	LPVOID m_memory;		// 原始字节 + 跳转到原始地址

	CONTEXT m_context;		//寄存器信息

	/* 检测错误 */
	void error(bool state, const char* text = nullptr)
	{
		if (state == false)
		{
			Beep(500, 500);
			MessageBoxA(0, text, "错误", MB_OK | MB_ICONERROR);
			exit(-1);
		}
	}

	/* 获取指令长度,必须 >= 5 */
	DWORD64 get_asm_length(DWORD64 original)
	{
		DWORD64 length = 0;

		while (length < 5)
		{
			unsigned char* point = (unsigned char*)original + length;

			struct t_disasm result { 0 };
			length += Disasm(point, 20, 0x400000, &result, DISASM_FILE);
		}

		return length;
	}

	/* 更新地址内存访问属性 */
	void update_page(DWORD64 original, DWORD64 len, BOOL backup = FALSE)
	{
		// 保存上一个页面修改前的访问属性
		static DWORD last_page = 0;

		BOOL State = FALSE;
		if (backup) State = VirtualProtect((LPVOID)original, len, last_page, &last_page);
		else State = VirtualProtect((LPVOID)original, len, PAGE_EXECUTE_READWRITE, &last_page);
		error(State, "修改内存属性失败");
	}

public:
	/* 构造函数开始hook */
	asm_hook(DWORD64 original, DWORD64 customize)
		: m_original(original), m_customize(customize)
	{
		// 获取指令长度
		m_asm_length = get_asm_length(original);

		// 计算原始地址到自定义地址的偏移
		DWORD64 offset = customize > (original + 5)
			? customize - (original + 5)
			: (original + 5) - customize;

		// 构造跳转到自定义地址的jmp
		unsigned char customize_byte[5]{ '\xe9' };
		memcpy(&customize_byte[1], &offset, 4);

		// 保存原始地址的字节数据
		memcpy(m_original_byte, (void*)original, 5);

		// 申请内存空间
		m_memory = VirtualAlloc(NULL, m_asm_length + 5, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
		error(m_memory, "申请内存控件失败");

		// 计算偏移
		offset = (original + m_asm_length) > ((int)m_memory + m_asm_length + 5)
			? ((int)m_memory + m_asm_length + 5) - (original + m_asm_length)
			: (original + m_asm_length) - ((int)m_memory + m_asm_length + 5);

		// 构造字节
		memcpy(m_memory, (void*)original, m_asm_length);
		reinterpret_cast<char*>(m_memory)[m_asm_length] = '\xe9';
		memcpy(reinterpret_cast<void*>((int)m_memory + m_asm_length + 1), &offset, 4);

		// 修改内存访问属性
		update_page(original, 5);

		// hook
		memcpy((void*)original, customize_byte, 5);

		// 修改内存访问属性
		update_page(original, 5, TRUE);
	}

	/* 执行原始代码 */
	void jmp_original()
	{
		LPVOID addr = m_memory;
		_asm jmp addr
	}

	/* 解除hook */
	void backup()
	{
		// 修改内存访问属性
		update_page(m_original, 5);

		// 解除hook
		memcpy((void*)m_original, m_original_byte, 5);

		// 修改内存访问属性
		update_page(m_original, 5, TRUE);

		// 释放内存
		VirtualFree(m_memory, m_asm_length + 5, MEM_RELEASE);
	}

	/* 开始处理 */
	void begin()
	{
		error(GetThreadContext(GetCurrentThread(), &m_context), "获取寄存器信息失败");
	}

	/* 获取寄存器结构指针 */
	inline CONTEXT* context() { return &m_context; }

	/* 结束处理 */
	void end()
	{
		error(SetThreadContext(GetCurrentThread(), &m_context), "设置寄存器信息失败");
	}
};