#pragma once
#include <iostream>
#include <Windows.h>

constexpr int ShellCodeLen = 5;			///��תָ�� + ������ַ
constexpr char Asmjmp = '\xe9';		///jmp

class InlineHook
{
private:
	unsigned char m_pOldAddress[ShellCodeLen];			///ԭʼ��ַ
	unsigned char m_pNewAddress[ShellCodeLen];		///�µĵ�ַ

	int m_nOldAddress;			///ԭʼ��ַ
	int m_nNewAddress;		///�µĵ�ַ

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

