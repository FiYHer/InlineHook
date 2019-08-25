#include "InlineHook.h"

InlineHook::InlineHook():m_nOldAddress(0),m_nNewAddress(0)
{
	memset(m_pOldAddress, 0, ShellCodeLen);
	memset(m_pNewAddress, 0, ShellCodeLen);

	m_pNewAddress[0] = Asmjmp;
}

InlineHook::~InlineHook()
{

}

bool InlineHook::Initialize(int nOldAddress, int nNewAddress)
{
	try
	{
		//构造跳转指令
		int nAddressOffset = nNewAddress - (nOldAddress + ShellCodeLen);
		memcpy(&m_pNewAddress[1], &nAddressOffset, ShellCodeLen - 1);

		DWORD dwOldProtect;
		if (VirtualProtect(reinterpret_cast<void*>(nOldAddress), ShellCodeLen, PAGE_EXECUTE_READWRITE, &dwOldProtect) == FALSE)
			throw std::runtime_error("VirtualProtect fail");

		//保持原始地址
		memcpy(m_pOldAddress, reinterpret_cast<void*>(nOldAddress), ShellCodeLen);

		if (VirtualProtect(reinterpret_cast<void*>(nOldAddress), ShellCodeLen, dwOldProtect, &dwOldProtect) == FALSE)
			throw std::runtime_error("VirtualProtect fail");

		m_nOldAddress = nOldAddress;
		m_nNewAddress = nNewAddress;
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
		DWORD dwOldProtect;
		if (VirtualProtect(reinterpret_cast<void*>(m_nOldAddress), ShellCodeLen, PAGE_EXECUTE_READWRITE, &dwOldProtect) == FALSE)
			throw std::runtime_error("VirtualProtect fail");

		memcpy(reinterpret_cast<void*>(m_nOldAddress), m_pNewAddress, ShellCodeLen);

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
		DWORD dwOldProtect;
		if (VirtualProtect(reinterpret_cast<void*>(m_nOldAddress), ShellCodeLen, PAGE_EXECUTE_READWRITE, &dwOldProtect) == FALSE)
			throw std::runtime_error("VirtualProtect fail");

		memcpy(reinterpret_cast<void*>(m_nOldAddress), m_pOldAddress, ShellCodeLen);

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
