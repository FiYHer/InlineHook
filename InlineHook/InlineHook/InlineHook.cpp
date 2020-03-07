#include "InlineHook.h"

InlineHook::InlineHook():m_nOldAddress(0),m_nNewAddress(0)
{
	memset(m_pOldAddress, 0, ShellCodeLen);
	memset(m_pNewAddress, 0, ShellCodeLen);
	m_pNewAddress[0] = Asmjmp;		///����ָ����ȥ�µĺ�����ַ
}

InlineHook::~InlineHook(){}

bool InlineHook::Initialize(int nOldAddress, int nNewAddress)
{
	try
	{
		///�����µĺ�����ַ
		int nAddressOffset = nNewAddress - (nOldAddress + ShellCodeLen);
		memcpy(&m_pNewAddress[1], &nAddressOffset, ShellCodeLen - 1);

		DWORD dwOldProtect;		///��ԭʼ������ַ���ڴ�ģʽ�޸�Ϊ�ɶ���д��ִ��
		if (VirtualProtect(reinterpret_cast<void*>(nOldAddress), ShellCodeLen, PAGE_EXECUTE_READWRITE, &dwOldProtect) == FALSE)
			throw std::runtime_error("VirtualProtect fail");

		///����ԭʼ��ַ
		memcpy(m_pOldAddress, reinterpret_cast<void*>(nOldAddress), ShellCodeLen);

		///��ԭʼ������ַ���ڴ�ģʽ��ԭ
		if (VirtualProtect(reinterpret_cast<void*>(nOldAddress), ShellCodeLen, dwOldProtect, &dwOldProtect) == FALSE)
			throw std::runtime_error("VirtualProtect fail");

		m_nOldAddress = nOldAddress;			///����ԭʼ������ַ
		m_nNewAddress = nNewAddress;		///�����µĺ�����ַ
	}
	catch (const std::exception& e)
	{
		std::cout << e.what() << std::endl;
		return false;
	}
	return true;
}

bool InlineHook::ModifyAddress()
{
	try
	{
		DWORD dwOldProtect;		///��ԭʼ������ַ���ڴ�ģʽ�޸�Ϊ�ɶ���д��ִ��
		if (VirtualProtect(reinterpret_cast<void*>(m_nOldAddress), ShellCodeLen, PAGE_EXECUTE_READWRITE, &dwOldProtect) == FALSE)
			throw std::runtime_error("VirtualProtect fail");

		///���º�����ַ���ֽ����ݸ��Ƶ�ԭʼ������ַ��ʵ��inline hook����
		memcpy(reinterpret_cast<void*>(m_nOldAddress), m_pNewAddress, ShellCodeLen);

		///��ԭʼ������ַ���ڴ�ģʽ��ԭ
		if (VirtualProtect(reinterpret_cast<void*>(m_nOldAddress), ShellCodeLen, dwOldProtect, &dwOldProtect) == FALSE)
			throw std::runtime_error("VirtualProtect fail");
	}
	catch (const std::exception& e)
	{
		std::cout << e.what() << std::endl;
		return false;
	}
	return true;
}

bool InlineHook::ReduceAddress()
{
	try
	{
		DWORD dwOldProtect;		///��ԭʼ������ַ���ڴ�ģʽ�޸�Ϊ�ɶ���д��ִ��
		if (VirtualProtect(reinterpret_cast<void*>(m_nOldAddress), ShellCodeLen, PAGE_EXECUTE_READWRITE, &dwOldProtect) == FALSE)
			throw std::runtime_error("VirtualProtect fail");

		///��ԭʼ�������ֽ����ݻ�ԭ��ʵ��ȥinline hook
		memcpy(reinterpret_cast<void*>(m_nOldAddress), m_pOldAddress, ShellCodeLen);

		///��ԭʼ������ַ���ڴ�ģʽ��ԭ
		if (VirtualProtect(reinterpret_cast<void*>(m_nOldAddress), ShellCodeLen, dwOldProtect, &dwOldProtect) == FALSE)
			throw std::runtime_error("VirtualProtect fail");
	}
	catch (const std::exception& e)
	{
		std::cout << e.what() << std::endl;
		return false;
	}
	return true;
}
