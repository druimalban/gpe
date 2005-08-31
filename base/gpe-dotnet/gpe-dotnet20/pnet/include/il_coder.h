/*
 * il_coder.h - Interface to the "coder" within the runtime engine.
 *
 * Copyright (C) 2001  Southern Storm Software, Pty Ltd.
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

/*
The coder interface provides hooks for "Just In Time" compilers
to add themselves to the runtime engine.  The bytecode verifier
makes callbacks into the coder to generate the final code.

Coders do not have to be JIT's.  They can be any back-end that
wants to rebuild the structure of a method's code given the type
information that is inferred by the bytecode verifier.
*/

#ifndef	_IL_CODER_H
#define	_IL_CODER_H

#include "il_engine.h"

#ifdef	__cplusplus
extern	"C" {
#endif

/*
 * Forward declarations.
 */
typedef struct _tagILCoder      ILCoder;
typedef struct _tagILCoderClass ILCoderClass;

/*
 * Engine types.
 */
typedef enum
{
	ILEngineType_I4,
	ILEngineType_I8,
	ILEngineType_I,
	ILEngineType_F,
	ILEngineType_M,
	ILEngineType_O,
	ILEngineType_T,
	ILEngineType_MV,
	ILEngineType_Invalid,
	ILEngineType_TypedRef,

} ILEngineType;
#define	ILEngineType_ValidTypes	8

/*
 * Type that is used for stack items during verfication.
 */
typedef struct
{
	ILEngineType engineType;
	ILType      *typeInfo;

} ILEngineStackItem;

/*
 * Common parent fields that are shared by all coder instances.
 */
struct _tagILCoder
{
	const ILCoderClass *classInfo;
};

/*
 * Argument information for a method call.
 */
typedef struct
{
	ILEngineStackItem *args;
	ILUInt32 numBaseArgs;
	ILUInt32 numVarArgs;
	int hasParamArray;
	int tailCall;

} ILCoderMethodInfo;

/*
 * Inlineable method calls.
 */
#define	IL_INLINEMETHOD_MONITOR_ENTER		0
#define	IL_INLINEMETHOD_MONITOR_EXIT		1
#define	IL_INLINEMETHOD_STRING_LENGTH		2
#define	IL_INLINEMETHOD_STRING_CONCAT_2		3
#define	IL_INLINEMETHOD_STRING_CONCAT_3		4
#define	IL_INLINEMETHOD_STRING_CONCAT_4		5
#define IL_INLINEMETHOD_STRING_EQUALS		6
#define	IL_INLINEMETHOD_STRING_NOT_EQUALS	7
#define	IL_INLINEMETHOD_STRING_GET_CHAR		8
#define	IL_INLINEMETHOD_TYPE_FROM_HANDLE	9
#define	IL_INLINEMETHOD_GET2D_INT			10
#define	IL_INLINEMETHOD_GET2D_OBJECT		11
#define	IL_INLINEMETHOD_GET2D_DOUBLE		12
#define	IL_INLINEMETHOD_SET2D_INT			13
#define	IL_INLINEMETHOD_SET2D_OBJECT		14
#define	IL_INLINEMETHOD_SET2D_DOUBLE		15
#define	IL_INLINEMETHOD_BUILDER_APPEND_CHAR	16
#define	IL_INLINEMETHOD_BUILDER_APPEND_SPACE 17
#define	IL_INLINEMETHOD_IS_WHITE_SPACE		18
#define	IL_INLINEMETHOD_ASIN				19
#define	IL_INLINEMETHOD_ATAN				20
#define	IL_INLINEMETHOD_ATAN2				21
#define	IL_INLINEMETHOD_CEILING				22
#define	IL_INLINEMETHOD_COS					23
#define	IL_INLINEMETHOD_COSH				24
#define	IL_INLINEMETHOD_EXP					25
#define	IL_INLINEMETHOD_FLOOR				26
#define	IL_INLINEMETHOD_IEEEREMAINDER		27
#define	IL_INLINEMETHOD_LOG					28
#define	IL_INLINEMETHOD_LOG10				29
#define	IL_INLINEMETHOD_MIN_I4				30
#define	IL_INLINEMETHOD_MAX_I4				31
#define	IL_INLINEMETHOD_MIN_R4				32
#define	IL_INLINEMETHOD_MAX_R4				33
#define	IL_INLINEMETHOD_MIN_R8				34
#define	IL_INLINEMETHOD_MAX_R8				35
#define	IL_INLINEMETHOD_POW					36
#define	IL_INLINEMETHOD_ROUND				37
#define	IL_INLINEMETHOD_SIGN_I4				38
#define	IL_INLINEMETHOD_SIGN_R4				39
#define	IL_INLINEMETHOD_SIGN_R8				40
#define	IL_INLINEMETHOD_SIN					41
#define	IL_INLINEMETHOD_SINH				42
#define	IL_INLINEMETHOD_SQRT				43
#define	IL_INLINEMETHOD_TAN					44
#define	IL_INLINEMETHOD_TANH				45
#define	IL_INLINEMETHOD_ABS_I4				46
#define	IL_INLINEMETHOD_ABS_R4				47
#define	IL_INLINEMETHOD_ABS_R8				48
/*
 * Return values for "ILCoderFinish".
 */
#define	IL_CODER_END_OK			0		/* Method is OK */
#define	IL_CODER_END_RESTART	1		/* Restart is required */
#define	IL_CODER_END_TOO_BIG	2		/* Method is too big for the cache */

/* 
 * Profiling flags
 */

#define IL_CODER_FLAG_IR_DUMP 1
#define IL_CODER_FLAG_METHOD_PROFILE 2
#define IL_CODER_FLAG_METHOD_TRACE 4
#define IL_CODER_FLAG_STATS 8
/*
 * Coder class definition.
 */
struct _tagILCoderClass
{
	/*
	 * Create a coder instance.  Returns NULL if not possible.
	 */
	ILCoder *(*create)(ILExecProcess *process, ILUInt32 size,
						unsigned long cachePageSize);
	
	/*
	 * Enable debug mode.  The coder will output extra information
	 * to the code stream to allow breakpoint debugging to proceed.
	 */
	void (*enableDebug)(ILCoder *coder);

	/*
	 * Allocate a block of memory within this coder instance.
	 * Returns NULL if cache overflow has occurred.
	 */
	void *(*alloc)(ILCoder *coder, ILUInt32 size);

	/*
	 * Get the amount of memory that is in use by the coder
	 * to cache translated methods.
	 */
	unsigned long (*getCacheSize)(ILCoder *coder);

	/*
	 * Set up a coder instance for processing a specific method.
	 * Returns zero if not possible.  The start of the method is
	 * returned in "*start".
	 */
	int (*setup)(ILCoder *coder, unsigned char **start,
				 ILMethod *method, ILMethodCode *code);

	/*
	 * Set up a coder instance for processing a specific external method.
	 * Returns zero if not possible.
	 */
	int (*setupExtern)(ILCoder *coder, unsigned char **start,
					   ILMethod *method, void *fn, void *cif,
					   int isInternal);

	/*
	 * Set up a coder instance for processing a specific external constructor.
	 * Returns zero if not possible.
	 */
	int (*setupExternCtor)(ILCoder *coder, unsigned char **start,
					       ILMethod *method, void *fn, void *cif,
					       void *ctorfn, void *ctorcif, int isInternal);

	/*
	 * Get the offset to use to convert a method entry point
	 * into an allocation constructor entry point.
	 */
	int (*ctorOffset)(ILCoder *coder);

	/*
	 * Destroy a coder instance.
	 */
	void (*destroy)(ILCoder *coder);

	/*
	 * Finish processing on the method.  Returns a coder
	 * "end" code.
	 */
	int (*finish)(ILCoder *coder);

	/*
	 * Output a label to the coder.  "offset" is the
	 * position within the IL bytecode of the label.
	 */
	void (*label)(ILCoder *coder, ILUInt32 offset);

	/*
	 * Refresh the coder's notion of the stack contents just
	 * after a label has been output.
	 */
	void (*stackRefresh)(ILCoder *coder, ILEngineStackItem *stack,
						 ILUInt32 stackSize);

	/*
	 * Handle a numeric constant value.
	 */
	void (*constant)(ILCoder *coder, int opcode, unsigned char *arg);

	/*
	 * Handle a string constant value.
	 */
	void (*stringConstant)(ILCoder *coder, ILToken token, void *object);

	/*
	 * Handle a binary operator.
	 */
	void (*binary)(ILCoder *coder, int opcode, ILEngineType type1,
				   ILEngineType type2);

	/*
	 * Handle a binary operator when pointer arithmetic is involved.
	 */
	void (*binaryPtr)(ILCoder *coder, int opcode, ILEngineType type1,
				      ILEngineType type2);

	/*
	 * Handle a shift operator.
	 */
	void (*shift)(ILCoder *coder, int opcode, ILEngineType type1,
				  ILEngineType type2);

	/*
	 * Handle a unary operator.
	 */
	void (*unary)(ILCoder *coder, int opcode, ILEngineType type);

	/*
	 * Load an argument onto the stack.
	 */
	void (*loadArg)(ILCoder *coder, ILUInt32 num, ILType *type);

	/*
	 * Store the top of the stack into an argument.
	 */
	void (*storeArg)(ILCoder *coder, ILUInt32 num,
					 ILEngineType engineType, ILType *type);

	/*
	 * Load the address of an argument onto the stack.
	 */
	void (*addrOfArg)(ILCoder *coder, ILUInt32 num);

	/*
	 * Load a local variable onto the stack.
	 */
	void (*loadLocal)(ILCoder *coder, ILUInt32 num, ILType *type);

	/*
	 * Store the top of the stack into a local variable.
	 */
	void (*storeLocal)(ILCoder *coder, ILUInt32 num,
					   ILEngineType engineType, ILType *type);

	/*
	 * Load the address of a local variable onto the stack.
	 */
	void (*addrOfLocal)(ILCoder *coder, ILUInt32 num);

	/*
	 * Duplicate the top of stack.
	 */
	void (*dup)(ILCoder *coder, ILEngineType valueType, ILType *type);

	/*
	 * Pop the top of stack.
	 */
	void (*pop)(ILCoder *coder, ILEngineType valueType, ILType *type);

	/*
	 * Access an element within an array.  If the element type
	 * is "ILType_Void", then it indicates that the array pointer
	 * was inferred to be "null" and no other element information
	 * is known.
	 */
	void (*arrayAccess)(ILCoder *coder, int opcode,
						ILEngineType indexType, ILType *elemType);

	/*
	 * Access a value by dereferencing a pointer.
	 */
	void (*ptrAccess)(ILCoder *coder, int opcode);

	/*
	 * Access a managed value by dereferencing a pointer.
	 */
	void (*ptrAccessManaged)(ILCoder *coder, int opcode,
							 ILClass *classInfo);

	/*
	 * Output a branch instruction.
	 */
	void (*branch)(ILCoder *coder, int opcode, ILUInt32 dest,
				   ILEngineType type1, ILEngineType type2);

	/*
	 * Output the start of a switch statement.
	 */
	void (*switchStart)(ILCoder *coder, ILUInt32 numEntries);

	/*
	 * Output a specific switch entry.
	 */
	void (*switchEntry)(ILCoder *coder, ILUInt32 dest);

	/*
	 * Output a comparison instruction.  If "invertTest" is
	 * non-zero, then the result of the test should be inverted.
	 */
	void (*compare)(ILCoder *coder, int opcode,
				    ILEngineType type1, ILEngineType type2,
					int invertTest);

	/*
	 * Output a conversion instruction.
	 */
	void (*conv)(ILCoder *coder, int opcode, ILEngineType type);

	/*
	 * Convert an integer (I or I4) into a pointer.  "type1" is
	 * the integer type to be converted.  "type2" is the type of
	 * the value on the stack just above the integer for a store
	 * opcode, or NULL for a load opcode.
	 */
	void (*toPointer)(ILCoder *coder, ILEngineType type1,
					  ILEngineStackItem *type2);

	/*
	 * Process a pointer alignment prefix.  An "alignment" value
	 * of zero indicates a "volatile" prefix.
	 */
	void (*ptrPrefix)(ILCoder *coder, int alignment);

	/*
	 * Get the length of an array.
	 */
	void (*arrayLength)(ILCoder *coder);

	/*
	 * Construct a new array, given a type and length value.
	 */
	void (*newArray)(ILCoder *coder, ILType *arrayType,
					 ILClass *arrayClass, ILEngineType lengthType);

	/*
	 * Allocate local stack space.
	 */
	void (*localAlloc)(ILCoder *coder, ILEngineType sizeType);

	/*
	 * Cast the top-most object on the stack to a new class.
	 * If "throwException" is non-zero, then throw an exception
	 * if the object is not an instance of the class.  Otherwise
	 * push NULL onto the stack.
	 */
	void (*castClass)(ILCoder *coder, ILClass *classInfo,
					  int throwException);

	/*
	 * Load the contents of an instance field onto the stack.
	 * "ptrType" is the type of pointer that is being used to
	 * dereference the field.  "objectType" is the type of the
	 * entire object.  "field" is the field descriptor.
	 * "fieldType" is the type of the field.
	 */
	void (*loadField)(ILCoder *coder, ILEngineType ptrType,
					  ILType *objectType, ILField *field,
					  ILType *fieldType);

	/*
	 * Load the contents of a static field onto the stack.
	 */
	void (*loadStaticField)(ILCoder *coder, ILField *field,
							ILType *fieldType);

	/*
	 * Load the contents of an instance field onto the stack,
	 * where the pointer is an object reference in argument 0.
	 */
	void (*loadThisField)(ILCoder *coder, ILField *field, ILType *fieldType);

	/*
	 * Load the address of an instance field onto the stack.
	 */
	void (*loadFieldAddr)(ILCoder *coder, ILEngineType ptrType,
					      ILType *objectType, ILField *field,
					      ILType *fieldType);

	/*
	 * Load the address of a static field onto the stack.
	 */
	void (*loadStaticFieldAddr)(ILCoder *coder, ILField *field,
							    ILType *fieldType);

	/*
	 * Store a value from the stack into an instance field.
	 * "ptrType" is the type of pointer that is being used to
	 * dereference the field.  "objectType" is the type of the
	 * entire object.  "field" is the field descriptor.
	 * "fieldType" is the type of the field.  "valueType" is
	 * the type of the value currently on the stack.
	 */
	void (*storeField)(ILCoder *coder, ILEngineType ptrType,
					   ILType *objectType, ILField *field,
					   ILType *fieldType, ILEngineType valueType);

	/*
	 * Store a value from the stack into a stack field.
	 */
	void (*storeStaticField)(ILCoder *coder, ILField *field,
							 ILType *fieldType, ILEngineType valueType);

	/*
	 * Copy the contents of an object, which has the class "classInfo".
	 */
	void (*copyObject)(ILCoder *coder, ILEngineType destPtrType,
					   ILEngineType srcPtrType, ILClass *classInfo);

	/*
	 * Copy the contents of a block of memory.
	 */
	void (*copyBlock)(ILCoder *coder, ILEngineType destPtrType,
					  ILEngineType srcPtrType);

	/*
	 * Initialize the contents of an object, which has the class "classInfo".
	 */
	void (*initObject)(ILCoder *coder, ILEngineType ptrType,
					   ILClass *classInfo);

	/*
	 * Initialize the contents of a block of memory.
	 */
	void (*initBlock)(ILCoder *coder, ILEngineType ptrType);

	/*
	 * Box a value which is already in its natural representation.
	 */
	void (*box)(ILCoder *coder, ILClass *boxClass,
				ILEngineType valueType, ILUInt32 size);

	/*
	 * Box a value which needs to be converted into a smaller
	 * representation before being copied to the final object.
	 * "smallerType" will be one of "ILType_Int8", "ILType_Int16",
	 * "ILType_Float32", or "ILType_Float64".
	 */
	void (*boxSmaller)(ILCoder *coder, ILClass *boxClass,
					   ILEngineType valueType, ILType *smallerType);

	/*
	 * Unbox an object into a managed pointer.
	 */
	void (*unbox)(ILCoder *coder, ILClass *boxClass);

	/*
	 * Make a typed reference from a pointer.
	 */
	void (*makeTypedRef)(ILCoder *coder, ILClass *classInfo);

	/*
	 * Extract the value part of a typed reference.
	 */
	void (*refAnyVal)(ILCoder *coder, ILClass *classInfo);

	/*
	 * Extract the type part of a typed reference.
	 */
	void (*refAnyType)(ILCoder *coder);

	/*
	 * Push a token item pointer onto the stack.
	 */
	void (*pushToken)(ILCoder *coder, ILProgramItem *item);

	/*
	 * Push the size of a type onto the stack as an unsigned I4.
	 */
	void (*sizeOf)(ILCoder *coder, ILType *type);

	/*
	 * Push a managed value of type "RuntimeArgumentHandle",
	 * which represents the arguments of the current method.
	 */
	void (*argList)(ILCoder *coder);

	/*
	 * Up-convert an argument that is of an integer size that
	 * is too small for the formal parameter.  "param" is 1 for
	 * the first argument.
	 */
	void (*upConvertArg)(ILCoder *coder, ILEngineStackItem *args,
						 ILUInt32 numArgs, ILUInt32 param,
						 ILType *paramType);

	/*
	 * Down-convert an argument that is of an integer size that
	 * is too large for the formal parameter.  "param" is 1 for
	 * the first argument.
	 */
	void (*downConvertArg)(ILCoder *coder, ILEngineStackItem *args,
						   ILUInt32 numArgs, ILUInt32 param,
						   ILType *paramType);

	/*
	 * Pack the top-most "num" items on the stack into an "Object[]"
	 * array, for passing to a vararg method.  The "firstParam"
	 * value is the first parameter in "callSiteSig" to use to
	 * get the parameter type information.
	 */
	void (*packVarArgs)(ILCoder *coder, ILType *callSiteSig,
					    ILUInt32 firstParam, ILEngineStackItem *args,
						ILUInt32 numArgs);

	/*
	 * Insert two values into the stack for value type construction.
	 * The first is a managed value, and the second is a pointer
	 * to the managed value.
	 */
	void (*valueCtorArgs)(ILCoder *coder, ILClass *classInfo,
						  ILEngineStackItem *args, ILUInt32 numArgs);

	/*
	 * Check a method call's first parameter for null.
	 */
	void (*checkCallNull)(ILCoder *coder, ILCoderMethodInfo *info);

	/*
	 * Call a method directly.
	 */
	void (*callMethod)(ILCoder *coder, ILCoderMethodInfo *info,
					   ILEngineStackItem *returnItem, ILMethod *methodInfo);

	/*
	 * Call a method using an indirect pointer.
	 */
	void (*callIndirect)(ILCoder *coder, ILCoderMethodInfo *info,
					     ILEngineStackItem *returnItem);

	/*
	 * Call a constructor method directly.
	 */
	void (*callCtor)(ILCoder *coder, ILCoderMethodInfo *info,
					 ILMethod *methodInfo);

	/*
	 * Call a virtual method.
	 */
	void (*callVirtual)(ILCoder *coder, ILCoderMethodInfo *info,
					    ILEngineStackItem *returnItem, ILMethod *methodInfo);

	/*
	 * Call an interface method.
	 */
	void (*callInterface)(ILCoder *coder, ILCoderMethodInfo *info,
					      ILEngineStackItem *returnItem, ILMethod *methodInfo);

	/*
	 * Call an inlineable method.  Returns zero if the coder
	 * cannot inline the method.
	 */
	int (*callInlineable)(ILCoder *coder, int inlineType, ILMethod *methodInfo);

	/*
	 * Jump to a method with the same signature as the current method.
	 */
	void (*jumpMethod)(ILCoder *coder, ILMethod *methodInfo);

	/*
	 * Return from the current method.  "engineType" will be
	 * "ILEngineType_Invalid" if the return type is "void".
	 */
	void (*returnInsn)(ILCoder *coder, ILEngineType engineType,
					   ILType *returnType);

	/*
	 * Load the address of a function onto the stack.
	 */
	void (*loadFuncAddr)(ILCoder *coder, ILMethod *methodInfo);

	/*
	 * Load the address of a virtual function onto the stack.
	 */
	void (*loadVirtualAddr)(ILCoder *coder, ILMethod *methodInfo);

	/*
	 * Load the address of an interface function onto the stack.
	 */
	void (*loadInterfaceAddr)(ILCoder *coder, ILMethod *methodInfo);

	/*
	 * Set up exception handling for the current method.
	 */
	void (*setupExceptions)(ILCoder *coder, ILException *exceptions,
							int hasRethrow);

	/*
	 * Throw an exception.  If "inCurrentMethod" is non-zero,
	 * then there is a catch block in the current method that
	 * surrounds the current code position.  If "inCurrentMethod"
	 * is zero, then the exception should be thrown to the caller.
	 */
	void (*throwException)(ILCoder *coder, int inCurrentMethod);


	/*
	 * Set a stack trace. This is often used in conjunction with 
	 * throwException to mark exceptions with stack traces */
	void (*setStackTrace)(ILCoder *coder);

	/*
	 * Re-throw the current exception for a particular exception region.
	 */
	void (*rethrow)(ILCoder *coder, ILException *exception);

	/*
	 * Jump to a "finally" or "fault" sub-routine.
	 */
	void (*jsr)(ILCoder *coder, ILUInt32 dest);

	/*
	 * Return from a "finally" or "fault" sub-routine.
	 */
	void (*retFromJsr)(ILCoder *coder);

	/*
	 * Start a "try" handler block for a region of code.
	 */
	void (*tryHandlerStart)(ILCoder *coder, ILUInt32 start, ILUInt32 end);

	/*
	 * End the current "try" handler block.
	 */
	void (*tryHandlerEnd)(ILCoder *coder);

	/*
	 * Output instructions to match a "catch" clause.
	 * If "hasRethrow" is non-zero, then the method contains
	 * "rethrow" instructions and the object must be saved in
	 * a local variable before jumping to the catch clause.
	 */
	void (*catchClause)(ILCoder *coder, ILException *exception,
						ILClass *classInfo, int hasRethrow);
	
	/*
	 * End a catch or finally clause.  Finally clauses are usually always a
	 * potential exit point while catch clauses are a potential exit point
	 * if there are no finally clauses in the try/catch region.
	 */
	void (*endCatchFinallyClause)(ILCoder *coder, ILException *exception);

	void (*finallyClause)(ILCoder *coder, ILException *exception, int dest);

	/*
	 * Convert a program counter into an exception handler address.
	 * Returns NULL if no exception handler for the PC.  If "beyond"
	 * is zero, then the pc points to the start of an instruction.
	 * If "beyond" is non-zero, then the pc points beyond the end
	 * of an instruction.
	 */
	void *(*pcToHandler)(ILCoder *coder, void *pc, int beyond);

	/*
	 * Convert a program counter into an ILMethod descriptor.
	 * Returns NULL if the PC is not within a converted method.
	 * If "beyond" is zero, then the pc points to the start of
	 * an instruction.  If "beyond" is non-zero, then the pc
	 * points beyond the end of an instruction.
	 */
	ILMethod *(*pcToMethod)(ILCoder *coder, void *pc, int beyond);

	/*
	 * Get the IL offset that corresponds to a native offset
	 * within a method.  Returns IL_MAX_UINT32 if no IL offset.
	 */
	ILUInt32 (*getILOffset)(ILCoder *coder, void *start,
							ILUInt32 offset, int exact);

	/*
	 * Get the native offset that corresponds to an IL offset
	 * within a method.  Returns IL_MAX_UINT32 if no native offset.
	 */
	ILUInt32 (*getNativeOffset)(ILCoder *coder, void *start,
							    ILUInt32 offset, int exact);

	/*
	 * Mark an offset in the current method with IL offset information.
	 */
	void (*markBytecode)(ILCoder *coder, ILUInt32 offset);

	/*
	 * Mark the end of a method's code, just prior to exception tables.
	 */
	void (*markEnd)(ILCoder *coder);

	/*
	 * Set the different flags which enable or disable
	 * profiling, debugging etc.
	 */
	void (*setFlags)(ILCoder *coder, int flags);

	int (*getFlags)(ILCoder *coder);

	/*
	 * Allocate an extra local variable in the current method frame.
	 * Returns the local variable index.
	 */
	ILUInt32 (*allocExtraLocal)(ILCoder *coder, ILType *type);

	/*
	 * Push a thread value onto the stack for an internalcall.
	 */
	void (*pushThread)(ILCoder *coder, int useRawCalls);

	/*
	 * Load the address of an argument onto the native argument stack.
	 */
	void (*loadNativeArgAddr)(ILCoder *coder, ILUInt32 num);

	/*
	 * Load the address of a local onto the native argument stack.
	 */
	void (*loadNativeLocalAddr)(ILCoder *coder, ILUInt32 num);

	/*
	 * Start pushing arguments for a "libffi" call onto the stack.
	 */
	void (*startFfiArgs)(ILCoder *coder);

	/*
	 * Push the address of the raw argument block onto the stack.
	 */
	void (*pushRawArgPointer)(ILCoder *coder);

	/*
	 * Perform a function call using "libffi".
	 */
	void (*callFfi)(ILCoder *coder, void *fn, void *cif,
					int useRawCalls, int hasReturn);

	/*
	 * Check the top of stack value for NULL.
	 */
	void (*checkNull)(ILCoder *coder);

	/*
	 * Output an instruction to convert the top of stack according
	 * to a PInvoke marshalling rule.
	 */
	void (*convert)(ILCoder *coder, int opcode);

	/*
	 * Output an instruction to convert the top of stack according
	 * to a custom marshalling rule.
	 */
	void (*convertCustom)(ILCoder *coder, int opcode,
						  ILUInt32 customNameLen, ILUInt32 customCookieLen,
						  void *customName, void *customCookie);

	/*
	 * Sentinel string to catch missing methods in class tables.
	 */
	const char *sentinel;

};

/*
 * Helper macros for calling coder methods.
 */
#define	ILCoderCreate(classInfo,process,size,cachePageSize)	\
			((*((classInfo)->create))((process), (size), (cachePageSize)))
#define	ILCoderEnableDebug(coder)	\
			((*((coder)->classInfo->enableDebug))((coder)))
#define	ILCoderAlloc(coder,size)	\
			((*((coder)->classInfo->alloc))((coder), (size)))
#define	ILCoderGetCacheSize(coder)	\
			((*((coder)->classInfo->getCacheSize))((coder)))
#define	ILCoderSetup(coder,start,method,code) \
			((*((coder)->classInfo->setup))((coder), (start), (method), (code)))
#define	ILCoderSetupExtern(coder,start,method,fn,cif,isInternal) \
			((*((coder)->classInfo->setupExtern))((coder), (start), (method), \
												  (fn), (cif), (isInternal)))
#define	ILCoderSetupExternCtor(coder,start,method,fn,cif,ctorfn,ctorcif,isInternal) \
			((*((coder)->classInfo->setupExternCtor)) \
						((coder), (start), (method), \
						 (fn), (cif), (ctorfn), \
						 (ctorcif), (isInternal)))
#define	ILCoderCtorOffset(coder) \
			((*((coder)->classInfo->ctorOffset))((coder)))
#define	ILCoderDestroy(coder) \
			((*((coder)->classInfo->destroy))((coder)))
#define	ILCoderFinish(coder) \
			((*((coder)->classInfo->finish))((coder)))
#define	ILCoderLabel(coder,offset) \
			((*((coder)->classInfo->label))((coder), (offset)))
#define	ILCoderStackRefresh(coder,stack,stackSize) \
			((*((coder)->classInfo->stackRefresh))((coder), (stack), \
												   (stackSize)))
#define	ILCoderConstant(coder,opcode,arg) \
			((*((coder)->classInfo->constant))((coder), (opcode), (arg)))
#define	ILCoderStringConstant(coder,token,object) \
			((*((coder)->classInfo->stringConstant))((coder), (token), \
													 (object)))
#define	ILCoderBinary(coder,opcode,type1,type2) \
			((*((coder)->classInfo->binary))((coder), (opcode), (type1), \
											 (type2)))
#define	ILCoderBinaryPtr(coder,opcode,type1,type2) \
			((*((coder)->classInfo->binaryPtr))((coder), (opcode), \
												(type1), (type2)))
#define	ILCoderShift(coder,opcode,type1,type2) \
			((*((coder)->classInfo->shift))((coder), (opcode), \
											(type1), (type2)))
#define	ILCoderUnary(coder,opcode,type) \
			((*((coder)->classInfo->unary))((coder), (opcode), (type)))
#define	ILCoderLoadArg(coder,num,type)	\
			((*((coder)->classInfo->loadArg))((coder), (num), (type)))
#define	ILCoderStoreArg(coder,num,etype,type)	\
			((*((coder)->classInfo->storeArg))((coder), (num), \
											   (etype), (type)))
#define	ILCoderAddrOfArg(coder,num)	\
			((*((coder)->classInfo->addrOfArg))((coder), (num)))
#define	ILCoderLoadLocal(coder,num,type)	\
			((*((coder)->classInfo->loadLocal))((coder), (num), (type)))
#define	ILCoderStoreLocal(coder,num,etype,type)	\
			((*((coder)->classInfo->storeLocal))((coder), (num), \
												 (etype), (type)))
#define	ILCoderAddrOfLocal(coder,num)	\
			((*((coder)->classInfo->addrOfLocal))((coder), (num)))
#define	ILCoderDup(coder,etype,type)	\
			((*((coder)->classInfo->dup))((coder), (etype), (type)))
#define	ILCoderPop(coder,etype,type)	\
			((*((coder)->classInfo->pop))((coder), (etype), (type)))
#define	ILCoderArrayAccess(coder,opcode,itype,etype)	\
			((*((coder)->classInfo->arrayAccess))((coder), (opcode), \
												  (itype), (etype)))
#define	ILCoderPtrAccess(coder,opcode)	\
			((*((coder)->classInfo->ptrAccess))((coder), (opcode)))
#define	ILCoderPtrAccessManaged(coder,opcode,_classInfo)	\
			((*((coder)->classInfo->ptrAccessManaged))((coder), (opcode), \
													   (_classInfo)))
#define	ILCoderBranch(coder,opcode,dest,type1,type2)	\
			((*((coder)->classInfo->branch))((coder), (opcode), (dest), \
											 (type1), (type2)))
#define	ILCoderSwitchStart(coder,numEntries) \
			((*((coder)->classInfo->switchStart))((coder), (numEntries)))
#define	ILCoderSwitchEntry(coder,dest) \
			((*((coder)->classInfo->switchEntry))((coder), (dest)))
#define	ILCoderCompare(coder,opcode,type1,type2,invertTest)	\
			((*((coder)->classInfo->compare))((coder), (opcode), \
											  (type1), (type2), (invertTest)))
#define	ILCoderConv(coder,opcode,type) \
			((*((coder)->classInfo->conv))((coder), (opcode), (type)))
#define	ILCoderToPointer(coder,type1,type2) \
			((*((coder)->classInfo->toPointer))((coder), (type1), (type2)))
#define	ILCoderPtrPrefix(coder,alignment) \
			((*((coder)->classInfo->ptrPrefix))((coder), (alignment)))
#define	ILCoderArrayLength(coder) \
			((*((coder)->classInfo->arrayLength))((coder)))
#define	ILCoderNewArray(coder,arrayType,arrayClass,lengthType) \
			((*((coder)->classInfo->newArray))((coder), (arrayType), \
											   (arrayClass), (lengthType)))
#define	ILCoderLocalAlloc(coder,sizeType) \
			((*((coder)->classInfo->localAlloc))((coder), (sizeType)))
#define	ILCoderCastClass(coder,_classInfo,throwException) \
			((*((coder)->classInfo->castClass))((coder), (_classInfo), \
												(throwException)))
#define	ILCoderLoadField(coder,ptrType,objectType,field,fieldType) \
			((*((coder)->classInfo->loadField))((coder), (ptrType), \
												(objectType), (field), \
												(fieldType)))
#define	ILCoderLoadThisField(coder,field,fieldType) \
			((*((coder)->classInfo->loadThisField))((coder), (field), \
													(fieldType)))
#define	ILCoderLoadStaticField(coder,field,fieldType) \
			((*((coder)->classInfo->loadStaticField))((coder), (field), \
													  (fieldType)))
#define	ILCoderLoadFieldAddr(coder,ptrType,objectType,field,fieldType) \
			((*((coder)->classInfo->loadFieldAddr))((coder), (ptrType), \
												    (objectType), (field), \
												    (fieldType)))
#define	ILCoderLoadStaticFieldAddr(coder,field,fieldType) \
			((*((coder)->classInfo->loadStaticFieldAddr))((coder), (field), \
													      (fieldType)))
#define	ILCoderStoreField(coder,ptrType,objectType,field,fieldType,valueType) \
			((*((coder)->classInfo->storeField))((coder), (ptrType), \
												 (objectType), (field), \
												 (fieldType), (valueType)))
#define	ILCoderStoreStaticField(coder,field,fieldType,valueType) \
			((*((coder)->classInfo->storeStaticField))((coder), (field), \
											       (fieldType), (valueType)))
#define	ILCoderCopyObject(coder,destPtrType,srcPtrType,_classInfo) \
			((*((coder)->classInfo->copyObject))((coder), (destPtrType), \
											     (srcPtrType), (_classInfo)))
#define	ILCoderCopyBlock(coder,destPtrType,srcPtrType) \
			((*((coder)->classInfo->copyBlock))((coder), (destPtrType), \
											    (srcPtrType)))
#define	ILCoderInitObject(coder,ptrType,_classInfo) \
			((*((coder)->classInfo->initObject))((coder), (ptrType), \
											     (_classInfo)))
#define	ILCoderInitBlock(coder,ptrType) \
			((*((coder)->classInfo->initBlock))((coder), (ptrType)))
#define	ILCoderBox(coder,boxClass,valueType,size) \
			((*((coder)->classInfo->box))((coder), (boxClass), \
										  (valueType), (size)))
#define	ILCoderBoxSmaller(coder,boxClass,valueType,smallerType) \
			((*((coder)->classInfo->boxSmaller))((coder), (boxClass), \
										         (valueType), (smallerType)))
#define	ILCoderUnbox(coder,boxClass) \
			((*((coder)->classInfo->unbox))((coder), (boxClass)))
#define	ILCoderMakeTypedRef(coder,_classInfo) \
			((*((coder)->classInfo->makeTypedRef))((coder), (_classInfo)))
#define	ILCoderRefAnyVal(coder,_classInfo) \
			((*((coder)->classInfo->refAnyVal))((coder), (_classInfo)))
#define	ILCoderRefAnyType(coder) \
			((*((coder)->classInfo->refAnyType))((coder)))
#define	ILCoderPushToken(coder,item) \
			((*((coder)->classInfo->pushToken))((coder), (item)))
#define	ILCoderSizeOf(coder,_classInfo) \
			((*((coder)->classInfo->sizeOf))((coder), (_classInfo)))
#define	ILCoderArgList(coder) \
			((*((coder)->classInfo->argList))((coder)))
#define	ILCoderUpConvertArg(coder,stack,numParams,param,paramType) \
			((*((coder)->classInfo->upConvertArg))((coder), (stack), \
												   (numParams), (param), \
												   (paramType)))
#define	ILCoderDownConvertArg(coder,stack,numParams,param,paramType) \
			((*((coder)->classInfo->downConvertArg))((coder), (stack), \
												     (numParams), (param), \
												     (paramType)))
#define	ILCoderPackVarArgs(coder,callSiteSig,firstParam,args,numArgs) \
			((*((coder)->classInfo->packVarArgs))((coder), (callSiteSig), \
											      (firstParam), (args), \
											      (numArgs)))
#define	ILCoderValueCtorArgs(coder,_classInfo,args,numArgs) \
			((*((coder)->classInfo->valueCtorArgs))((coder), (_classInfo), \
												    (args), (numArgs)))
#define	ILCoderCheckCallNull(coder,info) \
			((*((coder)->classInfo->checkCallNull))((coder), (info)))
#define	ILCoderCallMethod(coder,info,returnItem,methodInfo) \
			((*((coder)->classInfo->callMethod))((coder), (info), \
												 (returnItem), \
												 (methodInfo)))
#define	ILCoderCallIndirect(coder,info,returnItem) \
			((*((coder)->classInfo->callIndirect))((coder), (info), \
												   (returnItem)))
#define	ILCoderCallCtor(coder,info,methodInfo) \
			((*((coder)->classInfo->callCtor))((coder), (info), \
											   (methodInfo)))
#define	ILCoderCallVirtual(coder,info,returnItem,methodInfo) \
			((*((coder)->classInfo->callVirtual))((coder), (info), \
												  (returnItem), \
												  (methodInfo)))
#define	ILCoderCallInterface(coder,info,returnItem,methodInfo) \
			((*((coder)->classInfo->callInterface))((coder), (info), \
												    (returnItem), \
													(methodInfo)))
#define	ILCoderCallInlineable(coder,inlineType,methodInfo) \
			((*((coder)->classInfo->callInlineable))((coder), (inlineType), \
													 (methodInfo)))
#define	ILCoderJumpMethod(coder,methodInfo) \
			((*((coder)->classInfo->jumpMethod))((coder), (methodInfo)))
#define	ILCoderReturnInsn(coder,engineType,returnType) \
			((*((coder)->classInfo->returnInsn))((coder), (engineType), \
												 (returnType)))
#define	ILCoderLoadFuncAddr(coder,methodInfo) \
			((*((coder)->classInfo->loadFuncAddr))((coder), (methodInfo)))
#define	ILCoderLoadVirtualAddr(coder,methodInfo) \
			((*((coder)->classInfo->loadVirtualAddr))((coder), (methodInfo)))
#define	ILCoderLoadInterfaceAddr(coder,methodInfo) \
			((*((coder)->classInfo->loadInterfaceAddr))((coder), (methodInfo)))
#define	ILCoderSetupExceptions(coder,exceptions,hasRethrow) \
			((*((coder)->classInfo->setupExceptions))((coder), (exceptions), \
													  (hasRethrow)))
#define	ILCoderThrow(coder,inCurrent) \
			((*((coder)->classInfo->throwException))((coder), (inCurrent)))
#define	ILCoderSetStackTrace(coder) \
			((*((coder)->classInfo->setStackTrace))((coder)))
#define	ILCoderRethrow(coder,exception) \
			((*((coder)->classInfo->rethrow))((coder), (exception)))
#define	ILCoderJsr(coder,dest) \
			((*((coder)->classInfo->jsr))((coder), (dest)))
#define	ILCoderRetFromJsr(coder) \
			((*((coder)->classInfo->retFromJsr))((coder)))
#define	ILCoderTryHandlerStart(coder,start,end) \
			((*((coder)->classInfo->tryHandlerStart))((coder), (start), (end)))
#define	ILCoderTryHandlerEnd(coder) \
			((*((coder)->classInfo->tryHandlerEnd))((coder)))
#define	ILCoderCatch(coder,exception,info,hasRethrow) \
			((*((coder)->classInfo->catchClause))((coder), (exception), \
												  (info), (hasRethrow)))
#define ILCoderEndCatchFinally(coder, exception) \
			((*((coder)->classInfo->endCatchFinallyClause))((coder), \
												  (exception)))
#define ILCoderFinally(coder, exception, dest) \
			((*((coder)->classInfo->finallyClause))((coder), (exception), (dest)))
#define	ILCoderPCToHandler(coder,pc,beyond) \
			((*((coder)->classInfo->pcToHandler))((coder), (pc), (beyond)))
#define	ILCoderPCToMethod(coder,pc,beyond) \
			((*((coder)->classInfo->pcToMethod))((coder), (pc), (beyond)))
#define	ILCoderGetILOffset(coder,start,offset,exact) \
			((*((coder)->classInfo->getILOffset))((coder), (start), \
												  (offset), (exact)))
#define	ILCoderGetNativeOffset(coder,start,offset,exact) \
			((*((coder)->classInfo->getNativeOffset))((coder), (start), \
												      (offset), (exact)))
#define	ILCoderMarkBytecode(coder,offset) \
			((*((coder)->classInfo->markBytecode))((coder), (offset)))
#define ILCoderSetFlags(coder,flags) \
			((*((coder)->classInfo->setFlags))((coder), (flags)))
#define ILCoderGetFlags(coder) \
			((*((coder)->classInfo->getFlags))((coder)))
#define	ILCoderAllocExtraLocal(coder,type) \
			((*((coder)->classInfo->allocExtraLocal))((coder), (type)))
#define	ILCoderPushThread(coder,useRawCalls) \
			((*((coder)->classInfo->pushThread))((coder), (useRawCalls)))
#define	ILCoderLoadNativeArgAddr(coder,num) \
			((*((coder)->classInfo->loadNativeArgAddr))((coder), (num)))
#define	ILCoderLoadNativeLocalAddr(coder,num) \
			((*((coder)->classInfo->loadNativeLocalAddr))((coder), (num)))
#define	ILCoderStartFfiArgs(coder) \
			((*((coder)->classInfo->startFfiArgs))((coder)))
#define	ILCoderPushRawArgPointer(coder) \
			((*((coder)->classInfo->pushRawArgPointer))((coder)))
#define	ILCoderCallFfi(coder,fn,cif,useRawCalls,hasReturn) \
			((*((coder)->classInfo->callFfi))((coder), (fn), (cif), \
											  (useRawCalls), (hasReturn)))
#define	ILCoderCheckNull(coder) \
			((*((coder)->classInfo->checkNull))((coder)))
#define	ILCoderConvert(coder,opcode) \
			((*((coder)->classInfo->convert))((coder), (opcode)))
#define	ILCoderConvertCustom(coder,opcode,customNameLen,customCookieLen,customName,customCookie) \
			((*((coder)->classInfo->convertCustom))((coder), (opcode), \
													(customNameLen), \
													(customCookieLen), \
													(customName), \
													(customCookie)))
#define	ILCoderMarkEnd(coder) \
			((*((coder)->classInfo->markEnd))((coder)))

#ifdef	__cplusplus
};
#endif

#endif	/* _IL_CODER_H */
