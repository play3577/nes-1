#include <iostream>

#include "CPU.hpp"
#include "NES.hpp"

//*********************************************************************
// The Register template
//*********************************************************************

template <Register R>
CPU::RegisterAccess<R>::RegisterAccess( CPU::Registers& registers ) :
	registers(registers)
{
}

template <Register R>
CPU::RegisterAccess<R>& CPU::RegisterAccess<R>::operator = ( uint8_t value )
{
	switch(R)
	{
	case REGISTER_A:
		registers.a = value;
		break;
	case REGISTER_X:
		registers.x = value;
		break;
	case REGISTER_Y:
		registers.y = value;
		break;
	case REGISTER_S:
		registers.s = value;
		break;
	case REGISTER_PC:
		registers.pc.l = value;
		break;
	case REGISTER_P:
		registers.p.raw = value;
		break;
	}

	return (*this);
}

template <Register R>
CPU::RegisterAccess<R>::operator uint8_t()
{
	switch(R)
	{
	case REGISTER_A:
		return registers.a;
	case REGISTER_X:
		return registers.x;
	case REGISTER_Y:
		return registers.y;
	case REGISTER_S:
		return registers.s;
	case REGISTER_PC:
		return registers.pc.l;
	case REGISTER_P:
		return registers.p.raw;
	}
}

//*********************************************************************
// Opcode tempaltes
//*********************************************************************

template <MemoryAddressingMode M>
void CPU::opADC()
{
	MemoryAccess src = getMemory<M>();
	uint16_t temp = src + registers.a + (registers.p.carry ? 1 : 0);
	setZero(temp & 0xff);
	if( registers.p.decimal )
	{
		if( ((registers.a & 0xf) + (src & 0xf) + (registers.p.carry ? 1 : 0)) > 9 )
		{
			temp += 6;
		}
		setSign(temp);
		registers.p.overflow = (!((registers.a ^ src) & 0x80) && ((registers.a ^ temp) & 0x80));
		if( temp > 0x99 )
		{
			temp += 96;
		}
		registers.p.carry = (temp > 0x99);
	}
	else
	{
		setSign(temp);
		registers.p.overflow = (!((registers.a ^ src) & 0x80) && ((registers.a ^ temp) & 0x80));
		registers.p.carry = (temp > 0xff);
	}
	registers.a = (uint8_t)temp;
}

//*********************************************************************
// The CPU class
//*********************************************************************

CPU::CPU( NES& nes ) :
	nes(nes)
{
	// Initialize the opcode table
	for( int i = 0; i < 0x100; i++ )
	{
		opcodes[i] = nullptr;
	}

	// ADC
	opcodes[0x69] = &CPU::opADC<MEM_IMMEDIATE>;
	opcodes[0x65] = &CPU::opADC<MEM_ZERO_PAGE_ABSOLUTE>;
	opcodes[0x75] = &CPU::opADC<MEM_ZERO_PAGE_INDEXED_X>;
	opcodes[0x60] = &CPU::opADC<MEM_ABSOLUTE>;
	opcodes[0x70] = &CPU::opADC<MEM_INDEXED_X>;
	opcodes[0x79] = &CPU::opADC<MEM_INDEXED_Y>;
	opcodes[0x61] = &CPU::opADC<MEM_PRE_INDEXED_INDIRECT>;
	opcodes[0x71] = &CPU::opADC<MEM_POST_INDEXED_INDIRECT>;
}

uint8_t CPU::getImmediate8()
{
	uint8_t value = nes.getMemory().readByte(registers.pc.w);
	registers.pc.w++;

	return value;
}

uint16_t CPU::getImmediate16()
{
	uint16_t value = nes.getMemory().readWord(registers.pc.w);
	registers.pc.w += 2;

	return value;
}

template <MemoryAddressingMode M>
MemoryAccess CPU::getMemory()
{
	switch(M)
	{
	case MEM_IMMEDIATE:
		{
			uint16_t pc = registers.pc.w;
			registers.pc.w++;
			return MemoryAccess(nes.getMemory(), pc);
		}
	case MEM_ABSOLUTE:
		return MemoryAccess(nes.getMemory(), getImmediate16());
	case MEM_ZERO_PAGE_ABSOLUTE:
		return MemoryAccess(nes.getMemory(), getImmediate8());
	case MEM_INDEXED_X:
		return MemoryAccess(nes.getMemory(), getImmediate16() + registers.x);
	case MEM_INDEXED_Y:
		return MemoryAccess(nes.getMemory(), getImmediate16() + registers.y);
	case MEM_ZERO_PAGE_INDEXED_X:
		return MemoryAccess(nes.getMemory(), getImmediate8() + registers.x);
	case MEM_ZERO_PAGE_INDEXED_Y:
		return MemoryAccess(nes.getMemory(), getImmediate8() + registers.y);
	case MEM_INDIRECT:
		return MemoryAccess(nes.getMemory(), nes.getMemory().readWord(getImmediate16()));
	case MEM_PRE_INDEXED_INDIRECT:
		return MemoryAccess(nes.getMemory(), nes.getMemory().readWord(getImmediate8() + registers.x));
	case MEM_POST_INDEXED_INDIRECT:
		return MemoryAccess(nes.getMemory(), nes.getMemory().readWord(getImmediate8()) + registers.y);
	case MEM_RELATIVE:
		{
			uint16_t address;
			uint16_t offset = getImmediate8();
			if( offset < 0x80 )
			{
				address = getImmediate8() + offset;
			}
			else
			{
				address = getImmediate8() + offset - 0x100;
			}
			return MemoryAccess(nes.getMemory(), address);
		}
	}
}

void CPU::setSign( uint8_t value )
{
	registers.p.sign = ((value & BIT_7) ? 1 : 0);
}

void CPU::setZero( uint8_t value )
{
	registers.p.zero = ((value == 0) ? 1 : 0);
}
