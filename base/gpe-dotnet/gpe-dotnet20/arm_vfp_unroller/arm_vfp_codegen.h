/*
 * arm_vfp_codegen.h - Code generation macros for the ARM processor
 *			with a VFP.
 *
 * Copyright (C) 2003  Southern Storm Software, Pty Ltd.
 * Copyright (C) 2005  Kirill Kononenko  <Kirill.Kononenko@gmail.com>
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

#ifndef	_ARM_VFP_CODEGEN_H
#define	_ARM_VFP_CODEGEN_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Register numbers.
 */
typedef enum
{
	ARM_R0   = 0,
	ARM_R1   = 1,
	ARM_R2   = 2,
	ARM_R3   = 3,
	ARM_R4   = 4,
	ARM_R5   = 5,
	ARM_R6   = 6,
	ARM_R7   = 7,
	ARM_R8   = 8,
	ARM_R9   = 9,
	ARM_R10  = 10,
	ARM_R11  = 11,
	ARM_R12  = 12,
	ARM_R13  = 13,
	ARM_R14  = 14,
	ARM_R15  = 15,
	ARM_FP   = ARM_R11,			/* Frame pointer */
	ARM_LINK = ARM_R14,			/* Link register */
	ARM_PC   = ARM_R15,			/* Program counter */
	ARM_WORK = ARM_R12,			/* Work register that we can destroy */
	ARM_SP   = ARM_R13,			/* Stack pointer */

} ARM_REG;

/*
 * Floating-point double-precision register numbers.
 *
 * Note: for a single-precision operation we limit ourself to use only 
 * 16 registers out of possible 32. This is done to simplify native code
 * generation in the 'unroller'.
 *
 *
 * There is such thing as N, M, D bits. They extend the 4 bit
 * used to indicate a register number to 5 bits. N extends the first operand 
 * register of the instruction (four last bits are still in Fn); M extends the 
 * second operand register of the instrucion (4 last bits are still in Fm) 
 * and D extends the destination operand register of the instruction 
 * (4 last bits are still in Fd).
 *
 * For single-precision they can be 0 or 1. But for double-precision they
 * are always 0. That is for a single-precision operation we have 32 
 * single-precision registers, but for double-precision operation 
 * (which really use the 32 single-precision registers) we have only 
 * 16 double-precision registers.
 *
 * We set those N, M, D bits which holds the bottom bits of the operand 
 * registers to 0. 
 *
 * That is for double-precision register we use the 16 double-precision 
 * registers, but for a single-precision register that makes us use 
 * registers with numbers 0, 2, 4, 6, 8,...,16,...,30. 
 *
 * So, for a single-precision operations we use only 16 registers 
 * from at total of 32. This will simplify code generation for the 'unroller'.
 */

typedef enum
{
	ARM_D0   = 0,
	ARM_D1   = 1,
	ARM_D2   = 2,
	ARM_D3   = 3,
	ARM_D4   = 4,
	ARM_D5   = 5,
	ARM_D6   = 6,
	ARM_D7   = 7,
	ARM_D8   = 8,
	ARM_D9   = 9,
	ARM_D10  = 10,
	ARM_D11  = 11,
	ARM_D12  = 12,
	ARM_D13  = 13,
	ARM_D14  = 14,
	ARM_D15  = 15,


} ARM_FREG;



/*
 * Condition codes.
 */
typedef enum
{
	ARM_CC_EQ    = 0,			/* Equal */
	ARM_CC_NE    = 1,			/* Not equal */
	ARM_CC_CS    = 2,			/* Carry set */
	ARM_CC_CC    = 3,			/* Carry clear */
	ARM_CC_MI    = 4,			/* Negative */
	ARM_CC_PL    = 5,			/* Positive */
	ARM_CC_VS    = 6,			/* Overflow set */
	ARM_CC_VC    = 7,			/* Overflow clear */
	ARM_CC_HI    = 8,			/* Higher */
	ARM_CC_LS    = 9,			/* Lower or same */
	ARM_CC_GE    = 10,			/* Signed greater than or equal */
	ARM_CC_LT    = 11,			/* Signed less than */
	ARM_CC_GT    = 12,			/* Signed greater than */
	ARM_CC_LE    = 13,			/* Signed less than or equal */
	ARM_CC_AL    = 14,			/* Always */
	ARM_CC_NV    = 15,			/* Never */
	ARM_CC_GE_UN = ARM_CC_CS,	/* Unsigned greater than or equal */
	ARM_CC_LT_UN = ARM_CC_CC,	/* Unsigned less than */
	ARM_CC_GT_UN = ARM_CC_HI,	/* Unsigned greater than */
	ARM_CC_LE_UN = ARM_CC_LS,	/* Unsigned less than or equal */

} ARM_CC;

/*
 * Arithmetic and logical operations.
 */
typedef enum
{
	ARM_AND = 0,				/* Bitwise AND */
	ARM_EOR = 1,				/* Bitwise XOR */
	ARM_SUB = 2,				/* Subtract */
	ARM_RSB = 3,				/* Reverse subtract */
	ARM_ADD = 4,				/* Add */
	ARM_ADC = 5,				/* Add with carry */
	ARM_SBC = 6,				/* Subtract with carry */
	ARM_RSC = 7,				/* Reverse subtract with carry */
	ARM_TST = 8,				/* Test with AND */
	ARM_TEQ = 9,				/* Test with XOR */
	ARM_CMP = 10,				/* Test with SUB (compare) */
	ARM_CMN = 11,				/* Test with ADD */
	ARM_ORR = 12,				/* Bitwise OR */
	ARM_MOV = 13,				/* Move */
	ARM_BIC = 14,				/* Test with Op1 & ~Op2 */
	ARM_MVN = 15,				/* Bitwise NOT */

} ARM_OP;

/*
 * VFP operations.
 */
typedef enum
{
	ARM_FADDS	= 0,			/* Adds  */
	ARM_FSUBS	= 1,			/* Subs  */
	ARM_FMULS	= 2,			/* Muls  */
	ARM_FDIVS	= 3,			/* divs  */
	ARM_FCMPS	= 4, 			/* fcmps  */
	ARM_FNEGS	= 6,			/* fnegs  */
} ARM_FLOP;



/*
 * Shift operators.
 */
typedef enum
{
	ARM_SHL = 0,				/* Logical left */
	ARM_SHR = 1,				/* Logical right */
	ARM_SAR = 2,				/* Arithmetic right */
	ARM_ROR = 3,				/* Rotate right */

} ARM_SHIFT;

/*
 * Type for instruction pointers (word-based, not byte-based).
 */
typedef unsigned int *arm_inst_ptr;

/*
 * Build an instruction prefix from a condition code and a mask value.
 */
#define	arm_build_prefix(cond,mask)	\
			((((unsigned int)(cond)) << 28) | ((unsigned int)(mask)))

/*
 * Build an "always" instruction prefix for a regular instruction.
 */
#define arm_prefix(mask)	(arm_build_prefix(ARM_CC_AL, (mask)))

/*
 * Build special "always" prefixes.
 */
#define	arm_always			(arm_build_prefix(ARM_CC_AL, 0))
#define	arm_always_cc		(arm_build_prefix(ARM_CC_AL, (1 << 20)))
#define	arm_always_imm		(arm_build_prefix(ARM_CC_AL, (1 << 25)))

/*
 * Arithmetic or logical operation which doesn't set condition codes.
 */
#define	arm_alu_reg_reg(inst,opc,dreg,sreg1,sreg2)	\
			do { \
				*(inst)++ = arm_always | \
							(((unsigned int)(opc)) << 21) | \
							(((unsigned int)(dreg)) << 12) | \
							(((unsigned int)(sreg1)) << 16) | \
							 ((unsigned int)(sreg2)); \
			} while (0)
#define	arm_alu_reg_imm8(inst,opc,dreg,sreg,imm)	\
			do { \
				*(inst)++ = arm_always_imm | \
							(((unsigned int)(opc)) << 21) | \
							(((unsigned int)(dreg)) << 12) | \
							(((unsigned int)(sreg)) << 16) | \
							 ((unsigned int)((imm) & 0xFF)); \
			} while (0)
#define	arm_alu_reg_imm8_cond(inst,opc,dreg,sreg,imm,cond)	\
			do { \
				*(inst)++ = arm_build_prefix((cond), (1 << 25)) | \
							(((unsigned int)(opc)) << 21) | \
							(((unsigned int)(dreg)) << 12) | \
							(((unsigned int)(sreg)) << 16) | \
							 ((unsigned int)((imm) & 0xFF)); \
			} while (0)
#define	arm_alu_reg_imm8_rotate(inst,opc,dreg,sreg,imm,rotate)	\
			do { \
				*(inst)++ = arm_always_imm | \
							(((unsigned int)(opc)) << 21) | \
							(((unsigned int)(dreg)) << 12) | \
							(((unsigned int)(sreg)) << 16) | \
							(((unsigned int)(rotate)) << 8) | \
							 ((unsigned int)((imm) & 0xFF)); \
			} while (0)
extern arm_inst_ptr _arm_alu_reg_imm(arm_inst_ptr inst, int opc, int dreg,
							         int sreg, int imm, int saveWork);
#define	arm_alu_reg_imm(inst,opc,dreg,sreg,imm)	\
			do { \
				int __alu_imm = (int)(imm); \
				if(__alu_imm >= 0 && __alu_imm < 256) \
				{ \
					arm_alu_reg_imm8 \
						((inst), (opc), (dreg), (sreg), __alu_imm); \
				} \
				else \
				{ \
					(inst) = _arm_alu_reg_imm \
						((inst), (opc), (dreg), (sreg), __alu_imm, 0); \
				} \
			} while (0)
#define	arm_alu_reg_imm_save_work(inst,opc,dreg,sreg,imm)	\
			do { \
				int __alu_imm_save = (int)(imm); \
				if(__alu_imm_save >= 0 && __alu_imm_save < 256) \
				{ \
					arm_alu_reg_imm8 \
						((inst), (opc), (dreg), (sreg), __alu_imm_save); \
				} \
				else \
				{ \
					(inst) = _arm_alu_reg_imm \
						((inst), (opc), (dreg), (sreg), __alu_imm_save, 1); \
				} \
			} while (0)
#define arm_alu_reg(inst,opc,dreg,sreg)	\
			do { \
				*(inst)++ = arm_always | \
							(((unsigned int)(opc)) << 21) | \
							(((unsigned int)(dreg)) << 12) | \
							 ((unsigned int)(sreg)); \
			} while (0)
#define arm_alu_reg_cond(inst,opc,dreg,sreg,cond)	\
			do { \
				*(inst)++ = arm_build_prefix((cond), 0) | \
							(((unsigned int)(opc)) << 21) | \
							(((unsigned int)(dreg)) << 12) | \
							 ((unsigned int)(sreg)); \
			} while (0)

/*
 * Arithmetic or logical operation which sets condition codes.
 */
#define	arm_alu_cc_reg_reg(inst,opc,dreg,sreg1,sreg2)	\
			do { \
				*(inst)++ = arm_always_cc | \
							(((unsigned int)(opc)) << 21) | \
							(((unsigned int)(dreg)) << 12) | \
							(((unsigned int)(sreg1)) << 16) | \
							 ((unsigned int)(sreg2)); \
			} while (0)
#define	arm_alu_cc_reg_imm8(inst,opc,dreg,sreg,imm)	\
			do { \
				*(inst)++ = arm_always_imm | arm_always_cc | \
							(((unsigned int)(opc)) << 21) | \
							(((unsigned int)(dreg)) << 12) | \
							(((unsigned int)(sreg)) << 16) | \
							 ((unsigned int)((imm) & 0xFF)); \
			} while (0)
#define arm_alu_cc_reg(inst,opc,dreg,sreg)	\
			do { \
				*(inst)++ = arm_always_cc | \
							(((unsigned int)(opc)) << 21) | \
							(((unsigned int)(dreg)) << 12) | \
							 ((unsigned int)(sreg)); \
			} while (0)

/*
 * Test operation, which sets the condition codes but has no other result.
 */
#define arm_test_reg_reg(inst,opc,sreg1,sreg2)	\
			do { \
				arm_alu_cc_reg_reg((inst), (opc), 0, (sreg1), (sreg2)); \
			} while (0)
#define arm_test_reg_imm8(inst,opc,sreg,imm)	\
			do { \
				arm_alu_cc_reg_imm8((inst), (opc), 0, (sreg), (imm)); \
			} while (0)
#define arm_test_reg_imm(inst,opc,sreg,imm)	\
			do { \
				int __test_imm = (int)(imm); \
				if(__test_imm >= 0 && __test_imm < 256) \
				{ \
					arm_alu_cc_reg_imm8((inst), (opc), 0, (sreg), __test_imm); \
				} \
				else \
				{ \
					arm_mov_reg_imm((inst), ARM_WORK, __test_imm); \
					arm_test_reg_reg((inst), (opc), (sreg), ARM_WORK); \
				} \
			} while (0)

/*
 * Move a value between registers.
 */
#define	arm_mov_reg_reg(inst,dreg,sreg)	\
			do { \
				arm_alu_reg((inst), ARM_MOV, (dreg), (sreg)); \
			} while (0)

/*
 * Move an immediate value into a register.  This is hard because
 * ARM lacks an instruction to load a 32-bit immediate value directly.
 * We handle the simple cases and then bail out to a function for the rest.
 */
#define	arm_mov_reg_imm8(inst,reg,imm)	\
			do { \
				arm_alu_reg_imm8((inst), ARM_MOV, (reg), 0, (imm)); \
			} while (0)
#define	arm_mov_reg_imm8_rotate(inst,reg,imm,rotate)	\
			do { \
				arm_alu_reg_imm8_rotate((inst), ARM_MOV, (reg), \
										0, (imm), (rotate)); \
			} while (0)
extern arm_inst_ptr _arm_mov_reg_imm(arm_inst_ptr inst, int reg, int value);
#define	arm_mov_reg_imm(inst,reg,imm)	\
			do { \
				int __imm = (int)(imm); \
				if(__imm >= 0 && __imm < 256) \
				{ \
					arm_mov_reg_imm8((inst), (reg), __imm); \
				} \
				else if((reg) == ARM_PC) \
				{ \
					(inst) = _arm_mov_reg_imm((inst), ARM_WORK, __imm); \
					arm_mov_reg_reg((inst), ARM_PC, ARM_WORK); \
				} \
				else if(__imm > -256 && __imm < 0) \
				{ \
					arm_mov_reg_imm8((inst), (reg), ~(__imm)); \
					arm_alu_reg((inst), ARM_MVN, (reg), (reg)); \
				} \
				else \
				{ \
					(inst) = _arm_mov_reg_imm((inst), (reg), __imm); \
				} \
			} while (0)

/*
 * Clear a register to zero.
 */
#define	arm_clear_reg(inst,reg)	\
			do { \
				arm_mov_reg_imm8((inst), (reg), 0); \
			} while (0)

/*
 * No-operation instruction.
 */
#define	arm_nop(inst)	arm_mov_reg_reg((inst), ARM_R0, ARM_R0)

/*
 * Perform a shift operation.
 */
#define	arm_shift_reg_reg(inst,opc,dreg,sreg1,sreg2) \
			do { \
				*(inst)++ = arm_always | \
							(((unsigned int)ARM_MOV) << 21) | \
							(((unsigned int)(dreg)) << 12) | \
							(((unsigned int)(sreg2)) << 8) | \
							(((unsigned int)(opc)) << 5) | \
							 ((unsigned int)(1 << 4)) | \
							 ((unsigned int)(sreg1)); \
			} while (0)
#define	arm_shift_reg_imm8(inst,opc,dreg,sreg,imm) \
			do { \
				*(inst)++ = arm_always | \
							(((unsigned int)ARM_MOV) << 21) | \
							(((unsigned int)(dreg)) << 12) | \
							(((unsigned int)(opc)) << 5) | \
							(((unsigned int)(imm)) << 7) | \
							 ((unsigned int)(sreg)); \
			} while (0)

/*
 * Perform a multiplication instruction.  Note: ARM instruction rules
 * say that dreg should not be the same as sreg2, so we swap the order
 * of the arguments if that situation occurs.  We assume that sreg1
 * and sreg2 are distinct registers.
 */
#define arm_mul_reg_reg(inst,dreg,sreg1,sreg2)	\
			do { \
				if((dreg) != (sreg2)) \
				{ \
					*(inst)++ = arm_prefix(0x00000090) | \
								(((unsigned int)(dreg)) << 16) | \
								(((unsigned int)(sreg1)) << 8) | \
								 ((unsigned int)(sreg2)); \
				} \
				else \
				{ \
					*(inst)++ = arm_prefix(0x00000090) | \
								(((unsigned int)(dreg)) << 16) | \
								(((unsigned int)(sreg2)) << 8) | \
								 ((unsigned int)(sreg1)); \
				} \
			} while (0)

/*
 * Branch or jump immediate by a byte offset.  The offset is
 * assumed to be +/- 32 Mbytes.
 */
#define	arm_branch_imm(inst,cond,imm)	\
			do { \
				*(inst)++ = arm_build_prefix((cond), 0x0A000000) | \
							(((unsigned int)(((int)(imm)) >> 2)) & \
								0x00FFFFFF); \
			} while (0)
#define	arm_jump_imm(inst,imm)	arm_branch_imm((inst), ARM_CC_AL, (imm))

/*
 * Branch or jump to a specific target location.  The offset is
 * assumed to be +/- 32 Mbytes.
 */
#define	arm_branch(inst,cond,target)	\
			do { \
				int __br_offset = (int)(((unsigned char *)(target)) - \
							           (((unsigned char *)(inst)) + 8)); \
				arm_branch_imm((inst), (cond), __br_offset); \
			} while (0)
#define	arm_jump(inst,target)	arm_branch((inst), ARM_CC_AL, (target))

/*
 * Jump to a specific target location that may be greater than
 * 32 Mbytes away from the current location.
 */
#define	arm_jump_long(inst,target)	\
			do { \
				int __jmp_offset = (int)(((unsigned char *)(target)) - \
							            (((unsigned char *)(inst)) + 8)); \
				if(__jmp_offset >= -0x04000000 && __jmp_offset < 0x04000000) \
				{ \
					arm_jump_imm((inst), __jmp_offset); \
				} \
				else \
				{ \
					arm_mov_reg_imm((inst), ARM_PC, (int)(target)); \
				} \
			} while (0)

/*
 * Back-patch a branch instruction.
 */
#define	arm_patch(inst,target)	\
			do { \
				int __p_offset = (int)(((unsigned char *)(target)) - \
							          (((unsigned char *)(inst)) + 8)); \
				__p_offset = (__p_offset >> 2) & 0x00FFFFFF; \
				*((int *)(inst)) = (*((int *)(inst)) & 0xFF000000) | \
					__p_offset; \
			} while (0)

/*
 * Call a subroutine immediate by a byte offset.
 */
#define	arm_call_imm(inst,imm)	\
			do { \
				*(inst)++ = arm_prefix(0x0B000000) | \
							(((unsigned int)(((int)(imm)) >> 2)) & \
								0x00FFFFFF); \
			} while (0)

/*
 * Call a subroutine at a specific target location.
 */
#define	arm_call(inst,target)	\
			do { \
				int __call_offset = (int)(((unsigned char *)(target)) - \
							             (((unsigned char *)(inst)) + 8)); \
				if(__call_offset >= -0x04000000 && __call_offset < 0x04000000) \
				{ \
					arm_call_imm((inst), __call_offset); \
				} \
				else \
				{ \
					arm_mov_reg_imm((inst), ARM_WORK, (int)(target)); \
					arm_mov_reg_reg((inst), ARM_LINK, ARM_PC); \
					arm_mov_reg_reg((inst), ARM_PC, ARM_WORK); \
				} \
			} while (0)

/*
 * Return from a subroutine, where the return address is in the link register.
 */
#define	arm_return(inst)	\
			do { \
				arm_mov_reg_reg((inst), ARM_PC, ARM_LINK); \
			} while (0)

/*
 * Push a register onto the system stack.
 */
#define	arm_push_reg(inst,reg)	\
			do { \
				*(inst)++ = arm_prefix(0x05200004) | \
							(((unsigned int)ARM_SP) << 16) | \
							(((unsigned int)(reg)) << 12); \
			} while (0)

/*
 * Pop a register from the system stack.
 */
#define	arm_pop_reg(inst,reg)	\
			do { \
				*(inst)++ = arm_prefix(0x04900004) | \
							(((unsigned int)ARM_SP) << 16) | \
							(((unsigned int)(reg)) << 12); \
			} while (0)

/*
 * Load a word value from a pointer and then advance the pointer.
 */
#define	arm_load_advance(inst,dreg,sreg)	\
			do { \
				*(inst)++ = arm_prefix(0x04900004) | \
							(((unsigned int)(sreg)) << 16) | \
							(((unsigned int)(dreg)) << 12); \
			} while (0)

/*
 * Load a value from an address into a register.
 */
#define arm_load_membase_either(inst,reg,basereg,imm,mask)	\
			do { \
				int __mb_offset = (int)(imm); \
				if(__mb_offset >= 0 && __mb_offset < (1 << 12)) \
				{ \
					*(inst)++ = arm_prefix(0x05900000 | (mask)) | \
								(((unsigned int)(basereg)) << 16) | \
								(((unsigned int)(reg)) << 12) | \
								 ((unsigned int)__mb_offset); \
				} \
				else if(__mb_offset > -(1 << 12) && __mb_offset < 0) \
				{ \
					*(inst)++ = arm_prefix(0x05100000 | (mask)) | \
								(((unsigned int)(basereg)) << 16) | \
								(((unsigned int)(reg)) << 12) | \
								 ((unsigned int)(-__mb_offset)); \
				} \
				else \
				{ \
					arm_mov_reg_imm((inst), ARM_WORK, __mb_offset); \
					*(inst)++ = arm_prefix(0x07900000 | (mask)) | \
								(((unsigned int)(basereg)) << 16) | \
								(((unsigned int)(reg)) << 12) | \
								 ((unsigned int)ARM_WORK); \
				} \
			} while (0)
#define	arm_load_membase(inst,reg,basereg,imm)	\
			do { \
				arm_load_membase_either((inst), (reg), (basereg), (imm), 0); \
			} while (0)
#define	arm_load_membase_byte(inst,reg,basereg,imm)	\
			do { \
				arm_load_membase_either((inst), (reg), (basereg), (imm), \
										0x00400000); \
			} while (0)
#define	arm_load_membase_sbyte(inst,reg,basereg,imm)	\
			do { \
				arm_load_membase_either((inst), (reg), (basereg), (imm), \
										0x00400000); \
				arm_shift_reg_imm8((inst), ARM_SHL, (reg), (reg), 24); \
				arm_shift_reg_imm8((inst), ARM_SAR, (reg), (reg), 24); \
			} while (0)
#define	arm_load_membase_ushort(inst,reg,basereg,imm)	\
			do { \
				arm_load_membase_byte((inst), ARM_WORK, (basereg), (imm)); \
				arm_load_membase_byte((inst), (reg), (basereg), (imm) + 1); \
				arm_shift_reg_imm8((inst), ARM_SHL, (reg), (reg), 8); \
				arm_alu_reg_reg((inst), ARM_ORR, (reg), (reg), ARM_WORK); \
			} while (0)
#define	arm_load_membase_short(inst,reg,basereg,imm)	\
			do { \
				arm_load_membase_byte((inst), ARM_WORK, (basereg), (imm)); \
				arm_load_membase_byte((inst), (reg), (basereg), (imm) + 1); \
				arm_shift_reg_imm8((inst), ARM_SHL, (reg), (reg), 24); \
				arm_shift_reg_imm8((inst), ARM_SAR, (reg), (reg), 16); \
				arm_alu_reg_reg((inst), ARM_ORR, (reg), (reg), ARM_WORK); \
			} while (0)

/*
 * Store a value from a register into an address.
 *
 * Note: storing a 16-bit value destroys the value in the register.
 */
#define arm_store_membase_either(inst,reg,basereg,imm,mask)	\
			do { \
				int __sm_offset = (int)(imm); \
				if(__sm_offset >= 0 && __sm_offset < (1 << 12)) \
				{ \
					*(inst)++ = arm_prefix(0x05800000 | (mask)) | \
								(((unsigned int)(basereg)) << 16) | \
								(((unsigned int)(reg)) << 12) | \
								 ((unsigned int)__sm_offset); \
				} \
				else if(__sm_offset > -(1 << 12) && __sm_offset < 0) \
				{ \
					*(inst)++ = arm_prefix(0x05000000 | (mask)) | \
								(((unsigned int)(basereg)) << 16) | \
								(((unsigned int)(reg)) << 12) | \
								 ((unsigned int)(-__sm_offset)); \
				} \
				else \
				{ \
					arm_mov_reg_imm((inst), ARM_WORK, __sm_offset); \
					*(inst)++ = arm_prefix(0x07800000 | (mask)) | \
								(((unsigned int)(basereg)) << 16) | \
								(((unsigned int)(reg)) << 12) | \
								 ((unsigned int)ARM_WORK); \
				} \
			} while (0)
#define	arm_store_membase(inst,reg,basereg,imm)	\
			do { \
				arm_store_membase_either((inst), (reg), (basereg), (imm), 0); \
			} while (0)
#define	arm_store_membase_byte(inst,reg,basereg,imm)	\
			do { \
				arm_store_membase_either((inst), (reg), (basereg), (imm), \
										 0x00400000); \
			} while (0)
#define	arm_store_membase_sbyte(inst,reg,basereg,imm)	\
			do { \
				arm_store_membase_byte((inst), (reg), (basereg), (imm)); \
			} while (0)
#define	arm_store_membase_short(inst,reg,basereg,imm)	\
			do { \
				arm_store_membase_either((inst), (reg), (basereg), (imm), \
										 0x00400000); \
				arm_shift_reg_imm8((inst), ARM_SHR, (reg), (reg), 8); \
				arm_store_membase_either((inst), (reg), (basereg), \
										 (imm) + 1, 0x00400000); \
			} while (0)
#define	arm_store_membase_ushort(inst,reg,basereg,imm)	\
			do { \
				arm_store_membase_short((inst), (reg), (basereg), (imm)); \
			} while (0)

/*
 * Load a value from an indexed address into a register.
 */
#define arm_load_memindex_either(inst,reg,basereg,indexreg,shift,mask)	\
			do { \
				*(inst)++ = arm_prefix(0x07900000 | (mask)) | \
							(((unsigned int)(basereg)) << 16) | \
							(((unsigned int)(reg)) << 12) | \
							(((unsigned int)(shift)) << 7) | \
							 ((unsigned int)(indexreg)); \
			} while (0)
#define	arm_load_memindex(inst,reg,basereg,indexreg)	\
			do { \
				arm_load_memindex_either((inst), (reg), (basereg), \
										 (indexreg), 2, 0); \
			} while (0)
#define	arm_load_memindex_byte(inst,reg,basereg,indexreg)	\
			do { \
				arm_load_memindex_either((inst), (reg), (basereg), \
									     (indexreg), 0, 0x00400000); \
			} while (0)
#define	arm_load_memindex_sbyte(inst,reg,basereg,indexreg)	\
			do { \
				arm_load_memindex_either((inst), (reg), (basereg), \
									     (indexreg), 0, 0x00400000); \
				arm_shift_reg_imm8((inst), ARM_SHL, (reg), (reg), 24); \
				arm_shift_reg_imm8((inst), ARM_SAR, (reg), (reg), 24); \
			} while (0)
#define	arm_load_memindex_ushort(inst,reg,basereg,indexreg)	\
			do { \
				arm_alu_reg_reg((inst), ARM_ADD, ARM_WORK, (basereg), \
								(indexreg)); \
				arm_alu_reg_reg((inst), ARM_ADD, ARM_WORK, ARM_WORK, \
								(indexreg)); \
				arm_load_membase_byte((inst), (reg), ARM_WORK, 0); \
				arm_load_membase_byte((inst), ARM_WORK, ARM_WORK, 1); \
				arm_shift_reg_imm8((inst), ARM_SHL, ARM_WORK, ARM_WORK, 8); \
				arm_alu_reg_reg((inst), ARM_ORR, (reg), (reg), ARM_WORK); \
			} while (0)
#define	arm_load_memindex_short(inst,reg,basereg,indexreg)	\
			do { \
				arm_alu_reg_reg((inst), ARM_ADD, ARM_WORK, (basereg), \
								(indexreg)); \
				arm_alu_reg_reg((inst), ARM_ADD, ARM_WORK, ARM_WORK, \
								(indexreg)); \
				arm_load_membase_byte((inst), (reg), ARM_WORK, 0); \
				arm_load_membase_byte((inst), ARM_WORK, ARM_WORK, 1); \
				arm_shift_reg_imm8((inst), ARM_SHL, ARM_WORK, ARM_WORK, 24); \
				arm_shift_reg_imm8((inst), ARM_SAR, ARM_WORK, ARM_WORK, 16); \
				arm_alu_reg_reg((inst), ARM_ORR, (reg), (reg), ARM_WORK); \
			} while (0)

/*
 * Store a value from a register into an indexed address.
 *
 * Note: storing a 16-bit value destroys the values in the base
 * register and the source register.
 */
#define arm_store_memindex_either(inst,reg,basereg,indexreg,shift,mask)	\
			do { \
				*(inst)++ = arm_prefix(0x07800000 | (mask)) | \
							(((unsigned int)(basereg)) << 16) | \
							(((unsigned int)(reg)) << 12) | \
							(((unsigned int)(shift)) << 7) | \
							 ((unsigned int)(indexreg)); \
			} while (0)
#define	arm_store_memindex(inst,reg,basereg,indexreg)	\
			do { \
				arm_store_memindex_either((inst), (reg), (basereg), \
										  (indexreg), 2, 0); \
			} while (0)
#define	arm_store_memindex_byte(inst,reg,basereg,indexreg)	\
			do { \
				arm_store_memindex_either((inst), (reg), (basereg), \
										  (indexreg), 0, 0x00400000); \
			} while (0)
#define	arm_store_memindex_sbyte(inst,reg,basereg,indexreg)	\
			do { \
				arm_store_memindex_byte((inst), (reg), (basereg), \
										(indexreg)); \
			} while (0)
#define	arm_store_memindex_short(inst,reg,basereg,indexreg)	\
			do { \
				arm_store_memindex_either((inst), (reg), (basereg), \
										  (indexreg), 1, 0x00400000); \
				arm_alu_reg_imm8((inst), ARM_ADD, (basereg), (basereg), 1); \
				arm_shift_reg_imm8((inst), ARM_SHR, (reg), (reg), 8); \
				arm_store_memindex_either((inst), (reg), (basereg), \
										  (indexreg), 1, 0x00400000); \
			} while (0)
#define	arm_store_memindex_ushort(inst,reg,basereg,indexreg)	\
			do { \
				arm_store_memindex_short((inst), (reg), \
										 (basereg), (indexreg)); \
			} while (0)
/*
 * The FLDS (Floating-point Load, Single-precision) instruction loads a
 * single-precision register from memory.
 * Load a value from memory at start_address = Rn + offset_8 * 4
 * to single-precision register 'dreg' in VFP
 *
 * Note: always -> [31:28] = b1110 = 0xe
 * U = 1
 * D = 0 
 * cp_num = 0b1010 (single-precision coprocessor)
 */
#define arm_vfp_FLDS(inst,dreg,sreg,offset)	\
			do { \
				*(inst)++ = arm_always |
						((unsigned int)(0x0d900a00))) | \
						(((unsigned int)(dreg)) << 16) | \
						(((unsigned int)(sreg)) << 12) | \
						((unsigned int)(offset)); \
			} while (0)

/* The FSTS (FLoating-point Store, Single-precision) instruction stores a
 * single-precision register to memory.
 *
 * Store to memory at start_address = Rn + offset_8 * 4
 * a value from single-precision register 'dreg' in VFP
 *
 * Note: always -> [31:28] = b1110 = 0xe
 * U = 1
 * D = 0 
 * cp_num = 0b1010 (single-precision coprocessor)
 */
#define arm_vfp_FSTS(inst,dreg,sreg,offset)	\
			do { \
				*(inst)++ = arm_always | 
						((unsigned int)(0x0d800a00)) | \
						(((unsigned int)(dreg)) << 16) | \
						(((unsigned int)(sreg)) << 12) | \
						((unsigned int)(offset)); \
			} while (0)

/* The FLDD (Floating-point Load, Double-precision) instruction loads a
 * double-precision register from memory.
 *
 * Load a value from memory at start_address = Rn + offset_8 * 4
 * to double-precision register 'dreg' in VFP
 *
 * Note: always -> [31:28] = b1110 = 0xe
 * U = 1
 * D = 0 
 * cp_num = 0b1011 (double-precision coprocessor)
 */
#define arm_vfp_FLDD(inst,dreg,sreg,offset)	\
			do { \
				*(inst)++ = arm_always | 
						((unsigned int)(0x0d900b00)) | \
						(((unsigned int)(dreg)) << 16) | \
						(((unsigned int)(sreg)) << 12) | \
						((unsigned int)(offset)); \
			} while (0)

/* The FSTD (Floating-point Store, Double-precision) instruction stores a
 * double-precision register to memory.
 *
 * Store to memory at start_address = Rn + offset_8 * 4
 * a value from double-precision register 'dreg' in VFP
 *
 * Note: always -> [31:28] = b1110 = 0xe
 * U = 1
 * D = 0 
 * cp_num = 0b1011 (double-precision coprocessor)
 */
#define arm_vfp_FSTD(inst,dreg,sreg,offset)	\
			do { \
				*(inst)++ = arm_always | 
						((unsigned int)(0x0d800b00)) | \
						(((unsigned int)(dreg)) << 16) | \
						(((unsigned int)(sreg)) << 12) | \
						((unsigned int)(offset)); \
			} while (0)

/*
 * The FCVTS (Floating-point Convert to Double-precision from Single-presion)
 * instruction converts the value in a single-precision register to double
 * precision and writes the result to a double-precision register.
 */
#define arm_vfp_FCVTDS(inst,dreg,sreg,offset)	\
			do { \
				*(inst)++ = arm_always | 
						((unsigned int)(0x0eb70ac0)) | \
						(((unsigned int)(dreg)) << 12) | \
						((unsigned int)(sreg)); \
			} while (0)

/* The FCVTSD (Floating-point Convert to Single-precision from 
 * Double-precision) converts the value in a double-precision register to
 * single precision and writes the result to a single-precision register.
 */
#define arm_vfp_FCVTSSD(inst,dreg,sreg,offset)	\
			do { \
				*(inst)++ = arm_always | 
						((unsigned int)(0x0eb70bc0)) | \
						(((unsigned int)(dreg)) << 12) | \ | \
						((unsigned int)(sreg)); \
			} while (0)


/* The FADDD (Floating-point Addition, Double-precision) instruction adds
 * together two double-precision registers and writes the result to a third
 * double-precision register. It can also perform a vector version of this
 * operation.
 */
#define arm_vfp_FADDD(inst,dreg,sreg1,sreg2)	\
			do { \
				*(inst)++ = arm_always | 
						((unsigned int)(0x0e300b00)) | \
						(((unsigned int)(dreg)) << 12) | \
						(((unsigned int)(sreg1)) << 16) | \
						((unsigned int)(sreg2));
			} while (0)

/* The FSUBD (Floating-point Substact, Double-precision) instruction substracts
 * one double-precision register from another double-precision register and
 * writes the result to a third double-precision register. It can also perform
 * a vector version of this operation.
 */
#define arm_vfp_FSUBD(inst,dreg,sreg1,sreg2)	\
			do { \
				*(inst)++ = arm_always | 
						((unsigned int)(0x0e300b40)) | \
						(((unsigned int)(dreg)) << 12) | \
						(((unsigned int)(sreg1)) << 16) | \
						((unsigned int)(sreg2));
			} while (0)

/* The FMULD (Floating-point Multiply, Double-precision) instruction multiplies
 * together two double-precision registers and writes the result to a third 
 * double-precision register. It can also perform a vector version ot this operation.
 */
#define arm_vfp_FMULD(inst,dreg,sreg1,sreg2)	\
			do { \
				*(inst)++ = arm_always | 
						((unsigned int)(0x0e200b00)) | \
						(((unsigned int)(dreg)) << 12) | \
						(((unsigned int)(sreg1)) << 16) | \
						((unsigned int)(sreg2));
			} while (0)

/* The PDIVD (Floating-point Divide, Double-precision) instruction divides one
 * double-precision register by another double-precision register and writes the
 * result to a third double-precision register. It can alswo perform a vector
 * version of this operation.
 */
#define arm_vfp_FDIVD(inst,dreg,sreg1,sreg2)	\
			do { \
				*(inst)++ = arm_always | 
						((unsigned int)(0x0e800b00)) | \
						(((unsigned int)(dreg)) << 12) | \
						(((unsigned int)(sreg1)) << 16) | \
						((unsigned int)(sreg2));
			} while (0)

/* The PNEGD (Floating-point Negate, Double-precisin) instruction negates the
 * value of a double-precision register and writes the result to another 
 * double-precision register. It can also perform a vector version of this
 * operation.
 */

#define arm_vfp_FNEGD(inst,dreg,sreg)	\
			do { \
				*(inst)++ = arm_always | 
						((unsigned int)(0x0eb10b40)) | \
						(((unsigned int)(dreg)) << 12) | \
						((unsigned int)(sreg));
			} while (0)



#ifdef __cplusplus
};
#endif

#endif /* _ARM_VFP_CODEGEN_H */
