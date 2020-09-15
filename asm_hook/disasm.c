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

// Work variables of disassembler【反汇编程序的工作变量】
static ulong     g_ulDataSize;         // Size of data (1,2,4 bytes)【数据大小】
static ulong     g_ulAddrSize;         // Size of address (2 or 4 bytes)【地址大小】
static int       g_nSegPrefix;         // Segment override prefix or SEG_UNDEF【段重载前缀】
static int       g_nModRM;             // Command has ModR/M byte【判断是否有ModR/M】
static int       g_nSIB;               // Command has SIB byte【判断是否有SIB】
static int       g_nDispSize;          // Size of displacement (if any)【位移大小】
static int       g_nImmSize;           // Size of immediate data (if any)【立即数大小】
static int       g_nSoftError;         // Noncritical disassembler error【非紧急反汇编错误】
static int       g_nDumpLen;           // Current length of command dump【当前转储的长度索引 max=255】
static int       g_nResultLen;         // Current length of disassembly【当前反汇编的长度索引 max=255】
static int       g_nAddComment;        // Comment value of operand【操作数的值】

// Copy of input parameters of function Disasm()【Disasm()函数的参数】
static unsigned char      *g_uszCmd;            // Pointer to binary data【二进制指针】
static unsigned char      *g_uszPfixUp;         // Pointer to possible fixups or NULL【指向可能的修正的指针】
static ulong     g_ulHexLength;                 // Remaining size of the command buffer【命令缓冲区的剩余大小】
static t_disasm  *g_stDisasmInfo;				// Pointer to disassembly results【指向反汇编结构】
static int       g_nDisasmMode;                 // Disassembly mode (DISASM_xxx)【反汇编模式】

/* Disassemble name of 1, 2 or 4-byte general-purpose integer register and, if
 requested and available, dump its contents. Parameter type changes decoding
 of contents for some operand types.*/
 /*【反汇编1、2或4字节通用整数寄存器的名称，如果请求并可用，转储其内容。
 参数类型更改解码某些操作数类型的内容。】*/
static void DecodeRG(int index, int datasize, int type)
{
	int sizeindex;
	char name[9];
	if (g_nDisasmMode < DISASM_DATA)
		return;        // No need to decode【不需要解码】
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
		if (type < PSEUDOOP)                 // Not a pseudooperand【不是伪操作数】
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
// 【从modrm/sib字节反汇编内存/寄存器，如果可用，转储内存的地址和内容。】
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
	 /*【解码ModM/SIB地址有很多可能。第一次可能是注册在ModM - general-purpose, MMX or 3DNow!】*/
	if ((c & 0xC0) == 0xC0) // Decode register operand【解码寄存器操作数】
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
/*【跳过3dnow！操作数并提取命令后缀】*/
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

ulong Disasm(unsigned char *szHex,		//需要翻译成汇编代码的十六进制文本
	ulong ulHexLen,						//十六进制文本的长度
	ulong ulHexAddress,					//指令在程序中的的地址,相对地址需要这个
	t_disasm *pDisasmInfo,				//接收汇编指信息
	int nDisasmMode)					//反汇编模式
{
	int i;								//循环索引
	int j;								//循环索引
	int nIsPrefix;						//判断是否有指令前缀
	int nIs3DNow;						//判断是否有3DNow指令
	int nIsRepeated;					//判断是否有重复前缀
	int nOperandCount;					//操作数的数量
	int nMnemoSize;						//操作数/地址大小 byte word dword
	int nCurrentArg;					//当前的arguments
	ulong ulPrefixCounts;				//前缀的数量
	ulong ulCode;
	int nLockPrefix;                    // Non-zero if lock prefix present【锁定前缀】
	int nRepPrefix;                     // REPxxx prefix or 0【重复前缀】
	int nEcxSize;						// 寄存器Ecx的长度
	unsigned char name[TEXTLEN];
	unsigned char* pName;				// 字符串指针
	const t_cmddata *pd;
	const t_cmddata *pdan;
	// Prepare disassembler variables and initialize structure disasm.【准备反汇编变量并初始化结构disasm】
	g_ulDataSize = 4;					// 32-bit code and data segments only!【仅限32位代码和数据段！】
	g_ulAddrSize = 4;					// 32-bit code and data segments only!【仅限32位代码和数据段！】
	g_nSegPrefix = SEG_UNDEF;			// 未知段重载前缀
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
	g_stDisasmInfo->uszComment[0] = '\0';				//清空描述
	g_stDisasmInfo->nCmdType = C_BAD;					//无法识别的命令
	g_stDisasmInfo->nPrefix = 0;						//前缀的数量
	g_stDisasmInfo->nMemType = DEC_UNKNOWN;				//未知类型
	g_stDisasmInfo->nIndexed = 0;						//地址包含的寄存器
	g_stDisasmInfo->ulJmpConst = 0;
	g_stDisasmInfo->ulJmpTable = 0;
	g_stDisasmInfo->ulAddrconst = 0;
	g_stDisasmInfo->ulImmConst = 0;
	g_stDisasmInfo->nZeroConst = 0;
	g_stDisasmInfo->nFixUpOffset = 0;
	g_stDisasmInfo->nFixUpSize = 0;
	g_stDisasmInfo->nWarnings = 0;						// 警告
	g_stDisasmInfo->nError = DAE_NOERR;					// 没有发生错误
	g_nDisasmMode = nDisasmMode;				// No need to use register contents【无需使用寄存器内容】
	/* Correct 80x86 command may theoretically contain up to 4 prefixes belonging
	 to different prefix groups. This limits maximal possible size of the
	 command to MAXCMDSIZE=16 bytes. In order to maintain this limit, if
	 Disasm() detects second prefix from the same group, it flushes first
	 prefix in the sequence as a pseudocommand.*/
	 /*正确的80x86命令理论上最多可以包含4个属于到不同的前缀组。这限制了命令到16字节。
	 为了维持这一限制，如果disasm函数检测同一组中的第二个前缀，它首先刷新作为伪命令的序列前缀*/
	ulPrefixCounts = 0;						// 前缀的数量为0
	nIsRepeated = 0;						// 没有重复前缀
	while (g_ulHexLength > 0)				// 长度不为空
	{
		nIsPrefix = 1;						// Assume that there is some prefix【假设有一个前缀】
		switch (*g_uszCmd)					// A-2单字节表的判断，先判断段前缀
		{
		case 0x26:
			if (g_nSegPrefix == SEG_UNDEF)	// 如果这是第一个前缀
				g_nSegPrefix = SEG_ES;		// 保存前缀
			else
				nIsRepeated = 1;			// 前缀的数量大于1
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
		case 0x66://Operand Size Prefix【操作数大小重载前缀】
			if (g_ulDataSize == 4)
				g_ulDataSize = 2;		
			else
				nIsRepeated = 1;
			break;
		case 0x67://Address Size Prefix【地址大小重载前缀】
			if (g_ulAddrSize == 4)
				g_ulAddrSize = 2;	
			else
				nIsRepeated = 1;
			break;
		case 0xF0://Lock【锁定前缀】
			if (nLockPrefix == 0)
				nLockPrefix = 0xF0;	
			else
				nIsRepeated = 1;
			break;
		case 0xF2://Repne【重复前缀】
			if (nRepPrefix == 0)
				nRepPrefix = 0xF2;
			else
				nIsRepeated = 1;
			break;
		case 0xF3://Rep【重复前缀】
			if (nRepPrefix == 0)
				nRepPrefix = 0xF3;
			else
				nIsRepeated = 1;
			break;
		default:
			nIsPrefix = 0;						// 前缀数量为0
			break;
		};
		if (nIsPrefix == 0 || nIsRepeated != 0)	// No more prefixes or duplicated prefix【没有前缀或重复的前缀】
			break;
		if (g_nDisasmMode >= DISASM_FILE)		// 当前的反汇编模式为反汇编而不是分析代码
			g_nDumpLen += sprintf(g_stDisasmInfo->uszDump + g_nDumpLen, "%02X:", *g_uszCmd);//格式化前缀和递增索引
		g_stDisasmInfo->nPrefix++;				// 段前缀数量加一
		g_uszCmd++;								// 指向下一个字符
		ulHexAddress++;							// 指令地址加一
		g_ulHexLength--;						// 当前指令剩余长度减一
		ulPrefixCounts++;						// 前缀的数量加一
	};

	// We do have repeated prefix. Flush first prefix from the sequence.
	// 如果我们确实有重复的前缀。从序列中刷新第一个前缀。
	if (nIsRepeated)
	{
		if (g_nDisasmMode >= DISASM_FILE)				// 当前的模式是反汇编模式
		{
			g_stDisasmInfo->uszDump[3] = '\0';			// Leave only first dumped prefix【只保留第一个转储的前缀】
			g_stDisasmInfo->nPrefix = 1;				// 前缀数量设置为1
			switch (g_uszCmd[-(long)ulPrefixCounts])	// 判断前缀
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
			default: pName = "?"; break;				// 发生错误找不着前缀字符串
			};
			g_nResultLen += sprintf(g_stDisasmInfo->uszResult + g_nResultLen, "PREFIX %s:", pName);// 保存前缀
			if (g_nLowerCase) strlwr(g_stDisasmInfo->uszResult);//转化为小写字符
			if (g_nExtraPrefix == 0) strcpy(g_stDisasmInfo->uszComment, "Superfluous prefix");// 添加描述
		};
		g_stDisasmInfo->nWarnings |= DAW_PREFIX;				// 多余前缀警告
		if (nLockPrefix) g_stDisasmInfo->nWarnings |= DAW_LOCK;	// 锁定前缀警告
		g_stDisasmInfo->nCmdType = C_RARE;						// 罕见的命令
		return 1;												// Any prefix is 1 byte long【任何前缀都1字节长度】
	};
	/* If lock prefix available, display it and forget, because it has no
	 influence on decoding of rest of the command.*/
	 /*【不需要处理锁定前缀，因为它没有对命令其余部分解码的影响】*/
	if (nLockPrefix != 0)
	{
		if (g_nDisasmMode >= DISASM_FILE)												// 如果是反汇编模式
			g_nResultLen += sprintf(g_stDisasmInfo->uszResult + g_nResultLen, "LOCK ");	// 格式化锁定前缀
		g_stDisasmInfo->nWarnings |= DAW_LOCK;											// 锁定前缀警告
	};
	/* Fetch (if available) first 3 bytes of the command, add repeat prefix and find command in the command table.*/
	 /*【获取(如果可用)命令的前3个字节，添加重复前缀，在命令表中查找命令】*/
	ulCode = 0;
	if (g_ulHexLength > 0) *(((unsigned char *)&ulCode) + 0) = g_uszCmd[0];
	if (g_ulHexLength > 1) *(((unsigned char *)&ulCode) + 1) = g_uszCmd[1];
	if (g_ulHexLength > 2) *(((unsigned char *)&ulCode) + 2) = g_uszCmd[2];
	if (nRepPrefix != 0)							// RER/REPE/REPNE is considered to be【如果有重复前缀的话】
		ulCode = (ulCode << 8) | nRepPrefix;		// part of command.【那就覆盖掉重复前缀】
	if (g_nDecodeVxD && (ulCode & 0xFFFF) == 0x20CD)// 如果是VxD而且Code是0xFFFF
		pd = &cstVxDCmd;							// Decode VxD call (Win95/98)【解码vxd调用】
	else
	{
		for (pd = cstCmdData; pd->ulMask != 0; pd++)	// 对指令进行查表操作
		{
			if (((ulCode^pd->ulCode) & pd->ulMask) != 0)// ^二进制【位相同为0，不相同为1】
				continue;
			if (g_nDisasmMode >= DISASM_FILE && g_nShortStringCmds &&
				(pd->ucArg1 == MSO || pd->ucArg1 == MDE || pd->ucArg2 == MSO || pd->ucArg2 == MDE))
				continue;                  // Search short form of string command【搜索字符串命令的缩写形式】
			break;
		};
	};
	if ((pd->ucType & C_TYPEMASK) == C_NOW)// 3DNow! commands require additional search.【3DNow!命令需要额外的搜索】
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
	if (pd->ulMask == 0) // Command not found【掩码为0 找不到命令】
	{
		g_stDisasmInfo->nCmdType = C_BAD;							// 无法识别的命令
		if (g_ulHexLength < 2) g_stDisasmInfo->nError = DAE_CROSS;	// 长度太小导致越过末尾
		else g_stDisasmInfo->nError = DAE_BADCMD;					// 无法识别的命令错误
	}
	else // Command recognized, decode it【对命令进行识别，解码操作】
	{
		g_stDisasmInfo->nCmdType = pd->ucType;	// 复制
		nEcxSize = g_ulDataSize;				// Default size of ECX used as counter【用作计数器的ECX的默认大小】
		if (g_nSegPrefix == SEG_FS || g_nSegPrefix == SEG_GS || nLockPrefix != 0)
			g_stDisasmInfo->nCmdType |= C_RARE;		// These prefixes are rare【这些前缀很少见】
		if (pd->ucBits == PR)
			g_stDisasmInfo->nWarnings |= DAW_PRIV;	// Privileged command (ring 0)【Ring0的特权命令】
		else if (pd->ucBits == WP)
			g_stDisasmInfo->nWarnings |= DAW_IO;	// I/O command【I/O指令】
		/*Win32 programs usually try to keep stack dword-aligned, so INC ESP
		 (44) and DEC ESP (4C) usually don't appear in real code. Also check for
		 ADD ESP,imm and SUB ESP,imm (81,C4,imm32; 83,C4,imm8; 81,EC,imm32;83,EC,imm8).*/
		 /*【win32程序通常试图保持堆栈dword对齐，因此inc esp（44）和dec esp（4c）通常不会出现在实际代码中】*/
		if (g_uszCmd[0] == 0x44
			|| g_uszCmd[0] == 0x4C
			|| (g_ulHexLength >= 3 && (g_uszCmd[0] == 0x81 || g_uszCmd[0] == 0x83)
			&& (g_uszCmd[1] == 0xC4 || g_uszCmd[1] == 0xEC) && (g_uszCmd[2] & 0x03) != 0))
		{
			g_stDisasmInfo->nWarnings |= DAW_STACK;	// 未对齐的堆栈操作
			g_stDisasmInfo->nCmdType |= C_RARE;		// 罕见的命令
		};
		// Warn also on MOV SEG,... (8E...). Win32 works in flat mode.
		// 【win32在平面模式工作，在MOV SEG上也发出警告】
		if (g_uszCmd[0] == 0x8E)
			g_stDisasmInfo->nWarnings |= DAW_SEGMENT;	// 命令加载段寄存器警告
		// If opcode is 2-byte, adjust command.【如果操作码为2字节，请调整命令】
		if (pd->ucLen == 2)
		{
			if (g_ulHexLength == 0)g_stDisasmInfo->nError = DAE_CROSS;	// 越界错误
			else
			{
				if (g_nDisasmMode >= DISASM_FILE)		// 是反汇编模式
					g_nDumpLen += sprintf(g_stDisasmInfo->uszDump + g_nDumpLen, "%02X", *g_uszCmd);//格式化
				g_uszCmd++; ulHexAddress++; g_ulHexLength--;
			};
		};
		if (g_ulHexLength == 0)
			g_stDisasmInfo->nError = DAE_CROSS;	// 越界错误
		// Some commands either feature non-standard data size or have bit which allowes to select data size.
		//【 有些命令要么具有非标准数据大小，要么具有允许选择数据大小的位。】
		if ((pd->ucBits & WW) != 0 && (*g_uszCmd & WW) == 0)
			g_ulDataSize = 1;                      // Bit W in command set to 0【操作数大小命令设置为0】
		else if ((pd->ucBits & W3) != 0 && (*g_uszCmd & W3) == 0)
			g_ulDataSize = 1;                      // Another position of bit W【W位的另一个位置】
		else if ((pd->ucBits & FF) != 0)
			g_ulDataSize = 2;                      // Forced word (2-byte) size【强制字（2字节）大小】
		  /* Some commands either have mnemonics which depend on data size (8/16 bits
		   or 32 bits, like CWD/CDQ), or have several different mnemonics (like
		   JNZ/JNE). First case is marked by either '&' (mnemonic depends on
		   operand size) or '$' (depends on address size). In the second case,
		   there is no special marker and disassembler selects main mnemonic.*/
		   /*【有些命令有助记符，取决于数据大小（8/16位或者32位，比如cwd/cdq），或者有几个不同的助记符（比如
			JNZ/JNE）。第一个大小写用“&”（助记符取决于操作数大小）或“$”（取决于地址大小）。
			在第二种情况下，没有特殊的标记，反汇编程序选择主助记符。】*/
		if (g_nDisasmMode >= DISASM_FILE)		// 反汇编模式
		{
			if (pd->uszName[0] == '&')			// 取决于操作数大小
				nMnemoSize = g_ulDataSize;
			else if (pd->uszName[0] == '$')		// 取决于地址大小
				nMnemoSize = g_ulAddrSize;
			else nMnemoSize = 0;				// 大小赋值
			if (nMnemoSize != 0)
			{
				for (i = 0, j = 1; pd->uszName[j] != '\0'; j++)
				{
					if (pd->uszName[j] == ':') // Separator between 16/32 mnemonics【16/32助记符之间的分隔符】
					{
						if (nMnemoSize == 4)
							i = 0;
						else break;
					}
					else if (pd->uszName[j] == '*') //Substitute by 'W', 'D' or none 【用“w”\“d”\“none”替换】
					{
						if (nMnemoSize == 4 && g_nSizeSens != 2)
							name[i++] = 'D';
						else if (nMnemoSize != 4 && g_nSizeSens != 0)
							name[i++] = 'W';
					}
					else
						name[i++] = pd->uszName[j];// 直接复制
				};
				name[i] = '\0';
			}
			else
			{
				strcpy(name, pd->uszName);
				for (i = 0; name[i] != '\0'; i++)
				{
					if (name[i] == ',') // Use main mnemonic【使用主助记符】
					{
						name[i] = '\0'; 
						break;
					};
				};
			};
			if (nRepPrefix != 0 && g_nTabArguments)// 有重复前缀且要制表符
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
			else// 直接赋值
				g_nResultLen += sprintf(g_stDisasmInfo->uszResult + g_nResultLen, "%s", name);
			if (g_nLowerCase)strlwr(g_stDisasmInfo->uszResult);//字符小写
		};
		/* Decode operands (explicit - encoded in command, implicit - present in
		 mmemonic or assumed - used or modified by command). Assumed operands
		 must stay after all explicit and implicit operands. Up to 3 operands
		 are allowed.*/
		 /*【解码操作数（显式-在命令中编码，隐式-在mmemonic或假定-由命令使用或修改）。
		    假定操作数必须保留在所有显式和隐式操作数之后。最多3个操作是允许的。】*/
		for (nOperandCount = 0; nOperandCount < 3; nOperandCount++)
		{
			if (g_stDisasmInfo->nError)
				break;            // Error - no sense to continue【错误-无法继续】

			/* If command contains both source and destination, one usually must not
			 decode destination to comment because it will be overwritten on the
			 next step. Global addcomment takes care of this. Decoding routines,
			 however, may ignore this flag.*/
			 /*【如果命令同时包含源操作数和目标操作数，则通常不能将目标解码为注释，因为它将在
			 下一步。global add comment会处理这个问题。解码程序，但是，可以忽略此标志。】*/
			if (nOperandCount == 0 && pd->ucArg2 != NNN && pd->ucArg2 < PSEUDOOP)
				g_nAddComment = 0;
			else
				g_nAddComment = 1;
			// Get type of next argument.【获取下一个参数的类型。】
			if (nOperandCount == 0)			// 第一次读取
				nCurrentArg = pd->ucArg1;
			else if (nOperandCount == 1)	// 第二次读取
				nCurrentArg = pd->ucArg2;
			else                            // 第三次读取
				nCurrentArg = pd->ucArg3;
			if (nCurrentArg == NNN)			// 没有操作数
				break;             // No more operands【不再有操作数】

			/* Arguments with arg>=PSEUDOOP are assumed operands and are not
			 displayed in disassembled result, so they require no delimiter.*/
			 /*【arg>=伪操作的参数是假定的操作数，而不是显示在反汇编结果中，因此不需要分隔符。】*/
			if ((g_nDisasmMode >= DISASM_FILE) && nCurrentArg < PSEUDOOP)
			{
				if (nOperandCount == 0)		// 如果是第一个
				{
					g_stDisasmInfo->uszResult[g_nResultLen++] = ' ';
					if (g_nTabArguments)	// 如果需要制表符
					{
						while (g_nResultLen < 8) g_stDisasmInfo->uszResult[g_nResultLen++] = ' ';
					};
				}
				else						// 不是第一个
				{
					g_stDisasmInfo->uszResult[g_nResultLen++] = ',';//操作数之间的逗号间隔
					if (g_nExtraSpace) g_stDisasmInfo->uszResult[g_nResultLen++] = ' ';// 还需要额外空格
				};
			};
			// Decode, analyse and comment next operand of the command.【解码、分析并注释命令的操作数】
			switch (nCurrentArg)
			{
			case REG:						// Integer register in Reg field【REG字段中的整数寄存器】
				if (g_ulHexLength < 2)		// 长度小于2
					g_stDisasmInfo->nError = DAE_CROSS;	// 越界错误
				else
					DecodeRG(g_uszCmd[1] >> 3, g_ulDataSize, REG);
				g_nModRM = 1;				// 有ModRM
				break;
			case RCM:						// Integer register in command byte【命令字节中的整数寄存器】
				DecodeRG(g_uszCmd[0], g_ulDataSize, RCM);
				break;
			case RG4:						// Integer 4-byte register in Reg field【REG字段中的整数4字节寄存器】
				if (g_ulHexLength < 2)		// 长度小于2
					g_stDisasmInfo->nError = DAE_CROSS;	 // 越界错误
				else 
					DecodeRG(g_uszCmd[1] >> 3, 4, RG4);
				g_nModRM = 1;				// 有ModRM
				break;
			case RAC:                       // Accumulator (AL/AX/EAX, implicit)【累加器】
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
			case RMX:                       // MMX register MMx【MMX寄存器】
				if (g_ulHexLength < 2)		// 长度小于2
					g_stDisasmInfo->nError = DAE_CROSS;	//越界错误
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
				g_stDisasmInfo->nError = DAE_INTERN;        // Unknown argument type【未知参数错误】
				break;
			};
		};
		// Check whether command may possibly contain fixups.【检查命令是否可能包含修正】
		if (g_uszPfixUp != NULL && g_stDisasmInfo->nFixUpSize > 0)
			g_stDisasmInfo->nFixUpOffset = g_uszPfixUp - szHex;
		/* Segment prefix and address size prefix are superfluous for command which
		 does not access memory. If this the case, mark command as rare to help
		 in analysis.*/
		 /*【段前缀和地址大小前缀无法访问内存。如果是这样就将命令标记为罕见在分析中】*/
		if (g_stDisasmInfo->nMemType == DEC_UNKNOWN &&
			(g_nSegPrefix != SEG_UNDEF || (g_ulAddrSize != 4 && pd->uszName[0] != '$'))) 
		{
			g_stDisasmInfo->nWarnings |= DAW_PREFIX;	// 多余前缀警告
			g_stDisasmInfo->nCmdType |= C_RARE;			// 罕见的命令
		};
		/* 16-bit addressing is rare in 32-bit programs. 
		If this is the case,mark command as rare to help in analysis.*/
		/*【16位寻址在32位程序中很少见。如果是这种情况，将命令标记为罕见，以帮助分析。】*/
		if (g_ulAddrSize != 4) 
			g_stDisasmInfo->nCmdType |= C_RARE;			// 罕见的命令
	};
	// Suffix of 3DNow! command is accounted best by assuming it immediate byte constant.
	// 后缀3dnow！命令最好假定为立即字节常量。
	if (nIs3DNow)
	{
		if (g_nImmSize != 0) g_stDisasmInfo->nError = DAE_BADCMD;// 无法识别的命令
		else g_nImmSize = 1;
	};
	// Right or wrong, command decoded. Now dump it.
	// 如果发生错误那就拜拜。
	if (g_stDisasmInfo->nError != 0)
	{                  
		if (g_nDisasmMode >= DISASM_FILE)// Hard error in command detected【检测到命令中的硬错误】
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
	else	// No hard error, dump command【无硬错误，转储命令】
	{                               
		if (g_nDisasmMode >= DISASM_FILE)
		{
			g_nDumpLen += sprintf(g_stDisasmInfo->uszDump + g_nDumpLen, "%02X", *g_uszCmd++);//汇编
			if (g_nModRM) 
				g_nDumpLen += sprintf(g_stDisasmInfo->uszDump + g_nDumpLen, "%02X", *g_uszCmd++);//ModRM
			if (g_nSIB) 
				g_nDumpLen += sprintf(g_stDisasmInfo->uszDump + g_nDumpLen, "%02X", *g_uszCmd++);//SIB
			if (g_nDispSize != 0)// 有位移大小
			{
				g_stDisasmInfo->uszDump[g_nDumpLen++] = ' ';
				for (i = 0; i < g_nDispSize; i++)
				{
					g_nDumpLen += sprintf(g_stDisasmInfo->uszDump + g_nDumpLen, "%02X", *g_uszCmd++);
				};
			};
			if (g_nImmSize != 0)// 有立即数
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
	// Check that command is not a dangerous one.【检查命令是否危险】
	if (g_nDisasmMode >= DISASM_DATA)
	{
		for (pdan = cstDangerous; pdan->ulMask != 0; pdan++)
		{
			if (((ulCode^pdan->ulCode) & pdan->ulMask) != 0)
				continue;
			if (pdan->ucType == C_DANGERLOCK && nLockPrefix == 0)
				break;                         // Command harmless without LOCK prefix【不带锁前缀的无害命令】
			if (g_nIsWindowsNt && pdan->ucType == C_DANGER95)
				break;                         // Command harmless under Windows NT【windows Nt下的无害命令】
			  // Dangerous command!【危险指令】
			if (pdan->ucType == C_DANGER95) g_stDisasmInfo->nWarnings |= DAW_DANGER95;
			else g_stDisasmInfo->nWarnings |= DAW_DANGEROUS;
			break;
		};
	};
	if (g_stDisasmInfo->nError == 0 && g_nSoftError != 0)
		g_stDisasmInfo->nError = g_nSoftError;// Error, but still display command【错误，但仍显示命令】
	if (g_nDisasmMode >= DISASM_FILE)
	{
		if (g_stDisasmInfo->nError != DAE_NOERR) //还是有错误的
			switch (g_stDisasmInfo->nError)
			{
			case DAE_CROSS:		// 越界错误
				strcpy(g_stDisasmInfo->uszComment, "Command crosses end of memory block"); break;
			case DAE_BADCMD:	// 无法识别的指令
				strcpy(g_stDisasmInfo->uszComment, "Unknown command"); break;
			case DAE_BADSEG:	// 未定义段寄存器
				strcpy(g_stDisasmInfo->uszComment, "Undefined segment register"); break;
			case DAE_MEMORY:	// 内存错误
				strcpy(g_stDisasmInfo->uszComment, "Illegal use of register"); break;
			case DAE_REGISTER:	// 内存错误
				strcpy(g_stDisasmInfo->uszComment, "Memory address not allowed"); break;
			case DAE_INTERN:	// 内部错误
				strcpy(g_stDisasmInfo->uszComment, "Internal OLLYDBG error"); break;
			default:			// 未知错误
				strcpy(g_stDisasmInfo->uszComment, "Unknown error");
				break;
			}
		else if ((g_stDisasmInfo->nWarnings & DAW_PRIV) != 0 && g_nPrivileged == 0)// 特权警告
			strcpy(g_stDisasmInfo->uszComment, "Privileged command");
		else if ((g_stDisasmInfo->nWarnings & DAW_IO) != 0 && g_nIOCommand == 0)// IO警告
			strcpy(g_stDisasmInfo->uszComment, "I/O command");
		else if ((g_stDisasmInfo->nWarnings & DAW_FARADDR) != 0 && g_nFarCalls == 0)// 命令是远跳转 调用 返回
		{
			if ((g_stDisasmInfo->nCmdType & C_TYPEMASK) == C_JMP)// Jmp
				strcpy(g_stDisasmInfo->uszComment, "Far jump");
			else if ((g_stDisasmInfo->nCmdType & C_TYPEMASK) == C_CAL)// Call
				strcpy(g_stDisasmInfo->uszComment, "Far call");
			else if ((g_stDisasmInfo->nCmdType & C_TYPEMASK) == C_RET)// Ret
				strcpy(g_stDisasmInfo->uszComment, "Far return");
			;
		}
		else if ((g_stDisasmInfo->nWarnings & DAW_SEGMENT) != 0 && g_nFarCalls == 0)// 修改段寄存器警告
			strcpy(g_stDisasmInfo->uszComment, "Modification of segment register");
		else if ((g_stDisasmInfo->nWarnings & DAW_SHIFT) != 0 && g_nBadShift == 0)// 位移常数超出范围警告
			strcpy(g_stDisasmInfo->uszComment, "Shift constant out of range 1..31");
		else if ((g_stDisasmInfo->nWarnings & DAW_PREFIX) != 0 && g_nExtraPrefix == 0)// 多余前缀警告
			strcpy(g_stDisasmInfo->uszComment, "Superfluous prefix");
		else if ((g_stDisasmInfo->nWarnings & DAW_LOCK) != 0 && g_nLockedBus == 0)// 锁定前缀警告
			strcpy(g_stDisasmInfo->uszComment, "LOCK prefix");
		else if ((g_stDisasmInfo->nWarnings & DAW_STACK) != 0 && g_nStackAlign == 0)// 未对齐堆栈警告
			strcpy(g_stDisasmInfo->uszComment, "Unaligned stack operation");
		;
	};
	return (ulHexLen - g_ulHexLength);               // Returns number of recognized bytes【返回可识别的字节数】
};

