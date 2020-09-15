#pragma once

/*
fun : Hookָ����ַ�Ļ�����
time : 2020.9.15
by : fyh
*/

/*����Щ�궨�帴�Ƶ�Ԥ��������
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
	DWORD64 m_original;		// ԭʼ��ַ
	DWORD64 m_customize;	// �Զ����ַ

	DWORD64 m_asm_length;		// ԭʼ��ַ��ָ���

	unsigned char m_original_byte[5];		// ԭʼ��ַ���ֽ�����

	LPVOID m_memory;		// ԭʼ�ֽ� + ��ת��ԭʼ��ַ

	/* ��ȡָ���,���� >= 5 */
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

	/* ���µ�ַ�ڴ�������� */
	void update_page(DWORD64 original, DWORD64 len, BOOL backup = FALSE)
	{
		// ������һ��ҳ���޸�ǰ�ķ�������
		static DWORD last_page = 0;

		if (backup) VirtualProtect((LPVOID)original, len, last_page, &last_page);
		else  VirtualProtect((LPVOID)original, len, PAGE_EXECUTE_READWRITE, &last_page);
	}

public:
	/* ���캯����ʼhook */
	asm_hook(DWORD64 original, DWORD64 customize)
		: m_original(original), m_customize(customize)
	{
		// ��ȡָ���
		m_asm_length = get_asm_length(original);

		// ����ԭʼ��ַ���Զ����ַ��ƫ��
		DWORD64 offset = customize > (original + 5)
			? customize - (original + 5)
			: (original + 5) - customize;

		// ������ת���Զ����ַ��jmp
		unsigned char customize_byte[5]{ '\xe9' };
		memcpy(&customize_byte[1], &offset, 4);

		// ����ԭʼ��ַ���ֽ�����
		memcpy(m_original_byte, (void*)original, 5);

		// �����ڴ�ռ�
		m_memory = VirtualAlloc(NULL, m_asm_length + 5, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);

		// ����ƫ��
		offset = (original + m_asm_length) > ((int)m_memory + m_asm_length + 5)
			? ((int)m_memory + m_asm_length + 5) - (original + m_asm_length)
			: (original + m_asm_length) - ((int)m_memory + m_asm_length + 5);

		// �����ֽ�
		memcpy(m_memory, (void*)original, m_asm_length);
		reinterpret_cast<char*>(m_memory)[m_asm_length] = '\xe9';
		memcpy(reinterpret_cast<void*>((int)m_memory + m_asm_length + 1), &offset, 4);

		// �޸��ڴ��������
		update_page(original, 5);

		// hook
		memcpy((void*)original, customize_byte, 5);

		// �޸��ڴ��������
		update_page(original, 5, TRUE);
	}

	/* ��ȡ��ת��ַ */
	inline LPVOID get_jmp_address() { return m_memory; }

	/* ���hook */
	void backup()
	{
		// �޸��ڴ��������
		update_page(m_original, 5);

		// ���hook
		memcpy((void*)m_original, m_original_byte, 5);

		// �޸��ڴ��������
		update_page(m_original, 5, TRUE);

		// �ͷ��ڴ�
		VirtualFree(m_memory, m_asm_length + 5, MEM_RELEASE);
	}
};