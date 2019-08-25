#pragma once

#include <iostream>

#include <Windows.h>

constexpr int ShellCodeLen = 5;
constexpr char Asmjmp = '\xe9';
class InlineHook
{
private:
	unsigned char m_pOldAddress[ShellCodeLen];
	unsigned char m_pNewAddress[ShellCodeLen];

	int m_nOldAddress;
	int m_nNewAddress;

public:
	InlineHook();
	~InlineHook();

	///Note ��ʼ��Hook����
	///nOldAddress ������ҪHookס�ĺ�����ַ
	///nNewAddress �����Լ�����ĺ�����ַ
	///ִ�гɹ�����true�����򷵻�false
	bool Initialize(int nOldAddress,int nNewAddress);

	///Note �޸ĺ�����ַʵ��Hook����
	///ִ�гɹ�����true��ʧ�ܷ���false
	bool ModifyAddress();

	///Note ��ԭ������ַ
	///ִ�гɹ�����true�����򷵻�false
	bool ReduceAddress();

};

