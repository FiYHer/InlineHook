// Free Disassembler and Assembler -- Disassembler
//
// Copyright (C) 2001 Oleh Yuschuk
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA


#define STRICT

#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
//#include <dir.h>
#include <math.h>
#include <float.h>
#pragma hdrstop

#include "disasm.h"

////////////////////////////////////////////////////////////////////////////////
//////////////////////////// DISASSEMBLER FUNCTIONS ////////////////////////////

// Work variables of disassembler����������Ĺ���������
static ulong     g_ulDataSize;         // Size of data (1,2,4 bytes)�����ݴ�С��
static ulong     g_ulAddrSize;         // Size of address (2 or 4 bytes)����ַ��С��
static int       g_nSegPrefix;         // Segment override prefix or SEG_UNDEF��������ǰ׺��
static int       g_nModRM;             // Command has ModR/M byte���ж��Ƿ���ModR/M��
static int       g_nSIB;               // Command has SIB byte���ж��Ƿ���SIB��
static int       g_nDispSize;          // Size of displacement (if any)��λ�ƴ�С��
static int       g_nImmSize;           // Size of immediate data (if any)����������С��
static int       g_nSoftError;         // Noncritical disassembler error���ǽ�����������
static int       g_nDumpLen;           // Current length of command dump����ǰת���ĳ������� max=255��
static int       g_nResultLen;         // Current length of disassembly����ǰ�����ĳ������� max=255��
static int       g_nAddComment;        // Comment value of operand����������ֵ��

// Copy of input parameters of function Disasm()��Disasm()�����Ĳ�����
static unsigned char      *g_uszCmd;            // Pointer to binary data��������ָ�롿
static unsigned char      *g_uszPfixUp;         // Pointer to possible fixups or NULL��ָ����ܵ�������ָ�롿
static ulong     g_ulHexLength;                 // Remaining size of the command buffer�����������ʣ���С��
static t_disasm  *g_stDisasmInfo;				// Pointer to disassembly results��ָ�򷴻��ṹ��
static int       g_nDisasmMode;                 // Disassembly mode (DISASM_xxx)�������ģʽ��

/* Disassemble name of 1, 2 or 4-byte general-purpose integer register and, if
 requested and available, dump its contents. Parameter type changes decoding
 of contents for some operand types.*/
 /*�������1��2��4�ֽ�ͨ�������Ĵ��������ƣ�������󲢿��ã�ת�������ݡ�
 �������͸��Ľ���ĳЩ���������͵����ݡ���*/
static void DecodeRG(int index, int datasize, int type)
{
	int sizeindex;
	char name[9];
	if (g_nDisasmMode < DISASM_DATA)
		return;        // No need to decode������Ҫ���롿
	index &= 0x07;
	if (datasize == 1)
		sizeindex = 0;
	else if (datasize == 2)
		sizeindex = 1;
	else if (datasize == 4)
		sizeindex = 2;
	else
	{
		g_stDisasmInfo->nError = DAE_INTERN;
		return;
	};
	if (g_nDisasmMode >= DISASM_FILE)
	{
		strcpy(name, cuszRegName[sizeindex][index]);
		if (g_nLowerCase)
			strlwr(name);
		if (type < PSEUDOOP)                 // Not a pseudooperand������α��������
			g_nResultLen += sprintf(g_stDisasmInfo->uszResult + g_nResultLen, "%s", name);
		;
	};
};

// Disassemble name of 80-bit floating-point register and, if available, dump
// its contents.
static void DecodeST(int index, int pseudoop) {
	int i;
	char s[32];
	if (g_nDisasmMode < DISASM_FILE) return;        // No need to decode
	index &= 0x07;
	i = sprintf(s, "%s(%i)", (g_nLowerCase ? "st" : "ST"), index);
	if (pseudoop == 0) {
		strcpy(g_stDisasmInfo->uszResult + g_nResultLen, s);
		g_nResultLen += i;
	};
};

// Disassemble name of 64-bit MMX register.
static void DecodeMX(int index) {
	char *pr;
	if (g_nDisasmMode < DISASM_FILE) return;        // No need to decode
	index &= 0x07;
	pr = g_stDisasmInfo->uszResult + g_nResultLen;
	g_nResultLen += sprintf(pr, "%s%i", (g_nLowerCase ? "mm" : "MM"), index);
};

// Disassemble name of 64-bit 3DNow! register and, if available, dump its
// contents.
static void DecodeNR(int index) {
	char *pr;
	if (g_nDisasmMode < DISASM_FILE) return;        // No need to decode
	index &= 0x07;
	pr = g_stDisasmInfo->uszResult + g_nResultLen;
	g_nResultLen += sprintf(pr, "%s%i", (g_nLowerCase ? "mm" : "MM"), index);
};

// Service function, adds valid memory adress in MASM or Ideal format to
// disassembled string. Parameters: defseg - default segment for given
// register combination, descr - fully decoded register part of address,
// offset - constant part of address, dsize - data size in bytes. If global
// flag 'symbolic' is set, function also tries to decode offset as name of
// some label.
static void Memadr(int defseg, const unsigned char *descr, long offset, int dsize) {
	int i, n, seg;
	unsigned char *pr;
	unsigned char s[TEXTLEN];
	if (g_nDisasmMode < DISASM_FILE || descr == NULL)
		return;                            // No need or possibility to decode
	pr = g_stDisasmInfo->uszResult + g_nResultLen; n = 0;
	if (g_nSegPrefix != SEG_UNDEF) seg = g_nSegPrefix; else seg = defseg;
	if (g_nIdeal != 0) pr[n++] = '[';
	// In some cases Disassembler may omit size of memory operand. Namely, flag
	// showmemsize must be 0, type bit C_EXPL must be 0 (this bit namely means
	// that explicit operand size is necessary) and type of command must not be
	// C_MMX or C_NOW (because bit C_EXPL has in these cases different meaning).
	// Otherwise, exact size must be supplied.
	if (g_nShowMemSize != 0 || (g_stDisasmInfo->nCmdType & C_TYPEMASK) == C_MMX ||
		(g_stDisasmInfo->nCmdType & C_TYPEMASK) == C_NOW || (g_stDisasmInfo->nCmdType & C_EXPL) != 0
		) {
		if (dsize < sizeof(cuszSizeName) / sizeof(cuszSizeName[0]))
			n += sprintf(pr + n, "%s %s", cuszSizeName[dsize], (g_nIdeal == 0 ? "PTR " : ""));
		else
			n += sprintf(pr + n, "(%i-BYTE) %s", dsize, (g_nIdeal == 0 ? "PTR " : ""));
		;
	};
	if ((g_nPutDefSeg != 0 || seg != defseg) && seg != SEG_UNDEF)
		n += sprintf(pr + n, "%s:", cuszSegName[seg]);
	if (g_nIdeal == 0) pr[n++] = '[';
	n += sprintf(pr + n, "%s", descr);
	if (g_nLowerCase) strlwr(pr);
	if (offset == 0L) {
		if (*descr == '\0') pr[n++] = '0';
	}
	else {
		if (g_nSymbolic && g_nDisasmMode >= DISASM_CODE)
			i = Decodeaddress(offset, s, TEXTLEN - n - 24, NULL);
		else i = 0;
		if (i > 0) {                         // Offset decoded in symbolic form
			if (*descr != '\0') pr[n++] = '+';
			strcpy(pr + n, s); n += i;
		}
		else if (offset<0 && offset>-16384 && *descr != '\0')
			n += sprintf(pr + n, "-%lX", -offset);
		else {
			if (*descr != '\0') pr[n++] = '+';
			n += sprintf(pr + n, "%lX", offset);
		};
	};
	pr[n++] = ']'; pr[n] = '\0';
	g_nResultLen += n;
};

// Disassemble memory/register from the ModRM/SIB bytes and, if available, dump address and contents of memory.
// ����modrm/sib�ֽڷ�����ڴ�/�Ĵ�����������ã�ת���ڴ�ĵ�ַ�����ݡ���
static void DecodeMR(int type)
{
	int j, memonly, inmemory, seg;
	int c, sib;
	ulong dsize, regsize, addr;
	unsigned char s[TEXTLEN];
	if (g_ulHexLength < 2)
	{
		g_stDisasmInfo->nError = DAE_CROSS; 
		return;
	};    // ModR/M byte outside the memory block
	g_nModRM = 1;
	dsize = regsize = g_ulDataSize;              // Default size of addressed reg/memory
	memonly = 0;                           // Register in ModM field is allowed
	// Size and kind of addressed memory or register in ModM has no influence on
	// the command size, and exact calculations are omitted if only command size
	// is requested. If register is used, optype will be incorrect and we need
	// to correct it later.
	c = g_uszCmd[1] & 0xC7;                     // Leave only Mod and M fields
	if (g_nDisasmMode >= DISASM_DATA)
	{
		if ((c & 0xC0) == 0xC0)              // Register operand
			inmemory = 0;
		else                               // Memory operand
			inmemory = 1;
		switch (type)
		{
		case MRG:                        // Memory/register in ModRM byte
			if (inmemory)
			{
				if (g_ulDataSize == 1) 
					g_stDisasmInfo->nMemType = DEC_BYTE;
				else if (g_ulDataSize == 2) 
					g_stDisasmInfo->nMemType = DEC_WORD;
				else g_stDisasmInfo->nMemType = DEC_DWORD;
			};
			break;
		case MRJ:                        // Memory/reg in ModRM as JUMP target
			if (g_ulDataSize != 2 && inmemory)
				g_stDisasmInfo->nMemType = DEC_DWORD;
			if (g_nDisasmMode >= DISASM_FILE && g_nShowNear != 0)
				g_nResultLen += sprintf(g_stDisasmInfo->uszResult + g_nResultLen, "%s ", (g_nLowerCase ? "near" : "NEAR"));
			break;
		case MR1:                        // 1-byte memory/register in ModRM byte
			dsize = regsize = 1;
			if (inmemory) g_stDisasmInfo->nMemType = DEC_BYTE; break;
		case MR2:                        // 2-byte memory/register in ModRM byte
			dsize = regsize = 2;
			if (inmemory) g_stDisasmInfo->nMemType = DEC_WORD; break;
		case MR4:                        // 4-byte memory/register in ModRM byte
		case RR4:                        // 4-byte memory/register (register only)
			dsize = regsize = 4;
			if (inmemory) g_stDisasmInfo->nMemType = DEC_DWORD; break;
		case MR8:                        // 8-byte memory/MMX register in ModRM
		case RR8:                        // 8-byte MMX register only in ModRM
			dsize = 8;
			if (inmemory) g_stDisasmInfo->nMemType = DEC_QWORD; break;
		case MRD:                        // 8-byte memory/3DNow! register in ModRM
		case RRD:                        // 8-byte memory/3DNow! (register only)
			dsize = 8;
			if (inmemory) g_stDisasmInfo->nMemType = DEC_3DNOW; break;
		case MMA:                        // Memory address in ModRM byte for LEA
			memonly = 1; break;
		case MML:                        // Memory in ModRM byte (for LES)
			dsize = g_ulDataSize + 2; memonly = 1;
			if (g_ulDataSize == 4 && inmemory)
				g_stDisasmInfo->nMemType = DEC_FWORD;
			g_stDisasmInfo->nWarnings |= DAW_SEGMENT;
			break;
		case MMS:                        // Memory in ModRM byte (as SEG:OFFS)
			dsize = g_ulDataSize + 2; memonly = 1;
			if (g_ulDataSize == 4 && inmemory)
				g_stDisasmInfo->nMemType = DEC_FWORD;
			if (g_nDisasmMode >= DISASM_FILE)
				g_nResultLen += sprintf(g_stDisasmInfo->uszResult + g_nResultLen, "%s ", (g_nLowerCase ? "far" : "FAR"));
			break;
		case MM6:                        // Memory in ModRM (6-byte descriptor)
			dsize = 6; memonly = 1;
			if (inmemory) g_stDisasmInfo->nMemType = DEC_FWORD; break;
		case MMB:                        // Two adjacent memory locations (BOUND)
			dsize = (g_nIdeal ? g_ulDataSize : g_ulDataSize * 2); memonly = 1; break;
		case MD2:                        // Memory in ModRM byte (16-bit integer)
		case MB2:                        // Memory in ModRM byte (16-bit binary)
			dsize = 2; memonly = 1;
			if (inmemory) g_stDisasmInfo->nMemType = DEC_WORD; break;
		case MD4:                        // Memory in ModRM byte (32-bit integer)
			dsize = 4; memonly = 1;
			if (inmemory) g_stDisasmInfo->nMemType = DEC_DWORD; break;
		case MD8:                        // Memory in ModRM byte (64-bit integer)
			dsize = 8; memonly = 1;
			if (inmemory) g_stDisasmInfo->nMemType = DEC_QWORD; break;
		case MDA:                        // Memory in ModRM byte (80-bit BCD)
			dsize = 10; memonly = 1;
			if (inmemory) g_stDisasmInfo->nMemType = DEC_TBYTE; break;
		case MF4:                        // Memory in ModRM byte (32-bit float)
			dsize = 4; memonly = 1;
			if (inmemory) g_stDisasmInfo->nMemType = DEC_FLOAT4; break;
		case MF8:                        // Memory in ModRM byte (64-bit float)
			dsize = 8; memonly = 1;
			if (inmemory) g_stDisasmInfo->nMemType = DEC_FLOAT8; break;
		case MFA:                        // Memory in ModRM byte (80-bit float)
			dsize = 10; memonly = 1;
			if (inmemory) g_stDisasmInfo->nMemType = DEC_FLOAT10; break;
		case MFE:                        // Memory in ModRM byte (FPU environment)
			dsize = 28; memonly = 1; break;
		case MFS:                        // Memory in ModRM byte (FPU state)
			dsize = 108; memonly = 1; break;
		case MFX:                        // Memory in ModRM byte (ext. FPU state)
			dsize = 512; memonly = 1; break;
		default:                         // Operand is not in ModM!
			g_stDisasmInfo->nError = DAE_INTERN;
			break;
		};
	};
	addr = 0;
	/* There are many possibilities to decode ModM/SIB address. The first
	 possibility is register in ModM - general-purpose, MMX or 3DNow!*/
	 /*������ModM/SIB��ַ�кܶ���ܡ���һ�ο�����ע����ModM - general-purpose, MMX or 3DNow!��*/
	if ((c & 0xC0) == 0xC0) // Decode register operand������Ĵ�����������
	{
		if (type == MR8 || type == RR8)
			DecodeMX(c);                     // MMX register
		else if (type == MRD || type == RRD)
			DecodeNR(c);                     // 3DNow! register
		else
			DecodeRG(c, regsize, type);        // General-purpose register
		if (memonly != 0)
			g_nSoftError = DAE_MEMORY;            // Register where only memory allowed
		return;
	};
	// Next possibility: 16-bit addressing mode, very seldom in 32-bit flat model
	// but still supported by processor. SIB byte is never used here.
	if (g_ulAddrSize == 2)
	{
		if (c == 0x06)
		{                     // Special case of immediate address
			g_nDispSize = 2;
			if (g_ulHexLength < 4)
				g_stDisasmInfo->nError = DAE_CROSS;           // Disp16 outside the memory block
			else if (g_nDisasmMode >= DISASM_DATA)
			{
				g_stDisasmInfo->ulAddrconst = addr = *(ushort *)(g_uszCmd + 2);
				if (addr == 0) g_stDisasmInfo->nZeroConst = 1;
				seg = SEG_DS;
				Memadr(seg, "", addr, dsize);
			};
		}
		else
		{
			g_stDisasmInfo->nIndexed = 1;
			if ((c & 0xC0) == 0x40)
			{          // 8-bit signed displacement
				if (g_ulHexLength < 3) g_stDisasmInfo->nError = DAE_CROSS;
				else addr = (signed char)g_uszCmd[2] & 0xFFFF;
				g_nDispSize = 1;
			}
			else if ((c & 0xC0) == 0x80)
			{     // 16-bit unsigned displacement
				if (g_ulHexLength < 4) g_stDisasmInfo->nError = DAE_CROSS;
				else addr = *(ushort *)(g_uszCmd + 2);
				g_nDispSize = 2;
			};
			if (g_nDisasmMode >= DISASM_DATA && g_stDisasmInfo->nError == DAE_NOERR) {
				g_stDisasmInfo->ulAddrconst = addr;
				if (addr == 0) g_stDisasmInfo->nZeroConst = 1;
				seg = cstAddr16[c & 0x07].nDefSeg;
				Memadr(seg, cstAddr16[c & 0x07].uszDescr, addr, dsize);
			};
		};
	}
	// Next possibility: immediate 32-bit address.
	else if (c == 0x05)
	{                  // Special case of immediate address
		g_nDispSize = 4;
		if (g_ulHexLength < 6)
			g_stDisasmInfo->nError = DAE_CROSS;             // Disp32 outside the memory block
		else if (g_nDisasmMode >= DISASM_DATA)
		{
			g_stDisasmInfo->ulAddrconst = addr = *(ulong *)(g_uszCmd + 2);
			if (g_uszPfixUp == NULL) g_uszPfixUp = g_uszCmd + 2;
			g_stDisasmInfo->nFixUpSize += 4;
			if (addr == 0) g_stDisasmInfo->nZeroConst = 1;
			seg = SEG_DS;
			Memadr(seg, "", addr, dsize);
		};
	}
	// Next possibility: 32-bit address with SIB byte.
	else if ((c & 0x07) == 0x04)
	{         // SIB addresation
		sib = g_uszCmd[2]; g_nSIB = 1;
		*s = '\0';
		if (c == 0x04 && (sib & 0x07) == 0x05)
		{
			g_nDispSize = 4;                      // Immediate address without base
			if (g_ulHexLength < 7)
				g_stDisasmInfo->nError = DAE_CROSS;           // Disp32 outside the memory block
			else {
				g_stDisasmInfo->ulAddrconst = addr = *(ulong *)(g_uszCmd + 3);
				if (g_uszPfixUp == NULL) g_uszPfixUp = g_uszCmd + 3;
				g_stDisasmInfo->nFixUpSize += 4;
				if (addr == 0) g_stDisasmInfo->nZeroConst = 1;
				if ((sib & 0x38) != 0x20)
				{      // Index register present
					g_stDisasmInfo->nIndexed = 1;
					if (type == MRJ) g_stDisasmInfo->ulJmpTable = addr;
				};
				seg = SEG_DS;
			};
		}
		else {                             // Base and, eventually, displacement
			if ((c & 0xC0) == 0x40)
			{          // 8-bit displacement
				g_nDispSize = 1;
				if (g_ulHexLength < 4) g_stDisasmInfo->nError = DAE_CROSS;
				else {
					g_stDisasmInfo->ulAddrconst = addr = (signed char)g_uszCmd[3];
					if (addr == 0) g_stDisasmInfo->nZeroConst = 1;
				};
			}
			else if ((c & 0xC0) == 0x80)
			{     // 32-bit displacement
				g_nDispSize = 4;
				if (g_ulHexLength < 7)
					g_stDisasmInfo->nError = DAE_CROSS;         // Disp32 outside the memory block
				else {
					g_stDisasmInfo->ulAddrconst = addr = *(ulong *)(g_uszCmd + 3);
					if (g_uszPfixUp == NULL) g_uszPfixUp = g_uszCmd + 3;
					g_stDisasmInfo->nFixUpSize += 4;
					if (addr == 0) g_stDisasmInfo->nZeroConst = 1;
					// Most compilers use address of type [index*4+displacement] to
					// address jump table (switch). But, for completeness, I allow all
					// cases which include index with scale 1 or 4, base or both.
					if (type == MRJ) g_stDisasmInfo->ulJmpTable = addr;
				};
			};
			g_stDisasmInfo->nIndexed = 1;
			j = sib & 0x07;
			if (g_nDisasmMode >= DISASM_FILE)
			{
				strcpy(s, cuszRegName[2][j]);
				seg = cstAddr32[j].nDefSeg;
			};
		};
		if ((sib & 0x38) != 0x20)
		{          // Scaled index present
			if ((sib & 0xC0) == 0x40) g_stDisasmInfo->nIndexed = 2;
			else if ((sib & 0xC0) == 0x80) g_stDisasmInfo->nIndexed = 4;
			else if ((sib & 0xC0) == 0xC0) g_stDisasmInfo->nIndexed = 8;
			else g_stDisasmInfo->nIndexed = 1;
		};
		if (g_nDisasmMode >= DISASM_FILE && g_stDisasmInfo->nError == DAE_NOERR)
		{
			if ((sib & 0x38) != 0x20)
			{        // Scaled index present
				if (*s != '\0') strcat(s, "+");
				strcat(s, cstAddr32[(sib >> 3) & 0x07].uszDescr);
				if ((sib & 0xC0) == 0x40)
				{
					g_stDisasmInfo->ulJmpTable = 0;              // Hardly a switch!
					strcat(s, "*2");
				}
				else if ((sib & 0xC0) == 0x80)
					strcat(s, "*4");
				else if ((sib & 0xC0) == 0xC0)
				{
					g_stDisasmInfo->ulJmpTable = 0;              // Hardly a switch!
					strcat(s, "*8");
				};
			};
			Memadr(seg, s, addr, dsize);
		};
	}
	// Last possibility: 32-bit address without SIB byte.
	else
	{                               // No SIB
		if ((c & 0xC0) == 0x40)
		{
			g_nDispSize = 1;
			if (g_ulHexLength < 3) g_stDisasmInfo->nError = DAE_CROSS; // Disp8 outside the memory block
			else
			{
				g_stDisasmInfo->ulAddrconst = addr = (signed char)g_uszCmd[2];
				if (addr == 0) g_stDisasmInfo->nZeroConst = 1;
			};
		}
		else if ((c & 0xC0) == 0x80)
		{
			g_nDispSize = 4;
			if (g_ulHexLength < 6)
				g_stDisasmInfo->nError = DAE_CROSS;           // Disp32 outside the memory block
			else {
				g_stDisasmInfo->ulAddrconst = addr = *(ulong *)(g_uszCmd + 2);
				if (g_uszPfixUp == NULL) g_uszPfixUp = g_uszCmd + 2;
				g_stDisasmInfo->nFixUpSize += 4;
				if (addr == 0) g_stDisasmInfo->nZeroConst = 1;
				if (type == MRJ) g_stDisasmInfo->ulJmpTable = addr;
			};
		};
		g_stDisasmInfo->nIndexed = 1;
		if (g_nDisasmMode >= DISASM_FILE && g_stDisasmInfo->nError == DAE_NOERR)
		{
			seg = cstAddr32[c & 0x07].nDefSeg;
			Memadr(seg, cstAddr32[c & 0x07].uszDescr, addr, dsize);
		};
	};
};

// Disassemble implicit source of string operations and, if available, dump
// address and contents.
static void DecodeSO(void) {
	if (g_nDisasmMode < DISASM_FILE) return;        // No need to decode
	if (g_ulDataSize == 1) g_stDisasmInfo->nMemType = DEC_BYTE;
	else if (g_ulDataSize == 2) g_stDisasmInfo->nMemType = DEC_WORD;
	else if (g_ulDataSize == 4) g_stDisasmInfo->nMemType = DEC_DWORD;
	g_stDisasmInfo->nIndexed = 1;
	Memadr(SEG_DS, cuszRegName[g_ulAddrSize == 2 ? 1 : 2][REG_ESI], 0L, g_ulDataSize);
};

// Disassemble implicit destination of string operations and, if available,
// dump address and contents. Destination always uses segment ES, and this
// setting cannot be overridden.
static void DecodeDE(void) {
	int seg;
	if (g_nDisasmMode < DISASM_FILE) return;        // No need to decode
	if (g_ulDataSize == 1) g_stDisasmInfo->nMemType = DEC_BYTE;
	else if (g_ulDataSize == 2) g_stDisasmInfo->nMemType = DEC_WORD;
	else if (g_ulDataSize == 4) g_stDisasmInfo->nMemType = DEC_DWORD;
	g_stDisasmInfo->nIndexed = 1;
	seg = g_nSegPrefix; g_nSegPrefix = SEG_ES;     // Fake Memadr by changing segment prefix
	Memadr(SEG_DS, cuszRegName[g_ulAddrSize == 2 ? 1 : 2][REG_EDI], 0L, g_ulDataSize);
	g_nSegPrefix = seg;                       // Restore segment prefix
};

// Decode XLAT operand and, if available, dump address and contents.
static void DecodeXL(void) {
	if (g_nDisasmMode < DISASM_FILE) return;        // No need to decode
	g_stDisasmInfo->nMemType = DEC_BYTE;
	g_stDisasmInfo->nIndexed = 1;
	Memadr(SEG_DS, (g_ulAddrSize == 2 ? "BX+AL" : "EBX+AL"), 0L, 1);
};

// Decode immediate operand of size constsize. If sxt is non-zero, byte operand
// should be sign-extended to sxt bytes. If type of immediate constant assumes
// this, small negative operands may be displayed as signed negative numbers.
// Note that in most cases immediate operands are not shown in comment window.
static void DecodeIM(int constsize, int sxt, int type) {
	int i;
	signed long data;
	ulong l;
	unsigned char name[TEXTLEN] = { 0 };
	unsigned char comment[TEXTLEN] = { 0 };
	g_nImmSize += constsize;                    // Allows several immediate operands
	if (g_nDisasmMode < DISASM_DATA) return;
	l = 1 + g_nModRM + g_nSIB + g_nDispSize + (g_nImmSize - constsize);
	data = 0;
	if (g_ulHexLength < l + constsize)
		g_stDisasmInfo->nError = DAE_CROSS;
	else if (constsize == 1) {
		if (sxt == 0) data = (uchar)g_uszCmd[l];
		else data = (signed char)g_uszCmd[l];
		if (type == IMS && ((data & 0xE0) != 0 || data == 0)) {
			g_stDisasmInfo->nWarnings |= DAW_SHIFT;
			g_stDisasmInfo->nCmdType |= C_RARE;
		};
	}
	else if (constsize == 2) {
		if (sxt == 0) data = *(ushort *)(g_uszCmd + l);
		else data = *(short *)(g_uszCmd + l);
	}
	else {
		data = *(long *)(g_uszCmd + l);
		if (g_uszPfixUp == NULL) g_uszPfixUp = g_uszCmd + l;
		g_stDisasmInfo->nFixUpSize += 4;
	};
	if (sxt == 2) data &= 0x0000FFFF;
	if (data == 0 && g_stDisasmInfo->nError == 0) g_stDisasmInfo->nZeroConst = 1;
	// Command ENTER, as an exception from Intel's rules, has two immediate
	// constants. As the second constant is rarely used, I exclude it from
	// search if the first constant is non-zero (which is usually the case).
	if (g_stDisasmInfo->ulImmConst == 0)
		g_stDisasmInfo->ulImmConst = data;
	if (g_nDisasmMode >= DISASM_FILE && g_stDisasmInfo->nError == DAE_NOERR) {
		if (g_nDisasmMode >= DISASM_CODE && type != IMU)
			i = Decodeaddress(data, name, TEXTLEN - g_nResultLen - 24, comment);
		else {
			i = 0; comment[0] = '\0';
		};
		if (i != 0 && g_nSymbolic != 0) {
			strcpy(g_stDisasmInfo->uszResult + g_nResultLen, name); g_nResultLen += i;
		}
		else if (type == IMU || type == IMS || type == IM2 || data >= 0 || data < NEGLIMIT)
			g_nResultLen += sprintf(g_stDisasmInfo->uszResult + g_nResultLen, "%lX", data);
		else
			g_nResultLen += sprintf(g_stDisasmInfo->uszResult + g_nResultLen, "-%lX", -data);
		if (g_nAddComment && comment[0] != '\0') strcpy(g_stDisasmInfo->uszComment, comment);
	};
};

// Decode VxD service name (always 4-byte).
static void DecodeVX(void) {
	ulong l, data;
	g_nImmSize += 4;                          // Allows several immediate operands
	if (g_nDisasmMode < DISASM_DATA) return;
	l = 1 + g_nModRM + g_nSIB + g_nDispSize + (g_nImmSize - 4);
	if (g_ulHexLength < l + 4) {
		g_stDisasmInfo->nError = DAE_CROSS;
		return;
	};
	data = *(long *)(g_uszCmd + l);
	if (data == 0 && g_stDisasmInfo->nError == 0) g_stDisasmInfo->nZeroConst = 1;
	if (g_stDisasmInfo->ulImmConst == 0)
		g_stDisasmInfo->ulImmConst = data;
	if (g_nDisasmMode >= DISASM_FILE && g_stDisasmInfo->nError == DAE_NOERR) {
		if ((data & 0x00008000) != 0 && memicmp("VxDCall", g_stDisasmInfo->uszResult, 7) == 0)
			memcpy(g_stDisasmInfo->uszResult, g_nLowerCase ? "vxdjump" : "VxDJump", 7);
		g_nResultLen += sprintf(g_stDisasmInfo->uszResult + g_nResultLen, "%lX", data);
	};
};

// Decode implicit constant 1 (used in shift commands). This operand is so
// insignificant that it is never shown in comment window.
static void DecodeC1(void) {
	if (g_nDisasmMode < DISASM_DATA) return;
	g_stDisasmInfo->ulImmConst = 1;
	if (g_nDisasmMode >= DISASM_FILE) g_nResultLen += sprintf(g_stDisasmInfo->uszResult + g_nResultLen, "1");
};

// Decode immediate absolute data address. This operand is used in 8080-
// compatible commands which allow to move data from memory to accumulator and
// back. Note that bytes ModRM and SIB never appear in commands with IA operand.
static void DecodeIA(void) {
	ulong addr;
	if (g_ulHexLength < 1 + g_ulAddrSize) {
		g_stDisasmInfo->nError = DAE_CROSS; return;
	};
	g_nDispSize = g_ulAddrSize;
	if (g_nDisasmMode < DISASM_DATA) return;
	if (g_ulDataSize == 1) g_stDisasmInfo->nMemType = DEC_BYTE;
	else if (g_ulDataSize == 2) g_stDisasmInfo->nMemType = DEC_WORD;
	else if (g_ulDataSize == 4) g_stDisasmInfo->nMemType = DEC_DWORD;
	if (g_ulAddrSize == 2)
		addr = *(ushort *)(g_uszCmd + 1);
	else {
		addr = *(ulong *)(g_uszCmd + 1);
		if (g_uszPfixUp == NULL) g_uszPfixUp = g_uszCmd + 1;
		g_stDisasmInfo->nFixUpSize += 4;
	};
	g_stDisasmInfo->ulAddrconst = addr;
	if (addr == 0) g_stDisasmInfo->nZeroConst = 1;
	if (g_nDisasmMode >= DISASM_FILE) {
		Memadr(SEG_DS, "", addr, g_ulDataSize);
	};
};

// Decodes jump relative to nextip of size offsize.
static void DecodeRJ(ulong offsize, ulong nextip) {
	int i;
	ulong addr;
	unsigned char s[TEXTLEN];
	if (g_ulHexLength < offsize + 1) {
		g_stDisasmInfo->nError = DAE_CROSS; return;
	};
	g_nDispSize = offsize;                    // Interpret offset as displacement
	if (g_nDisasmMode < DISASM_DATA) return;
	if (offsize == 1)
		addr = (signed char)g_uszCmd[1] + nextip;
	else if (offsize == 2)
		addr = *(signed short *)(g_uszCmd + 1) + nextip;
	else
		addr = *(ulong *)(g_uszCmd + 1) + nextip;
	if (g_ulDataSize == 2)
		addr &= 0xFFFF;
	g_stDisasmInfo->ulJmpConst = addr;
	if (addr == 0) g_stDisasmInfo->nZeroConst = 1;
	if (g_nDisasmMode >= DISASM_FILE) {
		if (offsize == 1) g_nResultLen += sprintf(g_stDisasmInfo->uszResult + g_nResultLen,
			"%s ", (g_nLowerCase == 0 ? "SHORT" : "short"));
		if (g_nDisasmMode >= DISASM_CODE)
			i = Decodeaddress(addr, s, TEXTLEN, g_stDisasmInfo->uszComment);
		else
			i = 0;
		if (g_nSymbolic == 0 || i == 0)
			g_nResultLen += sprintf(g_stDisasmInfo->uszResult + g_nResultLen, "%08lX", addr);
		else
			g_nResultLen += sprintf(g_stDisasmInfo->uszResult + g_nResultLen, "%.*s", TEXTLEN - g_nResultLen - 25, s);
		if (g_nSymbolic == 0 && i != 0 && g_stDisasmInfo->uszComment[0] == '\0')
			strcpy(g_stDisasmInfo->uszComment, s);
		;
	};
};

// Decode immediate absolute far jump address. In flat model, such addresses
// are not used (mostly because selector is specified directly in the command),
// so I neither decode as symbol nor comment it. To allow search for selector
// by value, I interprete it as an immediate constant.
static void DecodeJF(void) {
	ulong addr, seg;
	if (g_ulHexLength < 1 + g_ulAddrSize + 2) {
		g_stDisasmInfo->nError = DAE_CROSS; return;
	};
	g_nDispSize = g_ulAddrSize; g_nImmSize = 2;        // Non-trivial but allowed interpretation
	if (g_nDisasmMode < DISASM_DATA) return;
	if (g_ulAddrSize == 2) {
		addr = *(ushort *)(g_uszCmd + 1);
		seg = *(ushort *)(g_uszCmd + 3);
	}
	else {
		addr = *(ulong *)(g_uszCmd + 1);
		seg = *(ushort *)(g_uszCmd + 5);
	};
	g_stDisasmInfo->ulJmpConst = addr;
	g_stDisasmInfo->ulImmConst = seg;
	if (addr == 0 || seg == 0) g_stDisasmInfo->nZeroConst = 1;
	if (g_nDisasmMode >= DISASM_FILE) {
		g_nResultLen += sprintf(g_stDisasmInfo->uszResult + g_nResultLen, "%s %04X:%08X",
			(g_nLowerCase == 0 ? "FAR" : "far"), seg, addr);
	};
};

// Decode segment register. In flat model, operands of this type are seldom.
static void DecodeSG(int index) {
	int i;
	if (g_nDisasmMode < DISASM_DATA) return;
	index &= 0x07;
	if (index >= 6) g_nSoftError = DAE_BADSEG;  // Undefined segment register
	if (g_nDisasmMode >= DISASM_FILE) {
		i = sprintf(g_stDisasmInfo->uszResult + g_nResultLen, "%s", cuszSegName[index]);
		if (g_nLowerCase) strlwr(g_stDisasmInfo->uszResult + g_nResultLen);
		g_nResultLen += i;
	};
};

// Decode control register addressed in R part of ModRM byte. Operands of
// this type are extremely rare. Contents of control registers are accessible
// only from privilege level 0, so I cannot dump them here.
static void DecodeCR(int index) {
	g_nModRM = 1;
	if (g_nDisasmMode >= DISASM_FILE) {
		index = (index >> 3) & 0x07;
		g_nResultLen += sprintf(g_stDisasmInfo->uszResult + g_nResultLen, "%s", cuszCRName[index]);
		if (g_nLowerCase) strlwr(g_stDisasmInfo->uszResult + g_nResultLen);
	};
};

// Decode debug register addressed in R part of ModRM byte. Operands of
// this type are extremely rare. I can dump only those debug registers
// available in CONTEXT structure.
static void DecodeDR(int index) {
	int i;
	g_nModRM = 1;
	if (g_nDisasmMode >= DISASM_FILE) {
		index = (index >> 3) & 0x07;
		i = sprintf(g_stDisasmInfo->uszResult + g_nResultLen, "%s", cuszDRName[index]);
		if (g_nLowerCase) strlwr(g_stDisasmInfo->uszResult + g_nResultLen);
		g_nResultLen += i;
	};
};

/* Skips 3DNow! operands and extracts command suffix. Returns suffix or -1 if
   suffix lies outside the memory block. This subroutine assumes that cmd still
   points to the beginning of 3DNow! command (i.e. to the sequence of two bytes
   0F, 0F).*/
/*������3dnow������������ȡ�����׺��*/
static int Get3dnowsuffix(void) 
{
	int c, sib;
	ulong offset;
	if (g_ulHexLength < 3) return -1;               // Suffix outside the memory block
	offset = 3;
	c = g_uszCmd[2] & 0xC7;                     // Leave only Mod and M fields
	// Register in ModM - general-purpose, MMX or 3DNow!
	if ((c & 0xC0) == 0xC0)
		;
	// 16-bit addressing mode, SIB byte is never used here.
	else if (g_ulAddrSize == 2) {
		if (c == 0x06)                       // Special case of immediate address
			offset += 2;
		else if ((c & 0xC0) == 0x40)         // 8-bit signed displacement
			offset++;
		else if ((c & 0xC0) == 0x80)         // 16-bit unsigned displacement
			offset += 2;
		;
	}
	// Immediate 32-bit address.
	else if (c == 0x05)                    // Special case of immediate address
		offset += 4;
	// 32-bit address with SIB byte.
	else if ((c & 0x07) == 0x04) {         // SIB addresation
		if (g_ulHexLength < 4) return -1;             // Suffix outside the memory block
		sib = g_uszCmd[3]; offset++;
		if (c == 0x04 && (sib & 0x07) == 0x05)
			offset += 4;                       // Immediate address without base
		else if ((c & 0xC0) == 0x40)         // 8-bit displacement
			offset += 1;
		else if ((c & 0xC0) == 0x80)         // 32-bit dislacement
			offset += 4;
		;
	}
	// 32-bit address without SIB byte
	else if ((c & 0xC0) == 0x40)
		offset += 1;
	else if ((c & 0xC0) == 0x80)
		offset += 4;
	if (offset >= g_ulHexLength) return -1;         // Suffix outside the memory block
	return g_uszCmd[offset];
};

// Function checks whether 80x86 flags meet condition set in the command.
// Returns 1 if condition is met, 0 if not and -1 in case of error (which is
// not possible).
int Checkcondition(int code, ulong flags) {
	ulong cond, temp;
	switch (code & 0x0E) {
	case 0:                            // If overflow
		cond = flags & 0x0800; break;
	case 2:                            // If below
		cond = flags & 0x0001; break;
	case 4:                            // If equal
		cond = flags & 0x0040; break;
	case 6:                            // If below or equal
		cond = flags & 0x0041; break;
	case 8:                            // If sign
		cond = flags & 0x0080; break;
	case 10:                           // If parity
		cond = flags & 0x0004; break;
	case 12:                           // If less
		temp = flags & 0x0880;
		cond = (temp == 0x0800 || temp == 0x0080); break;
	case 14:                           // If less or equal
		temp = flags & 0x0880;
		cond = (temp == 0x0800 || temp == 0x0080 || (flags & 0x0040) != 0); break;
	default: return -1;                // Internal error, not possible!
	};
	if ((code & 0x01) == 0) return (cond != 0);
	else return (cond == 0);               // Invert condition
};

ulong Disasm(unsigned char *szHex,		//��Ҫ����ɻ������ʮ�������ı�
	ulong ulHexLen,						//ʮ�������ı��ĳ���
	ulong ulHexAddress,					//ָ���ڳ����еĵĵ�ַ,��Ե�ַ��Ҫ���
	t_disasm *pDisasmInfo,				//���ջ��ָ��Ϣ
	int nDisasmMode)					//�����ģʽ
{
	int i;								//ѭ������
	int j;								//ѭ������
	int nIsPrefix;						//�ж��Ƿ���ָ��ǰ׺
	int nIs3DNow;						//�ж��Ƿ���3DNowָ��
	int nIsRepeated;					//�ж��Ƿ����ظ�ǰ׺
	int nOperandCount;					//������������
	int nMnemoSize;						//������/��ַ��С byte word dword
	int nCurrentArg;					//��ǰ��arguments
	ulong ulPrefixCounts;				//ǰ׺������
	ulong ulCode;
	int nLockPrefix;                    // Non-zero if lock prefix present������ǰ׺��
	int nRepPrefix;                     // REPxxx prefix or 0���ظ�ǰ׺��
	int nEcxSize;						// �Ĵ���Ecx�ĳ���
	unsigned char name[TEXTLEN];
	unsigned char* pName;				// �ַ���ָ��
	const t_cmddata *pd;
	const t_cmddata *pdan;
	// Prepare disassembler variables and initialize structure disasm.��׼��������������ʼ���ṹdisasm��
	g_ulDataSize = 4;					// 32-bit code and data segments only!������32λ��������ݶΣ���
	g_ulAddrSize = 4;					// 32-bit code and data segments only!������32λ��������ݶΣ���
	g_nSegPrefix = SEG_UNDEF;			// δ֪������ǰ׺
	g_nModRM = 0;
	g_nSIB = 0;
	g_nDispSize = 0;
	g_nImmSize = 0;
	nLockPrefix = 0;
	nRepPrefix = 0;
	g_nDumpLen = 0;
	g_nResultLen = 0;
	g_uszCmd = szHex;
	g_ulHexLength = ulHexLen;
	g_uszPfixUp = NULL;
	g_nSoftError = 0;
	nIs3DNow = 0;
	g_stDisasmInfo = pDisasmInfo;
	g_stDisasmInfo->ulIp = ulHexAddress;
	g_stDisasmInfo->uszComment[0] = '\0';				//�������
	g_stDisasmInfo->nCmdType = C_BAD;					//�޷�ʶ�������
	g_stDisasmInfo->nPrefix = 0;						//ǰ׺������
	g_stDisasmInfo->nMemType = DEC_UNKNOWN;				//δ֪����
	g_stDisasmInfo->nIndexed = 0;						//��ַ�����ļĴ���
	g_stDisasmInfo->ulJmpConst = 0;
	g_stDisasmInfo->ulJmpTable = 0;
	g_stDisasmInfo->ulAddrconst = 0;
	g_stDisasmInfo->ulImmConst = 0;
	g_stDisasmInfo->nZeroConst = 0;
	g_stDisasmInfo->nFixUpOffset = 0;
	g_stDisasmInfo->nFixUpSize = 0;
	g_stDisasmInfo->nWarnings = 0;						// ����
	g_stDisasmInfo->nError = DAE_NOERR;					// û�з�������
	g_nDisasmMode = nDisasmMode;				// No need to use register contents������ʹ�üĴ������ݡ�
	/* Correct 80x86 command may theoretically contain up to 4 prefixes belonging
	 to different prefix groups. This limits maximal possible size of the
	 command to MAXCMDSIZE=16 bytes. In order to maintain this limit, if
	 Disasm() detects second prefix from the same group, it flushes first
	 prefix in the sequence as a pseudocommand.*/
	 /*��ȷ��80x86���������������԰���4�����ڵ���ͬ��ǰ׺�顣�����������16�ֽڡ�
	 Ϊ��ά����һ���ƣ����disasm�������ͬһ���еĵڶ���ǰ׺��������ˢ����Ϊα���������ǰ׺*/
	ulPrefixCounts = 0;						// ǰ׺������Ϊ0
	nIsRepeated = 0;						// û���ظ�ǰ׺
	while (g_ulHexLength > 0)				// ���Ȳ�Ϊ��
	{
		nIsPrefix = 1;						// Assume that there is some prefix��������һ��ǰ׺��
		switch (*g_uszCmd)					// A-2���ֽڱ���жϣ����ж϶�ǰ׺
		{
		case 0x26:
			if (g_nSegPrefix == SEG_UNDEF)	// ������ǵ�һ��ǰ׺
				g_nSegPrefix = SEG_ES;		// ����ǰ׺
			else
				nIsRepeated = 1;			// ǰ׺����������1
			break;
		case 0x2E:
			if (g_nSegPrefix == SEG_UNDEF)
				g_nSegPrefix = SEG_CS;
			else
				nIsRepeated = 1;
			break;
		case 0x36:
			if (g_nSegPrefix == SEG_UNDEF)
				g_nSegPrefix = SEG_SS;
			else
				nIsRepeated = 1;
			break;
		case 0x3E:
			if (g_nSegPrefix == SEG_UNDEF)
				g_nSegPrefix = SEG_DS;
			else
				nIsRepeated = 1;
			break;
		case 0x64:
			if (g_nSegPrefix == SEG_UNDEF)
				g_nSegPrefix = SEG_FS;
			else
				nIsRepeated = 1;
			break;
		case 0x65:
			if (g_nSegPrefix == SEG_UNDEF)
				g_nSegPrefix = SEG_GS;
			else
				nIsRepeated = 1;
			break;
		case 0x66://Operand Size Prefix����������С����ǰ׺��
			if (g_ulDataSize == 4)
				g_ulDataSize = 2;		
			else
				nIsRepeated = 1;
			break;
		case 0x67://Address Size Prefix����ַ��С����ǰ׺��
			if (g_ulAddrSize == 4)
				g_ulAddrSize = 2;	
			else
				nIsRepeated = 1;
			break;
		case 0xF0://Lock������ǰ׺��
			if (nLockPrefix == 0)
				nLockPrefix = 0xF0;	
			else
				nIsRepeated = 1;
			break;
		case 0xF2://Repne���ظ�ǰ׺��
			if (nRepPrefix == 0)
				nRepPrefix = 0xF2;
			else
				nIsRepeated = 1;
			break;
		case 0xF3://Rep���ظ�ǰ׺��
			if (nRepPrefix == 0)
				nRepPrefix = 0xF3;
			else
				nIsRepeated = 1;
			break;
		default:
			nIsPrefix = 0;						// ǰ׺����Ϊ0
			break;
		};
		if (nIsPrefix == 0 || nIsRepeated != 0)	// No more prefixes or duplicated prefix��û��ǰ׺���ظ���ǰ׺��
			break;
		if (g_nDisasmMode >= DISASM_FILE)		// ��ǰ�ķ����ģʽΪ���������Ƿ�������
			g_nDumpLen += sprintf(g_stDisasmInfo->uszDump + g_nDumpLen, "%02X:", *g_uszCmd);//��ʽ��ǰ׺�͵�������
		g_stDisasmInfo->nPrefix++;				// ��ǰ׺������һ
		g_uszCmd++;								// ָ����һ���ַ�
		ulHexAddress++;							// ָ���ַ��һ
		g_ulHexLength--;						// ��ǰָ��ʣ�೤�ȼ�һ
		ulPrefixCounts++;						// ǰ׺��������һ
	};

	// We do have repeated prefix. Flush first prefix from the sequence.
	// �������ȷʵ���ظ���ǰ׺����������ˢ�µ�һ��ǰ׺��
	if (nIsRepeated)
	{
		if (g_nDisasmMode >= DISASM_FILE)				// ��ǰ��ģʽ�Ƿ����ģʽ
		{
			g_stDisasmInfo->uszDump[3] = '\0';			// Leave only first dumped prefix��ֻ������һ��ת����ǰ׺��
			g_stDisasmInfo->nPrefix = 1;				// ǰ׺��������Ϊ1
			switch (g_uszCmd[-(long)ulPrefixCounts])	// �ж�ǰ׺
			{
			case 0x26: pName = (unsigned char *)(cuszSegName[SEG_ES]); break;
			case 0x2E: pName = (unsigned char *)(cuszSegName[SEG_CS]); break;
			case 0x36: pName = (unsigned char *)(cuszSegName[SEG_SS]); break;
			case 0x3E: pName = (unsigned char *)(cuszSegName[SEG_DS]); break;
			case 0x64: pName = (unsigned char *)(cuszSegName[SEG_FS]); break;
			case 0x65: pName = (unsigned char *)(cuszSegName[SEG_GS]); break;
			case 0x66: pName = "DATASIZE"; break;
			case 0x67: pName = "ADDRSIZE"; break;
			case 0xF0: pName = "LOCK"; break;
			case 0xF2: pName = "REPNE"; break;
			case 0xF3: pName = "REPE"; break;
			default: pName = "?"; break;				// ���������Ҳ���ǰ׺�ַ���
			};
			g_nResultLen += sprintf(g_stDisasmInfo->uszResult + g_nResultLen, "PREFIX %s:", pName);// ����ǰ׺
			if (g_nLowerCase) strlwr(g_stDisasmInfo->uszResult);//ת��ΪСд�ַ�
			if (g_nExtraPrefix == 0) strcpy(g_stDisasmInfo->uszComment, "Superfluous prefix");// �������
		};
		g_stDisasmInfo->nWarnings |= DAW_PREFIX;				// ����ǰ׺����
		if (nLockPrefix) g_stDisasmInfo->nWarnings |= DAW_LOCK;	// ����ǰ׺����
		g_stDisasmInfo->nCmdType = C_RARE;						// ����������
		return 1;												// Any prefix is 1 byte long���κ�ǰ׺��1�ֽڳ��ȡ�
	};
	/* If lock prefix available, display it and forget, because it has no
	 influence on decoding of rest of the command.*/
	 /*������Ҫ��������ǰ׺����Ϊ��û�ж��������ಿ�ֽ����Ӱ�졿*/
	if (nLockPrefix != 0)
	{
		if (g_nDisasmMode >= DISASM_FILE)												// ����Ƿ����ģʽ
			g_nResultLen += sprintf(g_stDisasmInfo->uszResult + g_nResultLen, "LOCK ");	// ��ʽ������ǰ׺
		g_stDisasmInfo->nWarnings |= DAW_LOCK;											// ����ǰ׺����
	};
	/* Fetch (if available) first 3 bytes of the command, add repeat prefix and find command in the command table.*/
	 /*����ȡ(�������)�����ǰ3���ֽڣ�����ظ�ǰ׺����������в������*/
	ulCode = 0;
	if (g_ulHexLength > 0) *(((unsigned char *)&ulCode) + 0) = g_uszCmd[0];
	if (g_ulHexLength > 1) *(((unsigned char *)&ulCode) + 1) = g_uszCmd[1];
	if (g_ulHexLength > 2) *(((unsigned char *)&ulCode) + 2) = g_uszCmd[2];
	if (nRepPrefix != 0)							// RER/REPE/REPNE is considered to be��������ظ�ǰ׺�Ļ���
		ulCode = (ulCode << 8) | nRepPrefix;		// part of command.���Ǿ͸��ǵ��ظ�ǰ׺��
	if (g_nDecodeVxD && (ulCode & 0xFFFF) == 0x20CD)// �����VxD����Code��0xFFFF
		pd = &cstVxDCmd;							// Decode VxD call (Win95/98)������vxd���á�
	else
	{
		for (pd = cstCmdData; pd->ulMask != 0; pd++)	// ��ָ����в�����
		{
			if (((ulCode^pd->ulCode) & pd->ulMask) != 0)// ^�����ơ�λ��ͬΪ0������ͬΪ1��
				continue;
			if (g_nDisasmMode >= DISASM_FILE && g_nShortStringCmds &&
				(pd->ucArg1 == MSO || pd->ucArg1 == MDE || pd->ucArg2 == MSO || pd->ucArg2 == MDE))
				continue;                  // Search short form of string command�������ַ����������д��ʽ��
			break;
		};
	};
	if ((pd->ucType & C_TYPEMASK) == C_NOW)// 3DNow! commands require additional search.��3DNow!������Ҫ�����������
	{
		nIs3DNow = 1;
		j = Get3dnowsuffix();
		if (j < 0)
			g_stDisasmInfo->nError = DAE_CROSS;
		else
		{
			for (; pd->ulMask != 0; pd++)
			{
				if (((ulCode^pd->ulCode) & pd->ulMask) != 0) continue;
				if (((uchar *)&(pd->ulCode))[2] == j) break;
			};
		};
	};
	if (pd->ulMask == 0) // Command not found������Ϊ0 �Ҳ������
	{
		g_stDisasmInfo->nCmdType = C_BAD;							// �޷�ʶ�������
		if (g_ulHexLength < 2) g_stDisasmInfo->nError = DAE_CROSS;	// ����̫С����Խ��ĩβ
		else g_stDisasmInfo->nError = DAE_BADCMD;					// �޷�ʶ����������
	}
	else // Command recognized, decode it�����������ʶ�𣬽��������
	{
		g_stDisasmInfo->nCmdType = pd->ucType;	// ����
		nEcxSize = g_ulDataSize;				// Default size of ECX used as counter��������������ECX��Ĭ�ϴ�С��
		if (g_nSegPrefix == SEG_FS || g_nSegPrefix == SEG_GS || nLockPrefix != 0)
			g_stDisasmInfo->nCmdType |= C_RARE;		// These prefixes are rare����Щǰ׺���ټ���
		if (pd->ucBits == PR)
			g_stDisasmInfo->nWarnings |= DAW_PRIV;	// Privileged command (ring 0)��Ring0����Ȩ���
		else if (pd->ucBits == WP)
			g_stDisasmInfo->nWarnings |= DAW_IO;	// I/O command��I/Oָ�
		/*Win32 programs usually try to keep stack dword-aligned, so INC ESP
		 (44) and DEC ESP (4C) usually don't appear in real code. Also check for
		 ADD ESP,imm and SUB ESP,imm (81,C4,imm32; 83,C4,imm8; 81,EC,imm32;83,EC,imm8).*/
		 /*��win32����ͨ����ͼ���ֶ�ջdword���룬���inc esp��44����dec esp��4c��ͨ�����������ʵ�ʴ����С�*/
		if (g_uszCmd[0] == 0x44
			|| g_uszCmd[0] == 0x4C
			|| (g_ulHexLength >= 3 && (g_uszCmd[0] == 0x81 || g_uszCmd[0] == 0x83)
			&& (g_uszCmd[1] == 0xC4 || g_uszCmd[1] == 0xEC) && (g_uszCmd[2] & 0x03) != 0))
		{
			g_stDisasmInfo->nWarnings |= DAW_STACK;	// δ����Ķ�ջ����
			g_stDisasmInfo->nCmdType |= C_RARE;		// ����������
		};
		// Warn also on MOV SEG,... (8E...). Win32 works in flat mode.
		// ��win32��ƽ��ģʽ��������MOV SEG��Ҳ�������桿
		if (g_uszCmd[0] == 0x8E)
			g_stDisasmInfo->nWarnings |= DAW_SEGMENT;	// ������ضμĴ�������
		// If opcode is 2-byte, adjust command.�����������Ϊ2�ֽڣ���������
		if (pd->ucLen == 2)
		{
			if (g_ulHexLength == 0)g_stDisasmInfo->nError = DAE_CROSS;	// Խ�����
			else
			{
				if (g_nDisasmMode >= DISASM_FILE)		// �Ƿ����ģʽ
					g_nDumpLen += sprintf(g_stDisasmInfo->uszDump + g_nDumpLen, "%02X", *g_uszCmd);//��ʽ��
				g_uszCmd++; ulHexAddress++; g_ulHexLength--;
			};
		};
		if (g_ulHexLength == 0)
			g_stDisasmInfo->nError = DAE_CROSS;	// Խ�����
		// Some commands either feature non-standard data size or have bit which allowes to select data size.
		//�� ��Щ����Ҫô���зǱ�׼���ݴ�С��Ҫô��������ѡ�����ݴ�С��λ����
		if ((pd->ucBits & WW) != 0 && (*g_uszCmd & WW) == 0)
			g_ulDataSize = 1;                      // Bit W in command set to 0����������С��������Ϊ0��
		else if ((pd->ucBits & W3) != 0 && (*g_uszCmd & W3) == 0)
			g_ulDataSize = 1;                      // Another position of bit W��Wλ����һ��λ�á�
		else if ((pd->ucBits & FF) != 0)
			g_ulDataSize = 2;                      // Forced word (2-byte) size��ǿ���֣�2�ֽڣ���С��
		  /* Some commands either have mnemonics which depend on data size (8/16 bits
		   or 32 bits, like CWD/CDQ), or have several different mnemonics (like
		   JNZ/JNE). First case is marked by either '&' (mnemonic depends on
		   operand size) or '$' (depends on address size). In the second case,
		   there is no special marker and disassembler selects main mnemonic.*/
		   /*����Щ���������Ƿ���ȡ�������ݴ�С��8/16λ����32λ������cwd/cdq���������м�����ͬ�����Ƿ�������
			JNZ/JNE������һ����Сд�á�&�������Ƿ�ȡ���ڲ�������С����$����ȡ���ڵ�ַ��С����
			�ڵڶ�������£�û������ı�ǣ���������ѡ�������Ƿ�����*/
		if (g_nDisasmMode >= DISASM_FILE)		// �����ģʽ
		{
			if (pd->uszName[0] == '&')			// ȡ���ڲ�������С
				nMnemoSize = g_ulDataSize;
			else if (pd->uszName[0] == '$')		// ȡ���ڵ�ַ��С
				nMnemoSize = g_ulAddrSize;
			else nMnemoSize = 0;				// ��С��ֵ
			if (nMnemoSize != 0)
			{
				for (i = 0, j = 1; pd->uszName[j] != '\0'; j++)
				{
					if (pd->uszName[j] == ':') // Separator between 16/32 mnemonics��16/32���Ƿ�֮��ķָ�����
					{
						if (nMnemoSize == 4)
							i = 0;
						else break;
					}
					else if (pd->uszName[j] == '*') //Substitute by 'W', 'D' or none ���á�w��\��d��\��none���滻��
					{
						if (nMnemoSize == 4 && g_nSizeSens != 2)
							name[i++] = 'D';
						else if (nMnemoSize != 4 && g_nSizeSens != 0)
							name[i++] = 'W';
					}
					else
						name[i++] = pd->uszName[j];// ֱ�Ӹ���
				};
				name[i] = '\0';
			}
			else
			{
				strcpy(name, pd->uszName);
				for (i = 0; name[i] != '\0'; i++)
				{
					if (name[i] == ',') // Use main mnemonic��ʹ�������Ƿ���
					{
						name[i] = '\0'; 
						break;
					};
				};
			};
			if (nRepPrefix != 0 && g_nTabArguments)// ���ظ�ǰ׺��Ҫ�Ʊ��
			{
				for (i = 0; name[i] != '\0' && name[i] != ' '; i++)
					g_stDisasmInfo->uszResult[g_nResultLen++] = name[i];
				if (name[i] == ' ')
				{
					g_stDisasmInfo->uszResult[g_nResultLen++] = ' '; i++;
				};
				while (g_nResultLen < 8) g_stDisasmInfo->uszResult[g_nResultLen++] = ' ';
				for (; name[i] != '\0'; i++)
					g_stDisasmInfo->uszResult[g_nResultLen++] = name[i];
				;
			}
			else// ֱ�Ӹ�ֵ
				g_nResultLen += sprintf(g_stDisasmInfo->uszResult + g_nResultLen, "%s", name);
			if (g_nLowerCase)strlwr(g_stDisasmInfo->uszResult);//�ַ�Сд
		};
		/* Decode operands (explicit - encoded in command, implicit - present in
		 mmemonic or assumed - used or modified by command). Assumed operands
		 must stay after all explicit and implicit operands. Up to 3 operands
		 are allowed.*/
		 /*���������������ʽ-�������б��룬��ʽ-��mmemonic��ٶ�-������ʹ�û��޸ģ���
		    �ٶ����������뱣����������ʽ����ʽ������֮�����3������������ġ���*/
		for (nOperandCount = 0; nOperandCount < 3; nOperandCount++)
		{
			if (g_stDisasmInfo->nError)
				break;            // Error - no sense to continue������-�޷�������

			/* If command contains both source and destination, one usually must not
			 decode destination to comment because it will be overwritten on the
			 next step. Global addcomment takes care of this. Decoding routines,
			 however, may ignore this flag.*/
			 /*���������ͬʱ����Դ��������Ŀ�����������ͨ�����ܽ�Ŀ�����Ϊע�ͣ���Ϊ������
			 ��һ����global add comment�ᴦ��������⡣������򣬵��ǣ����Ժ��Դ˱�־����*/
			if (nOperandCount == 0 && pd->ucArg2 != NNN && pd->ucArg2 < PSEUDOOP)
				g_nAddComment = 0;
			else
				g_nAddComment = 1;
			// Get type of next argument.����ȡ��һ�����������͡���
			if (nOperandCount == 0)			// ��һ�ζ�ȡ
				nCurrentArg = pd->ucArg1;
			else if (nOperandCount == 1)	// �ڶ��ζ�ȡ
				nCurrentArg = pd->ucArg2;
			else                            // �����ζ�ȡ
				nCurrentArg = pd->ucArg3;
			if (nCurrentArg == NNN)			// û�в�����
				break;             // No more operands�������в�������

			/* Arguments with arg>=PSEUDOOP are assumed operands and are not
			 displayed in disassembled result, so they require no delimiter.*/
			 /*��arg>=α�����Ĳ����Ǽٶ��Ĳ���������������ʾ�ڷ�������У���˲���Ҫ�ָ�������*/
			if ((g_nDisasmMode >= DISASM_FILE) && nCurrentArg < PSEUDOOP)
			{
				if (nOperandCount == 0)		// ����ǵ�һ��
				{
					g_stDisasmInfo->uszResult[g_nResultLen++] = ' ';
					if (g_nTabArguments)	// �����Ҫ�Ʊ��
					{
						while (g_nResultLen < 8) g_stDisasmInfo->uszResult[g_nResultLen++] = ' ';
					};
				}
				else						// ���ǵ�һ��
				{
					g_stDisasmInfo->uszResult[g_nResultLen++] = ',';//������֮��Ķ��ż��
					if (g_nExtraSpace) g_stDisasmInfo->uszResult[g_nResultLen++] = ' ';// ����Ҫ����ո�
				};
			};
			// Decode, analyse and comment next operand of the command.�����롢������ע������Ĳ�������
			switch (nCurrentArg)
			{
			case REG:						// Integer register in Reg field��REG�ֶ��е������Ĵ�����
				if (g_ulHexLength < 2)		// ����С��2
					g_stDisasmInfo->nError = DAE_CROSS;	// Խ�����
				else
					DecodeRG(g_uszCmd[1] >> 3, g_ulDataSize, REG);
				g_nModRM = 1;				// ��ModRM
				break;
			case RCM:						// Integer register in command byte�������ֽ��е������Ĵ�����
				DecodeRG(g_uszCmd[0], g_ulDataSize, RCM);
				break;
			case RG4:						// Integer 4-byte register in Reg field��REG�ֶ��е�����4�ֽڼĴ�����
				if (g_ulHexLength < 2)		// ����С��2
					g_stDisasmInfo->nError = DAE_CROSS;	 // Խ�����
				else 
					DecodeRG(g_uszCmd[1] >> 3, 4, RG4);
				g_nModRM = 1;				// ��ModRM
				break;
			case RAC:                       // Accumulator (AL/AX/EAX, implicit)���ۼ�����
				DecodeRG(REG_EAX, g_ulDataSize, RAC); 
				break;
			case RAX:                      // AX (2-byte, implicit)
				DecodeRG(REG_EAX, 2, RAX);
				break;
			case RDX:                      // DX (16-bit implicit port address)
				DecodeRG(REG_EDX, 2, RDX); 
				break;
			case RCL:                      // Implicit CL register (for shifts)
				DecodeRG(REG_ECX, 1, RCL);
				break;
			case RS0:                      // Top of FPU stack (ST(0))
				DecodeST(0, 0); 
				break;
			case RST:                      // FPU register (ST(i)) in command byte
				DecodeST(g_uszCmd[0], 0); 
				break;
			case RMX:                       // MMX register MMx��MMX�Ĵ�����
				if (g_ulHexLength < 2)		// ����С��2
					g_stDisasmInfo->nError = DAE_CROSS;	//Խ�����
				else 
					DecodeMX(g_uszCmd[1] >> 3);
				g_nModRM = 1; 
				break;
			case R3D:                      // 3DNow! register MMx
				if (g_ulHexLength < 2) 
					g_stDisasmInfo->nError = DAE_CROSS;
				else 
					DecodeNR(g_uszCmd[1] >> 3);
				g_nModRM = 1;
				break;
			case MRG:                      // Memory/register in ModRM byte
			case MRJ:                      // Memory/reg in ModRM as JUMP target
			case MR1:                      // 1-byte memory/register in ModRM byte
			case MR2:                      // 2-byte memory/register in ModRM byte
			case MR4:                      // 4-byte memory/register in ModRM byte
			case MR8:                      // 8-byte memory/MMX register in ModRM
			case MRD:                      // 8-byte memory/3DNow! register in ModRM
			case MMA:                      // Memory address in ModRM byte for LEA
			case MML:                      // Memory in ModRM byte (for LES)
			case MM6:                      // Memory in ModRm (6-byte descriptor)
			case MMB:                      // Two adjacent memory locations (BOUND)
			case MD2:                      // Memory in ModRM byte (16-bit integer)
			case MB2:                      // Memory in ModRM byte (16-bit binary)
			case MD4:                      // Memory in ModRM byte (32-bit integer)
			case MD8:                      // Memory in ModRM byte (64-bit integer)
			case MDA:                      // Memory in ModRM byte (80-bit BCD)
			case MF4:                      // Memory in ModRM byte (32-bit float)
			case MF8:                      // Memory in ModRM byte (64-bit float)
			case MFA:                      // Memory in ModRM byte (80-bit float)
			case MFE:                      // Memory in ModRM byte (FPU environment)
			case MFS:                      // Memory in ModRM byte (FPU state)
			case MFX:                      // Memory in ModRM byte (ext. FPU state)
				DecodeMR(nCurrentArg);
				break;
			case MMS:                      // Memory in ModRM byte (as SEG:OFFS)
				DecodeMR(nCurrentArg);
				g_stDisasmInfo->nWarnings |= DAW_FARADDR;
				break;
			case RR4:                      // 4-byte memory/register (register only)
			case RR8:                      // 8-byte MMX register only in ModRM
			case RRD:                      // 8-byte memory/3DNow! (register only)
				if ((g_uszCmd[1] & 0xC0) != 0xC0)
					g_nSoftError = DAE_REGISTER;
				DecodeMR(nCurrentArg);
				break;
			case MSO:                      // Source in string op's ([ESI])
				DecodeSO();
				break;
			case MDE:                      // Destination in string op's ([EDI])
				DecodeDE(); 
				break;
			case MXL:                      // XLAT operand ([EBX+AL])
				DecodeXL();
				break;
			case IMM:                      // Immediate data (8 or 16/32)
			case IMU:                      // Immediate unsigned data (8 or 16/32)
				if ((pd->ucBits & SS) != 0 && (*g_uszCmd & 0x02) != 0)
					DecodeIM(1, g_ulDataSize, nCurrentArg);
				else
					DecodeIM(g_ulDataSize, 0, nCurrentArg);
				break;
			case VXD:                      // VxD service (32-bit only)
				DecodeVX();
				break;
			case IMX:                      // Immediate sign-extendable byte
				DecodeIM(1, g_ulDataSize, nCurrentArg);
				break;
			case C01:                      // Implicit constant 1 (for shifts)
				DecodeC1();
				break;
			case IMS:                      // Immediate byte (for shifts)
			case IM1:                      // Immediate byte
				DecodeIM(1, 0, nCurrentArg); 
				break;
			case IM2:                      // Immediate word (ENTER/RET)
				DecodeIM(2, 0, nCurrentArg);
				if ((g_stDisasmInfo->ulImmConst & 0x03) != 0) 
					g_stDisasmInfo->nWarnings |= DAW_STACK;
				break;
			case IMA:                      // Immediate absolute near data address
				DecodeIA();
				break;
			case JOB:                      // Immediate byte offset (for jumps)
				DecodeRJ(1, ulHexAddress + 2);
				break;
			case JOW:                      // Immediate full offset (for jumps)
				DecodeRJ(g_ulDataSize, ulHexAddress + g_ulDataSize + 1);
				break;
			case JMF:                      // Immediate absolute far jump/call addr
				DecodeJF();
				g_stDisasmInfo->nWarnings |= DAW_FARADDR; 
				break;
			case SGM:                      // Segment register in ModRM byte
				if (g_ulHexLength < 2) 
					g_stDisasmInfo->nError = DAE_CROSS;
				DecodeSG(g_uszCmd[1] >> 3);
				g_nModRM = 1; break;
			case SCM:                      // Segment register in command byte
				DecodeSG(g_uszCmd[0] >> 3);
				if ((g_stDisasmInfo->nCmdType & C_TYPEMASK) == C_POP)
					g_stDisasmInfo->nWarnings |= DAW_SEGMENT;
				break;
			case CRX:                      // Control register CRx
				if ((g_uszCmd[1] & 0xC0) != 0xC0)
					g_stDisasmInfo->nError = DAE_REGISTER;
				DecodeCR(g_uszCmd[1]);
				break;
			case DRX:                      // Debug register DRx
				if ((g_uszCmd[1] & 0xC0) != 0xC0)
					g_stDisasmInfo->nError = DAE_REGISTER;
				DecodeDR(g_uszCmd[1]); 
				break;
			case PRN:                      // Near return address (pseudooperand)
				break;
			case PRF:                      // Far return address (pseudooperand)
				g_stDisasmInfo->nWarnings |= DAW_FARADDR;
				break;
			case PAC:                      // Accumulator (AL/AX/EAX, pseudooperand)
				DecodeRG(REG_EAX, g_ulDataSize, PAC);
				break;
			case PAH:                      // AH (in LAHF/SAHF, pseudooperand)
			case PFL:                      // Lower byte of flags (pseudooperand)
				break;
			case PS0:                      // Top of FPU stack (pseudooperand)
				DecodeST(0, 1); 
				break;
			case PS1:                      // ST(1) (pseudooperand)
				DecodeST(1, 1);
				break;
			case PCX:                      // CX/ECX (pseudooperand)
				DecodeRG(REG_ECX, nEcxSize, PCX);
				break;
			case PDI:                      // EDI (pseudooperand in MMX extentions)
				DecodeRG(REG_EDI, 4, PDI);
				break;
			default:
				g_stDisasmInfo->nError = DAE_INTERN;        // Unknown argument type��δ֪��������
				break;
			};
		};
		// Check whether command may possibly contain fixups.����������Ƿ���ܰ���������
		if (g_uszPfixUp != NULL && g_stDisasmInfo->nFixUpSize > 0)
			g_stDisasmInfo->nFixUpOffset = g_uszPfixUp - szHex;
		/* Segment prefix and address size prefix are superfluous for command which
		 does not access memory. If this the case, mark command as rare to help
		 in analysis.*/
		 /*����ǰ׺�͵�ַ��Сǰ׺�޷������ڴ档����������ͽ�������Ϊ�����ڷ����С�*/
		if (g_stDisasmInfo->nMemType == DEC_UNKNOWN &&
			(g_nSegPrefix != SEG_UNDEF || (g_ulAddrSize != 4 && pd->uszName[0] != '$'))) 
		{
			g_stDisasmInfo->nWarnings |= DAW_PREFIX;	// ����ǰ׺����
			g_stDisasmInfo->nCmdType |= C_RARE;			// ����������
		};
		/* 16-bit addressing is rare in 32-bit programs. 
		If this is the case,mark command as rare to help in analysis.*/
		/*��16λѰַ��32λ�����к��ټ�������������������������Ϊ�������԰�����������*/
		if (g_ulAddrSize != 4) 
			g_stDisasmInfo->nCmdType |= C_RARE;			// ����������
	};
	// Suffix of 3DNow! command is accounted best by assuming it immediate byte constant.
	// ��׺3dnow��������üٶ�Ϊ�����ֽڳ�����
	if (nIs3DNow)
	{
		if (g_nImmSize != 0) g_stDisasmInfo->nError = DAE_BADCMD;// �޷�ʶ�������
		else g_nImmSize = 1;
	};
	// Right or wrong, command decoded. Now dump it.
	// ������������ǾͰݰݡ�
	if (g_stDisasmInfo->nError != 0)
	{                  
		if (g_nDisasmMode >= DISASM_FILE)// Hard error in command detected����⵽�����е�Ӳ����
			g_nResultLen = sprintf(g_stDisasmInfo->uszResult, "???");
		if (g_stDisasmInfo->nError == DAE_BADCMD &&
			(*g_uszCmd == 0x0F || *g_uszCmd == 0xFF) && g_ulHexLength > 0) 
		{
			if (g_nDisasmMode >= DISASM_FILE) 
				g_nDumpLen += sprintf(g_stDisasmInfo->uszDump + g_nDumpLen, "%02X", *g_uszCmd);
			g_uszCmd++; 
			g_ulHexLength--;
		};
		if (g_ulHexLength > 0) 
		{
			if (g_nDisasmMode >= DISASM_FILE) 
				g_nDumpLen += sprintf(g_stDisasmInfo->uszDump + g_nDumpLen, "%02X", *g_uszCmd);
			g_uszCmd++; 
			g_ulHexLength--;
		};
	}
	else	// No hard error, dump command����Ӳ����ת�����
	{                               
		if (g_nDisasmMode >= DISASM_FILE)
		{
			g_nDumpLen += sprintf(g_stDisasmInfo->uszDump + g_nDumpLen, "%02X", *g_uszCmd++);//���
			if (g_nModRM) 
				g_nDumpLen += sprintf(g_stDisasmInfo->uszDump + g_nDumpLen, "%02X", *g_uszCmd++);//ModRM
			if (g_nSIB) 
				g_nDumpLen += sprintf(g_stDisasmInfo->uszDump + g_nDumpLen, "%02X", *g_uszCmd++);//SIB
			if (g_nDispSize != 0)// ��λ�ƴ�С
			{
				g_stDisasmInfo->uszDump[g_nDumpLen++] = ' ';
				for (i = 0; i < g_nDispSize; i++)
				{
					g_nDumpLen += sprintf(g_stDisasmInfo->uszDump + g_nDumpLen, "%02X", *g_uszCmd++);
				};
			};
			if (g_nImmSize != 0)// ��������
			{
				g_stDisasmInfo->uszDump[g_nDumpLen++] = ' ';
				for (i = 0; i < g_nImmSize; i++)
				{
					g_nDumpLen += sprintf(g_stDisasmInfo->uszDump + g_nDumpLen, "%02X", *g_uszCmd++);
				};
			};
		}
		else
			g_uszCmd += 1 + g_nModRM + g_nSIB + g_nDispSize + g_nImmSize;
		g_ulHexLength -= 1 + g_nModRM + g_nSIB + g_nDispSize + g_nImmSize;
	};
	// Check that command is not a dangerous one.����������Ƿ�Σ�ա�
	if (g_nDisasmMode >= DISASM_DATA)
	{
		for (pdan = cstDangerous; pdan->ulMask != 0; pdan++)
		{
			if (((ulCode^pdan->ulCode) & pdan->ulMask) != 0)
				continue;
			if (pdan->ucType == C_DANGERLOCK && nLockPrefix == 0)
				break;                         // Command harmless without LOCK prefix��������ǰ׺���޺����
			if (g_nIsWindowsNt && pdan->ucType == C_DANGER95)
				break;                         // Command harmless under Windows NT��windows Nt�µ��޺����
			  // Dangerous command!��Σ��ָ�
			if (pdan->ucType == C_DANGER95) g_stDisasmInfo->nWarnings |= DAW_DANGER95;
			else g_stDisasmInfo->nWarnings |= DAW_DANGEROUS;
			break;
		};
	};
	if (g_stDisasmInfo->nError == 0 && g_nSoftError != 0)
		g_stDisasmInfo->nError = g_nSoftError;// Error, but still display command�����󣬵�����ʾ���
	if (g_nDisasmMode >= DISASM_FILE)
	{
		if (g_stDisasmInfo->nError != DAE_NOERR) //�����д����
			switch (g_stDisasmInfo->nError)
			{
			case DAE_CROSS:		// Խ�����
				strcpy(g_stDisasmInfo->uszComment, "Command crosses end of memory block"); break;
			case DAE_BADCMD:	// �޷�ʶ���ָ��
				strcpy(g_stDisasmInfo->uszComment, "Unknown command"); break;
			case DAE_BADSEG:	// δ����μĴ���
				strcpy(g_stDisasmInfo->uszComment, "Undefined segment register"); break;
			case DAE_MEMORY:	// �ڴ����
				strcpy(g_stDisasmInfo->uszComment, "Illegal use of register"); break;
			case DAE_REGISTER:	// �ڴ����
				strcpy(g_stDisasmInfo->uszComment, "Memory address not allowed"); break;
			case DAE_INTERN:	// �ڲ�����
				strcpy(g_stDisasmInfo->uszComment, "Internal OLLYDBG error"); break;
			default:			// δ֪����
				strcpy(g_stDisasmInfo->uszComment, "Unknown error");
				break;
			}
		else if ((g_stDisasmInfo->nWarnings & DAW_PRIV) != 0 && g_nPrivileged == 0)// ��Ȩ����
			strcpy(g_stDisasmInfo->uszComment, "Privileged command");
		else if ((g_stDisasmInfo->nWarnings & DAW_IO) != 0 && g_nIOCommand == 0)// IO����
			strcpy(g_stDisasmInfo->uszComment, "I/O command");
		else if ((g_stDisasmInfo->nWarnings & DAW_FARADDR) != 0 && g_nFarCalls == 0)// ������Զ��ת ���� ����
		{
			if ((g_stDisasmInfo->nCmdType & C_TYPEMASK) == C_JMP)// Jmp
				strcpy(g_stDisasmInfo->uszComment, "Far jump");
			else if ((g_stDisasmInfo->nCmdType & C_TYPEMASK) == C_CAL)// Call
				strcpy(g_stDisasmInfo->uszComment, "Far call");
			else if ((g_stDisasmInfo->nCmdType & C_TYPEMASK) == C_RET)// Ret
				strcpy(g_stDisasmInfo->uszComment, "Far return");
			;
		}
		else if ((g_stDisasmInfo->nWarnings & DAW_SEGMENT) != 0 && g_nFarCalls == 0)// �޸ĶμĴ�������
			strcpy(g_stDisasmInfo->uszComment, "Modification of segment register");
		else if ((g_stDisasmInfo->nWarnings & DAW_SHIFT) != 0 && g_nBadShift == 0)// λ�Ƴ���������Χ����
			strcpy(g_stDisasmInfo->uszComment, "Shift constant out of range 1..31");
		else if ((g_stDisasmInfo->nWarnings & DAW_PREFIX) != 0 && g_nExtraPrefix == 0)// ����ǰ׺����
			strcpy(g_stDisasmInfo->uszComment, "Superfluous prefix");
		else if ((g_stDisasmInfo->nWarnings & DAW_LOCK) != 0 && g_nLockedBus == 0)// ����ǰ׺����
			strcpy(g_stDisasmInfo->uszComment, "LOCK prefix");
		else if ((g_stDisasmInfo->nWarnings & DAW_STACK) != 0 && g_nStackAlign == 0)// δ�����ջ����
			strcpy(g_stDisasmInfo->uszComment, "Unaligned stack operation");
		;
	};
	return (ulHexLen - g_ulHexLength);               // Returns number of recognized bytes�����ؿ�ʶ����ֽ�����
};

