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
// 限制显示有符号的常量
#define NEGLIMIT       (-16384)

// Base for pseudooperands
// 伪操作数的基址
#define PSEUDOOP       128

// Maximal length of text string
// 文本字符串的最大长度
#define TEXTLEN        256

// Special command features.【特殊命令功能】
#define WW             0x01            // Bit W (size of operand)【操作数大小】
#define SS             0x02            // Bit S (sign extention of immediate)【立即的符号扩展】
#define WS             0x03            // Bits W and S【操作数大小和立即的符号扩展】
#define W3             0x08            // Bit W at position 3【操作数大小在位置3】
#define CC             0x10            // Conditional jump【条件跳转】
#define FF             0x20            // Forced 16-bit size【强制16位大小】
#define LL             0x40            // Conditional loop【条件循环】
#define PR             0x80            // Protected command【受保护的命令】
#define WP             0x81            // I/O command with bit W【操作数大小的I/O命令】

// All possible types of operands in 80x86. A bit more than you expected, he?【操作数类型】
#define NNN            0               // No operand【没有操作数】
#define REG            1               // Integer register in Reg field【REG字段中的整数寄存器】
#define RCM            2               // Integer register in command byte【命令字节中的整数寄存器】
#define RG4            3               // Integer 4-byte register in Reg field【REG字段中的整数4字节寄存器】
#define RAC            4               // Accumulator (AL/AX/EAX, implicit)【累加器】
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
#define PRN            (PSEUDOOP+0)    // Near return address【近返回地址】
#define PRF            (PSEUDOOP+1)    // Far return address【远返回地址】
#define PAC            (PSEUDOOP+2)    // Accumulator (AL/AX/EAX)
#define PAH            (PSEUDOOP+3)    // AH (in LAHF/SAHF commands)
#define PFL            (PSEUDOOP+4)    // Lower byte of flags (in LAHF/SAHF)
#define PS0            (PSEUDOOP+5)    // Top of FPU stack (ST(0))
#define PS1            (PSEUDOOP+6)    // ST(1)
#define PCX            (PSEUDOOP+7)    // CX/ECX
#define PDI            (PSEUDOOP+8)    // EDI (in MMX extentions)

// Errors detected during command disassembling.【命令反汇编期间检测到错误】
#define DAE_NOERR      0               // No error【没有发生错误】
#define DAE_BADCMD     1               // Unrecognized command【无法识别的命令】
#define DAE_CROSS      2               // Command crosses end of memory block【命令越过内存块的末尾】
#define DAE_BADSEG     3               // Undefined segment register【未定义段寄存器】
#define DAE_MEMORY     4               // Register where only memory allowed【仅在允许内存的地方注册】
#define DAE_REGISTER   5               // Memory where only register allowed【只允许寄存器的存储器】
#define DAE_INTERN     6               // Internal error【内部错误】

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
	ulong          ulMask;           // Mask for first 4 bytes of the command【命令的前4个字节的掩码】
	ulong          ulCode;           // Compare masked bytes with this【操作码】
	unsigned char  ucLen;            // Length of the main command code【操作码的长度】
	unsigned char  ucBits;           // Special bits within the command【命令中的特殊位】
	unsigned char  ucArg1;			 // Types of possible arguments1
	unsigned char  ucArg2;			 // Types of possible arguments2
	unsigned char  ucArg3;			 // Types of possible arguments3
	unsigned char  ucType;           // C_xxx + additional information【附加信息】
	unsigned char  *uszName;         // Symbolic name for this command【此命令的符号名】
} t_cmddata;

/*Initialized constant data structures used by all programs from assembler
 package. Contain names of register, register combinations or commands and
 their properties.*/
 /*【汇编程序中所有程序使用的初始化常量数据结构。包含寄存器、寄存器组合或命令的名称】*/
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

#define SEG_UNDEF     -1		//未定义
// Indexes of segment/selector registers
// 段/选择寄存器的索引
#define SEG_ES         0
#define SEG_CS         1
#define SEG_SS         2
#define SEG_DS         3
#define SEG_FS         4
#define SEG_GS         5

#define C_TYPEMASK     0xF0            // Mask for command type【命令类型的掩码】
#define   C_CMD        0x00            // Ordinary instruction【普通指令】
#define   C_PSH        0x10            // 1-word PUSH instruction【1字节推送指令】
#define   C_POP        0x20            // 1-word POP instruction【1字节推出指令】
#define   C_MMX        0x30            // MMX instruction【MMX指令】
#define   C_FLT        0x40            // FPU instruction【FPU指令】
#define   C_JMP        0x50            // JUMP instruction【JUMP指令】
#define   C_JMC        0x60            // Conditional JUMP instruction【条件JUMP指令】
#define   C_CAL        0x70            // CALL instruction【CALL指令】
#define   C_RET        0x80            // RET instruction【RET指令】
#define   C_FLG        0x90            // Changes system flags【更改系统标志】
#define   C_RTF        0xA0            // C_JMP and C_FLG simultaneously【同时使用两个】
#define   C_REP        0xB0            // Instruction with REPxx prefix【带重复前缀的指令】
#define   C_PRI        0xC0            // Privileged instruction【特权指令】
#define   C_DAT        0xD0            // Data (address) double word【数据（地址）双字】
#define   C_NOW        0xE0            // 3DNow! instruction【3DNow!指令】
#define   C_BAD        0xF0            // Unrecognized command【无法识别的命令】
#define C_RARE         0x08            // Rare command, seldom used in programs【罕见的命令】
#define C_SIZEMASK     0x07            // MMX data size or special flag【MMX数据大小或特殊标志】
#define   C_EXPL       0x01            // (non-MMX) Specify explicit memory size【指定显式内存大小】

#define C_DANGER95     0x01            // Command is dangerous under Win95/98【是危险的在Win95/98】
#define C_DANGER       0x03            // Command is dangerous everywhere【Win95/98在任何地方】
#define C_DANGERLOCK   0x07            // Dangerous with LOCK prefix【危险的有锁定前缀】

#define DEC_TYPEMASK   0x1F            // Type of memory byte【内存字节类型】
#define   DEC_UNKNOWN  0x00            // Unknown type【未知类型】
#define   DEC_BYTE     0x01            // Accessed as byte【以字节形式访问】
#define   DEC_WORD     0x02            // Accessed as short【以短整型形式访问】
#define   DEC_NEXTDATA 0x03            // Subsequent byte of code or data【代码或数据的后续字节】
#define   DEC_DWORD    0x04            // Accessed as long【以长整型形式访问】
#define   DEC_FLOAT4   0x05            // Accessed as float【以单浮点型形式访问】
#define   DEC_FWORD    0x06            // Accessed as descriptor/long pointer【以描述符/长指针访问】
#define   DEC_FLOAT8   0x07            // Accessed as double【以双浮点型形式访问】
#define   DEC_QWORD    0x08            // Accessed as 8-byte integer【以8字节整型访问】
#define   DEC_FLOAT10  0x09            // Accessed as long double【以长整型双浮点数访问】
#define   DEC_TBYTE    0x0A            // Accessed as 10-byte integer【以10字节整型形式访问】
#define   DEC_STRING   0x0B            // Zero-terminated ASCII string【以零结尾的ascii字符串】
#define   DEC_UNICODE  0x0C            // Zero-terminated UNICODE string【以零结尾的unicode字符串】
#define   DEC_3DNOW    0x0D            // Accessed as 3Dnow operand【作为3dNow操作数访问】
#define   DEC_BYTESW   0x11            // Accessed as byte index to switch【作为字节索引访问以进行切换】
#define   DEC_NEXTCODE 0x13            // Subsequent byte of command【命令的后续字节】
#define   DEC_COMMAND  0x1D            // First byte of command【命令的第一个字节】
#define   DEC_JMPDEST  0x1E            // Jump destination【跳转目的地】
#define   DEC_CALLDEST 0x1F            // Call (and maybe jump) destination【转移（可能是跳转）目的地】
#define DEC_PROCMASK   0x60            // Procedure analysis【程序分析】
#define   DEC_PROC     0x20            // Start of procedure【程序开始】
#define   DEC_PBODY    0x40            // Body of procedure【程序主体】
#define   DEC_PEND     0x60            // End of procedure【程序结尾】
#define DEC_CHECKED    0x80            // Byte was analysed【字节分析】

#define DECR_TYPEMASK  0x3F            // Type of register or memory【寄存器或存储器的类型】
#define   DECR_BYTE    0x21            // Byte register【字节寄存器】
#define   DECR_WORD    0x22            // Short integer register【短整数寄存器】
#define   DECR_DWORD   0x24            // Long integer register【长整数寄存器】
#define   DECR_QWORD   0x28            // MMX register【MMX寄存器】
#define   DECR_FLOAT10 0x29            // Floating-point register【浮点寄存器】
#define   DECR_SEG     0x2A            // Segment register【段寄存器】
#define   DECR_3DNOW   0x2D            // 3Dnow! register【3Dnow!寄存器】
#define DECR_ISREG     0x20            // Mask to check that operand is register【检查操作数是否为寄存器的掩码】

// 反汇编模式
#define DISASM_SIZE    0               // Determine command size only【仅确定命令大小】
#define DISASM_DATA    1               // Determine size and analysis data【确定尺寸和分析数据】
#define DISASM_FILE    3               // Disassembly, no symbols【反汇编，无符号】
#define DISASM_CODE    4               // Full disassembly【完全反汇编】

// Warnings issued by Disasm():【Disasm()的警告】
#define DAW_FARADDR    0x0001          // Command is a far jump, call or return【命令是远跳转、调用或返回】
#define DAW_SEGMENT    0x0002          // Command loads segment register【命令加载段寄存器】
#define DAW_PRIV       0x0004          // Privileged command【特权命令】
#define DAW_IO         0x0008          // I/O command【I/O命令】
#define DAW_SHIFT      0x0010          // Shift constant out of range 1..31【移位常数超出范围】
#define DAW_PREFIX     0x0020          // Superfluous prefix【多余前缀】
#define DAW_LOCK       0x0040          // Command has LOCK prefix【命令具有锁定前缀】
#define DAW_STACK      0x0080          // Unaligned stack operation【未对齐的堆栈操作】
#define DAW_DANGER95   0x1000          // May mess up Win95 if executed【如果执行可能会有问题在Win95】
#define DAW_DANGEROUS  0x3000          // May mess up any OS if executed【如果执行可能会有问题OS】

typedef struct t_disasm					 // Results of disassembling【反汇编结果】
{
	ulong          ulIp;                 // Instrucion pointer【指令指针】
	unsigned char  uszDump[TEXTLEN];     // Hexadecimal dump of the command【命令的十六进制转储】
	unsigned char  uszResult[TEXTLEN];   // Disassembled command【汇编命令】
	unsigned char  uszComment[TEXTLEN];  // Brief comment【简短描述】
	int            nCmdType;             // One of C_xxx【类型】
	int            nMemType;             // Type of addressed variable in memory【内存中寻址变量的类型】
	int            nPrefix;              // Number of prefixes【当前指令的前缀数目】
	int            nIndexed;             // Address contains register(s)【地址包含寄存器】
	ulong          ulJmpConst;           // Constant jump address【常数跳转地址】
	ulong          ulJmpTable;           // Possible address of switch table【交换表的可能地址】
	ulong          ulAddrconst;          // Constant part of address【地址的固定部分】
	ulong          ulImmConst;           // Immediate constant【立即常数】
	int            nZeroConst;           // Whether contains zero constant【零常数】
	int            nFixUpOffset;         // Possible offset of 32-bit fixups【32位修正的可能偏移量】
	int            nFixUpSize;           // Possible total size of fixups or 0【可能的总尺寸】
	int            nError;               // Error while disassembling command【反汇编命令时出错】
	int            nWarnings;            // Combination of DAW_xxx【出错信息】
} t_disasm;

typedef struct t_asmmodel			 	 // Model to search for assembler command【寻找汇编命令的模型】
{
	unsigned char  uszCode[MAXCMDSIZE];  // Binary code【二进制代码】
	unsigned char  uszMask[MAXCMDSIZE];  // Mask for binary code (0: bit ignored)【二进制代码掩码】
	int            nLength;              // Length of code, bytes (0: empty)【代码长度，字节】
	int            nJmpSize;             // Offset size if relative jump【相对跳跃时的偏移大小】
	int            nJmpOffset;           // Offset relative to IP【相对于IP的偏移量】
	int            nJmpPos;              // Position of jump offset in command【命令中的跳转偏移位置】
} t_asmmodel;

unique int       g_nIdeal;               // Force IDEAL decoding mode【强制理想解码模式】
unique int       g_nLowerCase;           // Force lowercase display【强制小写显示】
unique int       g_nTabArguments;        // Tab between mnemonic and arguments【助记符和参数之间的制表符】
unique int       g_nExtraSpace;          // Extra space between arguments【参数之间的额外空间】
unique int       g_nPutDefSeg;           // Display default segments in listing【在列表中显示默认段】
unique int       g_nShowMemSize;         // Always show memory size【始终显示内存大小】
unique int       g_nShowNear;            // Show NEAR modifiers【显示附近的修改器】
unique int       g_nShortStringCmds;     // Use short form of string commands【使用短格式的字符串命令】
unique int       g_nSizeSens;            // How to decode size-sensitive mnemonics【如何解码大小敏感助记符】
unique int       g_nSymbolic;            // Show symbolic addresses in disasm【在disasm中显示符号地址】
unique int       g_nFarCalls;            // Accept far calls, returns & addresses【接受远调用、返回和地址】
unique int       g_nDecodeVxD;           // Decode VxD calls (Win95/98)【解码vxd调用（win95/98）】
unique int       g_nPrivileged;          // Accept privileged commands【接受特权命令】
unique int       g_nIOCommand;           // Accept I/O commands【接受I/O命令】
unique int       g_nBadShift;            // Accept shift out of range 1..31【接受超出范围的换档】
unique int       g_nExtraPrefix;         // Accept superfluous prefixes【接受多余的前缀】
unique int       g_nLockedBus;           // Accept LOCK prefixes【接受锁定前缀】
unique int       g_nStackAlign;          // Accept unaligned stack operations【接受未对齐的堆栈操作】
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
	反汇编
	szHex：需要翻译成汇编指令的十六进制文本
	ulHexLen：需要翻译成汇编指令的十六进制文本的长度
	ulHexAddress：当前汇编指令的地址，有的跳转指令需要用到
	pDisasmInfo：反汇编信息
	nDisasmMode：反汇编模式
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
