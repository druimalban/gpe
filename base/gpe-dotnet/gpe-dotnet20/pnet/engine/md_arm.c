/*
 * md_arm.c - Machine-dependent definitions for ARM.
 *
 * Copyright (C) 2003  Southern Storm Software, Pty Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "cvm_config.h"
#include "md_arm.h"

#ifdef	__cplusplus
extern	"C" {
#endif

#ifdef CVM_ARM

arm_inst_ptr _arm_mov_reg_imm(arm_inst_ptr inst, int reg, int value)
{
	/* Handle bytes in various positions */
	if((value & 0x000000FF) == value)
	{
		arm_mov_reg_imm8(inst, reg, value);
		return inst;
	}
	else if((value & 0x0000FF00) == value)
	{
		arm_mov_reg_imm8_rotate(inst, reg, (value >> 8), 12);
		return inst;
	}
	else if((value & 0x00FF0000) == value)
	{
		arm_mov_reg_imm8_rotate(inst, reg, (value >> 16), 8);
		return inst;
	}
	else if((value & 0xFF000000) == value)
	{
		arm_mov_reg_imm8_rotate(inst, reg, ((value >> 24) & 0xFF), 4);
		return inst;
	}

	/* Handle inverted bytes in various positions */
	value = ~value;
	if((value & 0x000000FF) == value)
	{
		arm_mov_reg_imm8(inst, reg, value);
		arm_alu_reg(inst, ARM_MVN, reg, reg);
		return inst;
	}
	else if((value & 0x0000FF00) == value)
	{
		arm_mov_reg_imm8_rotate(inst, reg, (value >> 8), 12);
		arm_alu_reg(inst, ARM_MVN, reg, reg);
		return inst;
	}
	else if((value & 0x00FF0000) == value)
	{
		arm_mov_reg_imm8_rotate(inst, reg, (value >> 16), 8);
		arm_alu_reg(inst, ARM_MVN, reg, reg);
		return inst;
	}
	else if((value & 0xFF000000) == value)
	{
		arm_mov_reg_imm8_rotate(inst, reg, ((value >> 24) & 0xFF), 4);
		arm_alu_reg(inst, ARM_MVN, reg, reg);
		return inst;
	}

	/* Build the value the hard way, byte by byte */
	value = ~value;
	if((value & 0xFF000000) != 0)
	{
		arm_mov_reg_imm8_rotate(inst, reg, ((value >> 24) & 0xFF), 4);
		if((value & 0x00FF0000) != 0)
		{
			arm_alu_reg_imm8_rotate
				(inst, ARM_ADD, reg, reg, ((value >> 16) & 0xFF), 8);
		}
		if((value & 0x0000FF00) != 0)
		{
			arm_alu_reg_imm8_rotate
				(inst, ARM_ADD, reg, reg, ((value >> 8) & 0xFF), 12);
		}
		if((value & 0x000000FF) != 0)
		{
			arm_alu_reg_imm8(inst, ARM_ADD, reg, reg, (value & 0xFF));
		}
	}
	else if((value & 0x00FF0000) != 0)
	{
		arm_mov_reg_imm8_rotate(inst, reg, ((value >> 16) & 0xFF), 8);
		if((value & 0x0000FF00) != 0)
		{
			arm_alu_reg_imm8_rotate
				(inst, ARM_ADD, reg, reg, ((value >> 8) & 0xFF), 12);
		}
		if((value & 0x000000FF) != 0)
		{
			arm_alu_reg_imm8(inst, ARM_ADD, reg, reg, (value & 0xFF));
		}
	}
	else if((value & 0x0000FF00) != 0)
	{
		arm_mov_reg_imm8_rotate(inst, reg, ((value >> 8) & 0xFF), 12);
		if((value & 0x000000FF) != 0)
		{
			arm_alu_reg_imm8(inst, ARM_ADD, reg, reg, (value & 0xFF));
		}
	}
	else
	{
		arm_mov_reg_imm8(inst, reg, (value & 0xFF));
	}
	return inst;
}

arm_inst_ptr _arm_alu_reg_imm(arm_inst_ptr inst, int opc,
					          int dreg, int sreg, int imm,
					          int saveWork)
{
	int tempreg;
	if(saveWork)
	{
		if(dreg != ARM_R2 && sreg != ARM_R2)
		{
			tempreg = ARM_R2;
		}
		else if(dreg != ARM_R3 && sreg != ARM_R3)
		{
			tempreg = ARM_R3;
		}
		else
		{
			tempreg = ARM_R4;
		}
		arm_push_reg(inst, tempreg);
	}
	else
	{
		tempreg = ARM_WORK;
	}
	_arm_mov_reg_imm(inst, tempreg, imm);
	arm_alu_reg_reg(inst, opc, dreg, sreg, tempreg);
	if(saveWork)
	{
		arm_pop_reg(inst, tempreg);
	}
	return inst;
}

md_inst_ptr _md_arm_setcc(md_inst_ptr inst, int reg, int cond, int invcond)
{
	arm_test_reg_imm8(inst, ARM_CMP, reg, 0);
	arm_alu_reg_imm8_cond(inst, ARM_MOV, reg, 0, 1, cond);
	arm_alu_reg_imm8_cond(inst, ARM_MOV, reg, 0, 0, invcond);
	return inst;
}

#endif /* CVM_ARM */

#ifdef	__cplusplus
};
#endif
