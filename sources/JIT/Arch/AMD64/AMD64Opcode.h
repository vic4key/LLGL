/*
 * AMD64Opcode.h
 * 
 * This file is part of the "LLGL" project (Copyright (c) 2015-2018 by Lukas Hermanns)
 * See "LICENSE.txt" for license information.
 */

#ifndef LLGL_AMD64_OPCODE_H
#define LLGL_AMD64_OPCODE_H


#include <LLGL/Export.h>
#include <vector>
#include <cstdint>


namespace LLGL
{

namespace JIT
{

/*
ModR/M => Mode Register/Memory
SIB    => Scale-Index-Base
mod = b11 => addressing via register
REX.W: 0 = default operand size, 1 = 64-bit operand size
REX.R: permitting access to 16 registers in ModR/M <reg> field
REX.X: permitting access to 16 registers in SIB <index> field
REX.B: permitting access to 16 registers in ModR/M <r/m> field
-------------------------------------------------------------------------------------
| Field: |   REX    |     Opcode     | ModR/M |   SIB    | Displacement | Immediate |
|--------|----------|----------------|--------|----------|--------------|-----------|
| Size:  |   0-1    |      1-3       |  0-1   |   0-1    |   0,1,2,4    |  0,1,2,4  |
|--------|----------|----------------|--------|----------|--------------|-----------|
| Bits:  | 0100WRXB | <op>           | mod: 2 | scale: 2 |              |           |
|        |          | 0x0F <op>      | reg: 3 | index: 3 |              |           |
|        |          | 0x0F 0x38 <op> | r/m: 3 | base:  3 |              |           |
|        |          | 0x0F 0x3A <op> |        |          |              |           |
-------------------------------------------------------------------------------------
*/

enum REXBits : std::uint8_t
{
    REX_Prefix  = 0x40,
    REX_W       = 0x08,
    REX_R       = 0x04,
    REX_B       = 0x01,
};

enum ModRMBits : std::uint8_t
{
    Operand_Mod01 = 0x40, // ?
    Operand_Mod10 = 0x80, // ?
    Operand_Mod11 = 0xC0, // ?
};

enum OpcodePrefix : std::uint8_t
{
    OpcodePrefix_2  = 0x0F,
    OpcodePrefix_3a = 0x38,
    OpcodePrefix_3b = 0x3A,
};

enum Opcode : std::uint8_t
{
    Opcode_PushImm      = 0x68, // 68 id
    Opcode_PushImm8     = 0x6A, // 6A ib
    Opcode_PushReg      = 0x50,
    Opcode_PopReg       = 0x58, // 58 +rq
    Opcode_AddImm       = 0x81, // 81 /0 id
    Opcode_SubImm       = 0x81, // 81 /5 id
    Opcode_DivReg       = 0xF7, // F7 /6
    Opcode_MovRegImm8   = 0xB0, // B0 +rb ib
    Opcode_MovRegImm    = 0xB8, // [REX.W] B8 +rd id
    Opcode_MovMemImm    = 0xC7, // C7 /0 id
    Opcode_MovMemReg    = 0x89, // 89 /r
    Opcode_RetNear      = 0xC3, // C3
    Opcode_RetFar       = 0xCB, // CB
    Opcode_RetNearImm16 = 0xC2, // C2 iw
    Opcode_RetFarImm16  = 0xCA, // CA iw
    Opcode_CallNear     = 0x10, // /2 => 00 010 000 => 0x10
};


} // /namespace JIT

} // /namespace LLGL


#endif



// ================================================================================