// Free Disassembler and Assembler -- Header file
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

#ifndef MAINPROG
#define unique extern
#else
#define unique
#endif

// If you prefere Borland, this will force necessary setting (but, as a side
// effect, may cause plenty of warnings if other include files will be compiled
// with different options):

#ifdef __BORLANDC__
#pragma option -K                      // Unsigned char
#endif
/*
#if (unsigned char)0xFF!=255
#error Please set default char type to unsigned
#endif
*/

// Limit to display constans as signed
// ������ʾ�з��ŵĳ���
#define NEGLIMIT       (-16384)

// Base for pseudooperands
// α�������Ļ�ַ
#define PSEUDOOP       128

// Maximal length of text string
// �ı��ַ�������󳤶�
#define TEXTLEN        256

// Special command features.����������ܡ�
#define WW             0x01            // Bit W (size of operand)����������С��
#define SS             0x02            // Bit S (sign extention of immediate)�������ķ�����չ��
#define WS             0x03            // Bits W and S����������С�������ķ�����չ��
#define W3             0x08            // Bit W at position 3����������С��λ��3��
#define CC             0x10            // Conditional jump��������ת��
#define FF             0x20            // Forced 16-bit size��ǿ��16λ��С��
#define LL             0x40            // Conditional loop������ѭ����
#define PR             0x80            // Protected command���ܱ��������
#define WP             0x81            // I/O command with bit W����������С��I/O���

// All possible types of operands in 80x86. A bit more than you expected, he?�����������͡�
#define NNN            0               // No operand��û�в�������
#define REG            1               // Integer register in Reg field��REG�ֶ��е������Ĵ�����
#define RCM            2               // Integer register in command byte�������ֽ��е������Ĵ�����
#define RG4            3               // Integer 4-byte register in Reg field��REG�ֶ��е�����4�ֽڼĴ�����
#define RAC            4               // Accumulator (AL/AX/EAX, implicit)���ۼ�����
#define RAX            5               // AX (2-byte, implicit)
#define RDX            6               // DX (16-bit implicit port address)
#define RCL            7               // Implicit CL register (for shifts)
#define RS0            8               // Top of FPU stack (ST(0), implicit)
#define RST            9               // FPU register (ST(i)) in command byte
#define RMX            10              // MMX register MMx
#define R3D            11              // 3DNow! register MMx
#define MRG            12              // Memory/register in ModRM byte
#define MR1            13              // 1-byte memory/register in ModRM byte
#define MR2            14              // 2-byte memory/register in ModRM byte
#define MR4            15              // 4-byte memory/register in ModRM byte
#define RR4            16              // 4-byte memory/register (register only)
#define MR8            17              // 8-byte memory/MMX register in ModRM
#define RR8            18              // 8-byte MMX register only in ModRM
#define MRD            19              // 8-byte memory/3DNow! register in ModRM
#define RRD            20              // 8-byte memory/3DNow! (register only)
#define MRJ            21              // Memory/reg in ModRM as JUMP target
#define MMA            22              // Memory address in ModRM byte for LEA
#define MML            23              // Memory in ModRM byte (for LES)
#define MMS            24              // Memory in ModRM byte (as SEG:OFFS)
#define MM6            25              // Memory in ModRm (6-byte descriptor)
#define MMB            26              // Two adjacent memory locations (BOUND)
#define MD2            27              // Memory in ModRM (16-bit integer)
#define MB2            28              // Memory in ModRM (16-bit binary)
#define MD4            29              // Memory in ModRM byte (32-bit integer)
#define MD8            30              // Memory in ModRM byte (64-bit integer)
#define MDA            31              // Memory in ModRM byte (80-bit BCD)
#define MF4            32              // Memory in ModRM byte (32-bit float)
#define MF8            33              // Memory in ModRM byte (64-bit float)
#define MFA            34              // Memory in ModRM byte (80-bit float)
#define MFE            35              // Memory in ModRM byte (FPU environment)
#define MFS            36              // Memory in ModRM byte (FPU state)
#define MFX            37              // Memory in ModRM byte (ext. FPU state)
#define MSO            38              // Source in string op's ([ESI])
#define MDE            39              // Destination in string op's ([EDI])
#define MXL            40              // XLAT operand ([EBX+AL])
#define IMM            41              // Immediate data (8 or 16/32)
#define IMU            42              // Immediate unsigned data (8 or 16/32)
#define VXD            43              // VxD service
#define IMX            44              // Immediate sign-extendable byte
#define C01            45              // Implicit constant 1 (for shifts)
#define IMS            46              // Immediate byte (for shifts)
#define IM1            47              // Immediate byte
#define IM2            48              // Immediate word (ENTER/RET)
#define IMA            49              // Immediate absolute near data address
#define JOB            50              // Immediate byte offset (for jumps)
#define JOW            51              // Immediate full offset (for jumps)
#define JMF            52              // Immediate absolute far jump/call addr
#define SGM            53              // Segment register in ModRM byte
#define SCM            54              // Segment register in command byte
#define CRX            55              // Control register CRx
#define DRX            56              // Debug register DRx
// Pseudooperands (implicit operands, never appear in assembler commands). Must
// have index equal to or exceeding PSEUDOOP.
#define PRN            (PSEUDOOP+0)    // Near return address�������ص�ַ��
#define PRF            (PSEUDOOP+1)    // Far return address��Զ���ص�ַ��
#define PAC            (PSEUDOOP+2)    // Accumulator (AL/AX/EAX)
#define PAH            (PSEUDOOP+3)    // AH (in LAHF/SAHF commands)
#define PFL            (PSEUDOOP+4)    // Lower byte of flags (in LAHF/SAHF)
#define PS0            (PSEUDOOP+5)    // Top of FPU stack (ST(0))
#define PS1            (PSEUDOOP+6)    // ST(1)
#define PCX            (PSEUDOOP+7)    // CX/ECX
#define PDI            (PSEUDOOP+8)    // EDI (in MMX extentions)

// Errors detected during command disassembling.���������ڼ��⵽����
#define DAE_NOERR      0               // No error��û�з�������
#define DAE_BADCMD     1               // Unrecognized command���޷�ʶ������
#define DAE_CROSS      2               // Command crosses end of memory block������Խ���ڴ���ĩβ��
#define DAE_BADSEG     3               // Undefined segment register��δ����μĴ�����
#define DAE_MEMORY     4               // Register where only memory allowed�����������ڴ�ĵط�ע�᡿
#define DAE_REGISTER   5               // Memory where only register allowed��ֻ����Ĵ����Ĵ洢����
#define DAE_INTERN     6               // Internal error���ڲ�����

typedef unsigned char  uchar;          // Unsigned character (byte)
typedef unsigned short ushort;         // Unsigned short
typedef unsigned int   uint;           // Unsigned integer
typedef unsigned long  ulong;          // Unsigned long

typedef struct t_addrdec
{
	int            nDefSeg;
	unsigned char  *uszDescr;
} t_addrdec;

typedef struct t_cmddata
{
	ulong          ulMask;           // Mask for first 4 bytes of the command�������ǰ4���ֽڵ����롿
	ulong          ulCode;           // Compare masked bytes with this�������롿
	unsigned char  ucLen;            // Length of the main command code��������ĳ��ȡ�
	unsigned char  ucBits;           // Special bits within the command�������е�����λ��
	unsigned char  ucArg1;			 // Types of possible arguments1
	unsigned char  ucArg2;			 // Types of possible arguments2
	unsigned char  ucArg3;			 // Types of possible arguments3
	unsigned char  ucType;           // C_xxx + additional information��������Ϣ��
	unsigned char  *uszName;         // Symbolic name for this command��������ķ�������
} t_cmddata;

/*Initialized constant data structures used by all programs from assembler
 package. Contain names of register, register combinations or commands and
 their properties.*/
 /*�������������г���ʹ�õĳ�ʼ���������ݽṹ�������Ĵ������Ĵ�����ϻ���������ơ�*/
extern const unsigned char      *cuszRegName[3][9];
extern const unsigned char      *cuszSegName[8];
extern const unsigned char      *cuszSizeName[11];
extern const t_addrdec cstAddr16[8];
extern const t_addrdec cstAddr32[8];
extern const unsigned char      *cuszFPUName[9];
extern const unsigned char      *cuszMMXName[9];
extern const unsigned char      *cuszCRName[9];
extern const unsigned char      *cuszDRName[9];
extern const unsigned char      *cuszCondition[16];
extern const t_cmddata cstCmdData[];
extern const t_cmddata cstVxDCmd;
extern const t_cmddata cstDangerous[];

////////////////////////////////////////////////////////////////////////////////
//////////////////// ASSEMBLER, DISASSEMBLER AND EXPRESSIONS ///////////////////

#define MAXCMDSIZE     16              // Maximal length of 80x86 command
#define MAXCALSIZE     8               // Max length of CALL without prefixes
#define NMODELS        8               // Number of assembler search models

#define INT3           0xCC            // Code of 1-byte breakpoint
#define NOP            0x90            // Code of 1-byte NOP command
#define TRAPFLAG       0x00000100      // Trap flag in CPU flag register

#define REG_EAX        0               // Indexes of general-purpose registers
#define REG_ECX        1               // in t_reg.
#define REG_EDX        2
#define REG_EBX        3
#define REG_ESP        4
#define REG_EBP        5
#define REG_ESI        6
#define REG_EDI        7

#define SEG_UNDEF     -1		//δ����
// Indexes of segment/selector registers
// ��/ѡ��Ĵ���������
#define SEG_ES         0
#define SEG_CS         1
#define SEG_SS         2
#define SEG_DS         3
#define SEG_FS         4
#define SEG_GS         5

#define C_TYPEMASK     0xF0            // Mask for command type���������͵����롿
#define   C_CMD        0x00            // Ordinary instruction����ָͨ�
#define   C_PSH        0x10            // 1-word PUSH instruction��1�ֽ�����ָ�
#define   C_POP        0x20            // 1-word POP instruction��1�ֽ��Ƴ�ָ�
#define   C_MMX        0x30            // MMX instruction��MMXָ�
#define   C_FLT        0x40            // FPU instruction��FPUָ�
#define   C_JMP        0x50            // JUMP instruction��JUMPָ�
#define   C_JMC        0x60            // Conditional JUMP instruction������JUMPָ�
#define   C_CAL        0x70            // CALL instruction��CALLָ�
#define   C_RET        0x80            // RET instruction��RETָ�
#define   C_FLG        0x90            // Changes system flags������ϵͳ��־��
#define   C_RTF        0xA0            // C_JMP and C_FLG simultaneously��ͬʱʹ��������
#define   C_REP        0xB0            // Instruction with REPxx prefix�����ظ�ǰ׺��ָ�
#define   C_PRI        0xC0            // Privileged instruction����Ȩָ�
#define   C_DAT        0xD0            // Data (address) double word�����ݣ���ַ��˫�֡�
#define   C_NOW        0xE0            // 3DNow! instruction��3DNow!ָ�
#define   C_BAD        0xF0            // Unrecognized command���޷�ʶ������
#define C_RARE         0x08            // Rare command, seldom used in programs�����������
#define C_SIZEMASK     0x07            // MMX data size or special flag��MMX���ݴ�С�������־��
#define   C_EXPL       0x01            // (non-MMX) Specify explicit memory size��ָ����ʽ�ڴ��С��

#define C_DANGER95     0x01            // Command is dangerous under Win95/98����Σ�յ���Win95/98��
#define C_DANGER       0x03            // Command is dangerous everywhere��Win95/98���κεط���
#define C_DANGERLOCK   0x07            // Dangerous with LOCK prefix��Σ�յ�������ǰ׺��

#define DEC_TYPEMASK   0x1F            // Type of memory byte���ڴ��ֽ����͡�
#define   DEC_UNKNOWN  0x00            // Unknown type��δ֪���͡�
#define   DEC_BYTE     0x01            // Accessed as byte�����ֽ���ʽ���ʡ�
#define   DEC_WORD     0x02            // Accessed as short���Զ�������ʽ���ʡ�
#define   DEC_NEXTDATA 0x03            // Subsequent byte of code or data����������ݵĺ����ֽڡ�
#define   DEC_DWORD    0x04            // Accessed as long���Գ�������ʽ���ʡ�
#define   DEC_FLOAT4   0x05            // Accessed as float���Ե���������ʽ���ʡ�
#define   DEC_FWORD    0x06            // Accessed as descriptor/long pointer����������/��ָ����ʡ�
#define   DEC_FLOAT8   0x07            // Accessed as double����˫��������ʽ���ʡ�
#define   DEC_QWORD    0x08            // Accessed as 8-byte integer����8�ֽ����ͷ��ʡ�
#define   DEC_FLOAT10  0x09            // Accessed as long double���Գ�����˫���������ʡ�
#define   DEC_TBYTE    0x0A            // Accessed as 10-byte integer����10�ֽ�������ʽ���ʡ�
#define   DEC_STRING   0x0B            // Zero-terminated ASCII string�������β��ascii�ַ�����
#define   DEC_UNICODE  0x0C            // Zero-terminated UNICODE string�������β��unicode�ַ�����
#define   DEC_3DNOW    0x0D            // Accessed as 3Dnow operand����Ϊ3dNow���������ʡ�
#define   DEC_BYTESW   0x11            // Accessed as byte index to switch����Ϊ�ֽ����������Խ����л���
#define   DEC_NEXTCODE 0x13            // Subsequent byte of command������ĺ����ֽڡ�
#define   DEC_COMMAND  0x1D            // First byte of command������ĵ�һ���ֽڡ�
#define   DEC_JMPDEST  0x1E            // Jump destination����תĿ�ĵء�
#define   DEC_CALLDEST 0x1F            // Call (and maybe jump) destination��ת�ƣ���������ת��Ŀ�ĵء�
#define DEC_PROCMASK   0x60            // Procedure analysis�����������
#define   DEC_PROC     0x20            // Start of procedure������ʼ��
#define   DEC_PBODY    0x40            // Body of procedure���������塿
#define   DEC_PEND     0x60            // End of procedure�������β��
#define DEC_CHECKED    0x80            // Byte was analysed���ֽڷ�����

#define DECR_TYPEMASK  0x3F            // Type of register or memory���Ĵ�����洢�������͡�
#define   DECR_BYTE    0x21            // Byte register���ֽڼĴ�����
#define   DECR_WORD    0x22            // Short integer register���������Ĵ�����
#define   DECR_DWORD   0x24            // Long integer register���������Ĵ�����
#define   DECR_QWORD   0x28            // MMX register��MMX�Ĵ�����
#define   DECR_FLOAT10 0x29            // Floating-point register������Ĵ�����
#define   DECR_SEG     0x2A            // Segment register���μĴ�����
#define   DECR_3DNOW   0x2D            // 3Dnow! register��3Dnow!�Ĵ�����
#define DECR_ISREG     0x20            // Mask to check that operand is register�����������Ƿ�Ϊ�Ĵ��������롿

// �����ģʽ
#define DISASM_SIZE    0               // Determine command size only����ȷ�������С��
#define DISASM_DATA    1               // Determine size and analysis data��ȷ���ߴ�ͷ������ݡ�
#define DISASM_FILE    3               // Disassembly, no symbols������࣬�޷��š�
#define DISASM_CODE    4               // Full disassembly����ȫ����ࡿ

// Warnings issued by Disasm():��Disasm()�ľ��桿
#define DAW_FARADDR    0x0001          // Command is a far jump, call or return��������Զ��ת�����û򷵻ء�
#define DAW_SEGMENT    0x0002          // Command loads segment register��������ضμĴ�����
#define DAW_PRIV       0x0004          // Privileged command����Ȩ���
#define DAW_IO         0x0008          // I/O command��I/O���
#define DAW_SHIFT      0x0010          // Shift constant out of range 1..31����λ����������Χ��
#define DAW_PREFIX     0x0020          // Superfluous prefix������ǰ׺��
#define DAW_LOCK       0x0040          // Command has LOCK prefix�������������ǰ׺��
#define DAW_STACK      0x0080          // Unaligned stack operation��δ����Ķ�ջ������
#define DAW_DANGER95   0x1000          // May mess up Win95 if executed�����ִ�п��ܻ���������Win95��
#define DAW_DANGEROUS  0x3000          // May mess up any OS if executed�����ִ�п��ܻ�������OS��

typedef struct t_disasm					 // Results of disassembling�����������
{
	ulong          ulIp;                 // Instrucion pointer��ָ��ָ�롿
	unsigned char  uszDump[TEXTLEN];     // Hexadecimal dump of the command�������ʮ������ת����
	unsigned char  uszResult[TEXTLEN];   // Disassembled command��������
	unsigned char  uszComment[TEXTLEN];  // Brief comment�����������
	int            nCmdType;             // One of C_xxx�����͡�
	int            nMemType;             // Type of addressed variable in memory���ڴ���Ѱַ���������͡�
	int            nPrefix;              // Number of prefixes����ǰָ���ǰ׺��Ŀ��
	int            nIndexed;             // Address contains register(s)����ַ�����Ĵ�����
	ulong          ulJmpConst;           // Constant jump address��������ת��ַ��
	ulong          ulJmpTable;           // Possible address of switch table��������Ŀ��ܵ�ַ��
	ulong          ulAddrconst;          // Constant part of address����ַ�Ĺ̶����֡�
	ulong          ulImmConst;           // Immediate constant������������
	int            nZeroConst;           // Whether contains zero constant���㳣����
	int            nFixUpOffset;         // Possible offset of 32-bit fixups��32λ�����Ŀ���ƫ������
	int            nFixUpSize;           // Possible total size of fixups or 0�����ܵ��ܳߴ硿
	int            nError;               // Error while disassembling command�����������ʱ����
	int            nWarnings;            // Combination of DAW_xxx��������Ϣ��
} t_disasm;

typedef struct t_asmmodel			 	 // Model to search for assembler command��Ѱ�һ�������ģ�͡�
{
	unsigned char  uszCode[MAXCMDSIZE];  // Binary code�������ƴ��롿
	unsigned char  uszMask[MAXCMDSIZE];  // Mask for binary code (0: bit ignored)�������ƴ������롿
	int            nLength;              // Length of code, bytes (0: empty)�����볤�ȣ��ֽڡ�
	int            nJmpSize;             // Offset size if relative jump�������Ծʱ��ƫ�ƴ�С��
	int            nJmpOffset;           // Offset relative to IP�������IP��ƫ������
	int            nJmpPos;              // Position of jump offset in command�������е���תƫ��λ�á�
} t_asmmodel;

unique int       g_nIdeal;               // Force IDEAL decoding mode��ǿ���������ģʽ��
unique int       g_nLowerCase;           // Force lowercase display��ǿ��Сд��ʾ��
unique int       g_nTabArguments;        // Tab between mnemonic and arguments�����Ƿ��Ͳ���֮����Ʊ����
unique int       g_nExtraSpace;          // Extra space between arguments������֮��Ķ���ռ䡿
unique int       g_nPutDefSeg;           // Display default segments in listing�����б�����ʾĬ�϶Ρ�
unique int       g_nShowMemSize;         // Always show memory size��ʼ����ʾ�ڴ��С��
unique int       g_nShowNear;            // Show NEAR modifiers����ʾ�������޸�����
unique int       g_nShortStringCmds;     // Use short form of string commands��ʹ�ö̸�ʽ���ַ������
unique int       g_nSizeSens;            // How to decode size-sensitive mnemonics����ν����С�������Ƿ���
unique int       g_nSymbolic;            // Show symbolic addresses in disasm����disasm����ʾ���ŵ�ַ��
unique int       g_nFarCalls;            // Accept far calls, returns & addresses������Զ���á����غ͵�ַ��
unique int       g_nDecodeVxD;           // Decode VxD calls (Win95/98)������vxd���ã�win95/98����
unique int       g_nPrivileged;          // Accept privileged commands��������Ȩ���
unique int       g_nIOCommand;           // Accept I/O commands������I/O���
unique int       g_nBadShift;            // Accept shift out of range 1..31�����ܳ�����Χ�Ļ�����
unique int       g_nExtraPrefix;         // Accept superfluous prefixes�����ܶ����ǰ׺��
unique int       g_nLockedBus;           // Accept LOCK prefixes����������ǰ׺��
unique int       g_nStackAlign;          // Accept unaligned stack operations������δ����Ķ�ջ������
unique int       g_nIsWindowsNt;         // When checking for dangers, assume NT

#ifdef __cplusplus
extern "C"
{
#endif

	int Assemble(unsigned char *cmd, ulong ip,
		t_asmmodel *model,
		int attempt,
		int constsize,
		unsigned char *errtext);

	int Checkcondition(int code,
		ulong flags);

	int Decodeaddress(ulong addr,
		unsigned char *symb,
		int nsymb,
		unsigned char *comment);

	/*
	�����
	szHex����Ҫ����ɻ��ָ���ʮ�������ı�
	ulHexLen����Ҫ����ɻ��ָ���ʮ�������ı��ĳ���
	ulHexAddress����ǰ���ָ��ĵ�ַ���е���תָ����Ҫ�õ�
	pDisasmInfo���������Ϣ
	nDisasmMode�������ģʽ
	*/
	ulong Disasm(unsigned char *szHex,
		ulong ulHexLen,
		ulong ulHexAddress,
		t_disasm *pDisasmInfo,
		int nDisasmMode);

	ulong Disassembleback(unsigned char *block, ulong base,
		ulong size,
		ulong ip,
		int n);

	ulong Disassembleforward(unsigned char *block,
		ulong base,
		ulong size,
		ulong ip,
		int n);

	int Isfilling(ulong addr,
		unsigned char *data,
		ulong size,
		ulong align);

	int Print3dnow(unsigned char *s,
		unsigned char *f);

	int Printfloat10(unsigned char *s,
		long double ext);

	int Printfloat4(unsigned char *s,
		float f);

	int Printfloat8(unsigned char *s,
		double d);

#ifdef __cplusplus
}
#endif
