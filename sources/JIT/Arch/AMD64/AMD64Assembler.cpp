/*
 * AMD64Assembler.cpp
 * 
 * This file is part of the "LLGL" project (Copyright (c) 2015-2018 by Lukas Hermanns)
 * See "LICENSE.txt" for license information.
 */

#include "AMD64Assembler.h"
#include "AMD64Opcode.h"
#include <limits.h>


namespace LLGL
{

namespace JIT
{


/*
 * Internal members
 */

/*
List of registers that are used for the first couple of arguments.
Note the difference between Microsoft and Unix x64 calling conventions.
see https://en.wikipedia.org/wiki/X86_calling_conventions#List_of_x86_calling_conventions
*/
#ifdef _WIN32

// Microsoft x64 calling convention (Windows)
static const Reg g_amd64IntParams[] = { Reg::RCX, Reg::RDX, Reg::R8, Reg::R9 };
static const Reg g_amd64FltParams[] = { Reg::XMM0, Reg::XMM1, Reg::XMM2, Reg::XMM3 };

#else

// System V AMD64 ABI (Solaris, Linux, BSD, macOS)
static const Reg g_amd64IntParams[] = { Reg::RDI, Reg::RSI, Reg::RDX, Reg::RCX, Reg::R8, Reg::R9 };
static const Reg g_amd64FltParams[] = { Reg::XMM0, Reg::XMM1, Reg::XMM2, Reg::XMM3, Reg::XMM4, Reg::XMM5, Reg::XMM6, Reg::XMM7 };

#endif

static const std::size_t g_amd64IntParamsCount = sizeof(g_amd64IntParams)/sizeof(g_amd64IntParams[0]);
static const std::size_t g_amd64FltParamsCount = sizeof(g_amd64FltParams)/sizeof(g_amd64FltParams[0]);


/*
 * AMD64Assembler class
 */

void AMD64Assembler::Begin()
{
    PushReg(Reg::RBP);
    MovReg(Reg::RBP, Reg::RSP);
    SubImm32(Reg::RSP, 0x20);//???
    
    #if 0//TEST: force interruption
    MovRegImm32(Reg::RAX, 0);
    MovRegImm32(Reg::RDX, 0);
    DivReg(Reg::RAX);
    
    MovMemImm32(Reg::RBX, 0x999, -0xF);
    MovMemImm32(Reg::RBP, 0x999, -0xF);
    MovMemReg(Reg::RBX, Reg::RCX, 0x1A);
    #endif
}

void AMD64Assembler::End()
{
    AddImm32(Reg::RSP, 0x20);//???
    PopReg(Reg::RBP);
    RetNear();
}

void AMD64Assembler::WriteFuncCall(const void* addr, const JITCallConv conv, bool farCall)
{
    const auto& args = GetArgs();
    
    /* Move first couple of arguments into registers */
    std::size_t numIntRegs = 0, numFltRegs = 0;
    std::size_t lastInt = ~0, lastFlt = ~0;
    std::size_t num = args.size();
    
    for (std::size_t i = 0; i < num; ++i)
    {
        const auto& arg = args[i];
        
        /* Determine destination register for argument */
        Reg dstReg = Reg::R10;
        bool isFloat = IsFloat(arg.type);
        
        if (isFloat && numFltRegs < g_amd64FltParamsCount)
        {
            dstReg  = g_amd64FltParams[numFltRegs++];
            lastFlt = i;
        }
        else if (!isFloat && numIntRegs < g_amd64IntParamsCount)
        {
            dstReg  = g_amd64IntParams[numIntRegs++];
            lastInt = i;
        }
        else
            break;
        
        /* Move value into destination register */
        if (dstReg >= Reg::R8 && dstReg <= Reg::R15)
        {
            /* R8-R15 registers are only supported for 64-bit operand size */
            MovRegImm64(dstReg, arg.value.i64);
        }
        else
        {
            switch (arg.type)
            {
                case ArgType::Byte:
                    MovRegImm32(dstReg, arg.value.i8);
                    break;
                case ArgType::Word:
                    MovRegImm32(dstReg, arg.value.i16);
                    break;
                case ArgType::DWord:
                    MovRegImm32(dstReg, arg.value.i32);
                    break;
                case ArgType::QWord:
                    MovRegImm64(dstReg, arg.value.i64);
                    break;
                case ArgType::Ptr:
                    MovRegImm64(dstReg, arg.value.i64);
                    break;
                case ArgType::Float:
                    //TODO...
                    break;
                case ArgType::Double:
                    //TODO...
                    break;
            }
        }
    }
    
    /* Push remaining arguments onto stack */
    for (std::size_t i = 0; i < num; ++i)
    {
        auto iRev = num - i - 1u;
        const auto& arg = args[iRev];
        
        /* Check if argument has already been moved into a register */
        bool isFloat = IsFloat(arg.type);
        
        if ( (  isFloat && iRev == lastFlt ) ||
             ( !isFloat && iRev == lastInt ) )
        {
            break;
        }

        /* Push argument onto stack */
        switch (arg.type)
        {
            case ArgType::Byte:
                PushImm8(arg.value.i8);
                break;
            case ArgType::Word:
                PushImm16(arg.value.i16);
                break;
            case ArgType::DWord:
            case ArgType::Float:
                PushImm32(arg.value.i32);
                break;
            case ArgType::QWord:
            case ArgType::Ptr:
            case ArgType::Double:
                //MovRegImm64(Reg::R10, arg.value.i64);
                //PushReg(Reg::R10);
                break;
        }
    }
    
    /* Write 'call' instruction */
    MovRegImm64(Reg::RAX, reinterpret_cast<std::uint64_t>(addr));
    CallNear(Reg::RAX);
}


/*
 * ======= Private: =======
 */

bool AMD64Assembler::IsLittleEndian() const
{
    return true;
}

void AMD64Assembler::WriteREXOpt(const Reg reg)
{
    std::uint8_t prefix = 0;
    
    if (Is64Reg(reg))
    {
        prefix |= REX_W;
        if (reg >= Reg::R8)
            prefix |= REX_B;
    }
    
    if (prefix != 0)
        WriteByte(REX_Prefix | prefix);
}

/* ----- PUSH ----- */

void AMD64Assembler::PushReg(const Reg reg)
{
    WriteByte(Opcode_PushReg | RegByte(reg));
}

void AMD64Assembler::PushImm8(std::uint8_t byte)
{
    WriteByte(Opcode_PushImm8);
    WriteByte(byte);
}

void AMD64Assembler::PushImm16(std::uint16_t word)
{
    PushImm32(word);
}

void AMD64Assembler::PushImm32(std::uint32_t dword)
{
    WriteByte(Opcode_PushImm);
    WriteDWord(dword);
}

/* ----- POP ----- */

void AMD64Assembler::PopReg(const Reg reg)
{
    WriteByte(Opcode_PopReg | RegByte(reg));
}

/* ----- MOV ----- */

void AMD64Assembler::MovReg(const Reg dst, const Reg src)
{
    WriteREXOpt(dst);
    WriteByte(Opcode_MovMemReg);
    WriteByte(Operand_Mod11 | RegByte(src) << 3 | RegByte(dst));
}

void AMD64Assembler::MovRegImm32(const Reg reg, std::uint32_t dword)
{
    WriteByte(Opcode_MovRegImm | RegByte(reg));
    WriteDWord(dword);
}

void AMD64Assembler::MovRegImm64(const Reg reg, std::uint64_t qword)
{
    WriteREXOpt(reg);
    WriteByte(Opcode_MovRegImm | RegByte(reg));
    WriteQWord(qword);
}

void AMD64Assembler::MovMemImm32(const Reg reg, std::uint32_t dword, std::uint32_t offset)
{
    #ifdef LLGL_DEBUG
    //if (reg == Reg::RSP)
    //    Error("invalid use of %RSP register in MOV instruction");
    #endif
    
    std::uint8_t disp = 0;
    if (offset > 0)
    {
        if (offset > UCHAR_MAX)
            disp |= Operand_Mod10; // disp32
        else
            disp |= Operand_Mod01; // disp8
    }
    
    /* Write opcode */
    WriteREXOpt(reg);
    WriteByte(Opcode_MovMemImm);
    WriteByte(disp | RegByte(reg));
    
    /* Write optional displacement */
    if (offset > 0)
    {
        if (offset > UCHAR_MAX)
            WriteDWord(offset);
        else
            WriteByte(static_cast<std::uint8_t>(offset));
    }
    
    /* Write immediate value */
    WriteDWord(dword);
}

void AMD64Assembler::MovMemReg(const Reg dstMemReg, const Reg srcReg, std::uint32_t offset)
{
    std::uint8_t disp = 0;
    if (offset > 0)
    {
        if (offset > UCHAR_MAX)
            disp |= Operand_Mod10; // disp32
        else
            disp |= Operand_Mod01; // disp8
    }
    
    /* Write opcode */
    WriteREXOpt(srcReg);
    WriteByte(Opcode_MovMemReg);
    WriteByte(disp | RegByte(srcReg) << 3 | RegByte(dstMemReg));
    
    /* Write optional displacement */
    if (offset > 0)
    {
        if (offset > UCHAR_MAX)
            WriteDWord(offset);
        else
            WriteByte(static_cast<std::uint8_t>(offset));
    }
}

/* ----- ADD ----- */

void AMD64Assembler::AddImm32(const Reg dst, std::uint32_t dword)
{
    WriteREXOpt(dst);
    WriteByte(Opcode_AddImm);
    WriteByte(Operand_Mod11 | RegByte(dst));
    WriteDWord(dword);
}

/* ----- SUB ----- */

void AMD64Assembler::SubImm32(const Reg dst, std::uint32_t dword)
{
    WriteREXOpt(dst);
    WriteByte(Opcode_SubImm);
    WriteByte(Operand_Mod11 | (5u << 3) | RegByte(dst));
    WriteDWord(dword);
}

/* ----- DIV ----- */

void AMD64Assembler::DivReg(const Reg src)
{
    WriteREXOpt(src);
    WriteByte(Opcode_DivReg);
    WriteByte(Operand_Mod11 | (6u << 3) | RegByte(src));
}

/* ----- CALL ----- */

void AMD64Assembler::CallNear(const Reg reg)
{
    WriteByte(0xFF);
    WriteByte(Opcode_CallNear | Operand_Mod11 | RegByte(reg));
}

/* ----- RET ----- */

void AMD64Assembler::RetNear(std::uint16_t word)
{
    if (word > 0)
    {
        WriteByte(Opcode_RetNearImm16);
        WriteWord(word);
    }
    else
        WriteByte(Opcode_RetNear);
}

void AMD64Assembler::RetFar(std::uint16_t word)
{
    if (word > 0)
    {
        WriteByte(Opcode_RetFarImm16);
        WriteWord(word);
    }
    else
        WriteByte(Opcode_RetFar);
}


} // /namespace JIT

} // /namespace LLGL



// ================================================================================