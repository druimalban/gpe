/*
 * cs_attrs.c - Attribute handling.
 *
 * Copyright (C) 2002  Southern Storm Software, Pty Ltd.
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

#include "cs_internal.h"
#include <codegen/cg_nodemap.h>
#include "il_serialize.h"

#ifdef	__cplusplus
extern	"C" {
#endif

/*
 * Get the "field" target for a program item.
 */
static ILProgramItem *GetFieldTarget(ILGenInfo *info, ILProgramItem *item)
{
	ILEvent *event;
	ILNode_EventDeclarator *eventNode;

	/* If the item is already a field, then it is the requested target */
	if(ILProgramItemToField(item) != 0)
	{
		return item;
	}

	/* Determine if the item is a non-abstract event */
	event = ILProgramItemToEvent(item);
	if(event == 0)
	{
		return 0;
	}
	eventNode = (ILNode_EventDeclarator *)
		ILProgramItemToNode(info, ILToProgramItem(event));
	if(!eventNode)
	{
		return 0;
	}

	/* Return the backing field for the event, if there is one */
	return ILToProgramItem(eventNode->backingField);
}

/*
 * Get a particular "param" target for a method.
 */
static ILProgramItem *GetParamTarget(ILGenInfo *info, ILMethod *method,
									 unsigned long paramNum)
{
	ILParameter *param;

	/* If we are looking for the return value, then the
	   return type must not be "void" */
	if(paramNum == 0 &&
	   ILTypeGetReturn(ILMethod_Signature(method)) == ILType_Void)
	{
		return 0;
	}

	/* Look for a pre-existing parameter record */
	param = 0;
	while((param = ILMethodNextParam(method, param)) != 0)
	{
		if(ILParameter_Num(param) == paramNum)
		{
			return ILToProgramItem(param);
		}
	}

	/* Create a new parameter record */
	param = ILParameterCreate
		(method, 0, 0,
		 ((paramNum == 0) ? IL_META_PARAMDEF_RETVAL : 0),
	     (ILUInt32)paramNum);
	if(!param)
	{
		CCOutOfMemory();
	}
	return ILToProgramItem(param);
}

/*
 * Determine if a class is "AttributeUsageAttribute".
 */
static int IsAttributeUsage(ILClass *classInfo)
{
	const char *namespace;
	if(strcmp(ILClass_Name(classInfo), "AttributeUsageAttribute") != 0)
	{
		return 0;
	}
	namespace = ILClass_Namespace(classInfo);
	if(!namespace || strcmp(namespace, "System") != 0)
	{
		return 0;
	}
	return (ILClass_NestedParent(classInfo) == 0);
}

/*
 * Modify an attribute name node so that it uses ILNode_AttrQualIdent
 * or ILNode_AttrIdentifier for the top-level attribute lookup.
 */
static ILNode *ModifyAttrName(ILNode *node)
{
	ILNode *newNode;
	if(yyisa(node, ILNode_QualIdent))
	{
		newNode = ILNode_AttrQualIdent_create
			(((ILNode_QualIdent *)node)->left,
			 ((ILNode_QualIdent *)node)->right);
	}
	else if(yyisa(node, ILNode_Identifier))
	{
		newNode = ILNode_AttrIdentifier_create
			(((ILNode_Identifier *)node)->name);
	}
	else
	{
		return node;
	}
	yysetfilename(newNode, yygetfilename(node));
	yysetlinenum(newNode, yygetlinenum(node));
	return newNode;
}

/*
 * Look up a named field or property within an attribute type.
 */
static ILProgramItem *LookupAttrField(ILGenInfo *info, ILType *type,
									  ILNode *nameNode)
{
	char *name = ILQualIdentName(nameNode, 0);
	ILClass *classInfo = ILClassResolve(ILTypeToClass(info, type));
	ILClass *scope = ILClassLookup(ILClassGlobalScope(info->image),
							 	   "<Module>", (const char *)0);
	ILMember *member;
	ILMethod *setter;
	while(classInfo != 0)
	{
		member = 0;
		while((member = ILClassNextMemberMatch
				(classInfo, member, 0, name, 0)) != 0)
		{
			/* Skip members that aren't accessible to the module */
			if(!ILMemberAccessible(member, scope))
			{
				continue;
			}

			/* Return the field or property */
			if(ILMember_IsField(member))
			{
				/* The field must not be static */
				if(!ILField_IsStatic((ILField *)member))
				{
					return ILToProgramItem(member);
				}
			}
			else if(ILMember_IsProperty(member))
			{
				/* The property must have a one-argument setter, and
				   must not be static */
				setter = ILProperty_Setter((ILProperty *)member);
				if(setter && !ILMethod_IsStatic(setter) &&
				   ILTypeNumParams(ILMethod_Signature(setter)) == 1)
				{
					return ILToProgramItem(member);
				}
			}

			/* Method, event, or something else that is not usable */
			return 0;
		}
		classInfo = ILClass_Parent(classInfo);
	}
	return 0;
}

/*
 * Get the type that is associated with a field or property.
 */
static ILType *GetAttrFieldType(ILProgramItem *item)
{
	ILField *field = ILProgramItemToField(item);
	ILProperty *property = ILProgramItemToProperty(item);
	ILMethod *setter;
	if(field)
	{
		return ILField_Type(field);
	}
	else if(property)
	{
		setter = ILProperty_Setter(property);
		return ILTypeGetParam(ILMethod_Signature(setter), 1);
	}
	else
	{
		return ILType_Invalid;
	}
}

/*
 * Check if there are any circular semantic analysis cases .
 * The attribute target might not be distinct from the attribute
 * itself , like being a nested class or a member of the attribute.
 */
static int IsAttributeTargetDistinct(ILGenInfo *info, ILProgramItem *item,
					ILClass *classInfo)
{
	ILClass *target;
	if(ILToProgramItem(classInfo)==item)return 0;
	target= ILProgramItemToClass(item);
	if(target==NULL)
	{
		ILMember *member=ILProgramItemToMember(item);
		if(member)
		{
			target=ILMember_Owner(member);
		}
	}
	while(target!=NULL)
	{
		if(ILToProgramItem(classInfo)==ILToProgramItem(target))
		{
			return 0;
		}
		target=ILClass_NestedParent(target);
	}
	return 1;
}

/*
 * Convert a type into a name, formatted for use in attribute values.
 */
static const char *CSTypeToAttrName(ILGenInfo *info, ILType *type)
{
	ILClass *classInfo = ILTypeToClass(info, type);
	const char *name = ILClass_Name(classInfo);
	const char *namespace = ILClass_Namespace(classInfo);
	const char *finalName;
	if(namespace)
	{
		finalName = ILInternAppendedString
					(ILInternAppendedString
						(ILInternString((char *)namespace, -1),
						 ILInternString((char *)".", 1)),
					 ILInternString((char *)name, -1)).string;
	}
	else
	{
		finalName = name;
	}
	if(ILClass_NestedParent(classInfo) != 0)
	{
		/* Prepend the name of the enclosing nesting class */
		const char *parentName = CSTypeToAttrName
			(info, ILType_FromClass(ILClass_NestedParent(classInfo)));
		finalName = ILInternAppendedString
					(ILInternAppendedString
						(ILInternString((char *)parentName, -1),
						 ILInternString((char *)"+", 1)),
					 ILInternString((char *)finalName, -1)).string;
	}
	return finalName;
}

/* 
 * write an entry into the serialized stream using the provide paramType and
 * argValue and serialType. 
 */

static void WriteSerializedEntry(ILGenInfo *info,
								 ILSerializeWriter *writer, 
								 ILType *paramType,
								 ILEvalValue *argValue,
								 ILType *argType,
								 int serialType)
{	
	ILType *systemType=ILFindSystemType(info,"Type");

	switch(serialType)
	{
		case IL_META_SERIALTYPE_BOOLEAN:
		case IL_META_SERIALTYPE_I1:
		case IL_META_SERIALTYPE_U1:
		case IL_META_SERIALTYPE_I2:
		case IL_META_SERIALTYPE_U2:
		case IL_META_SERIALTYPE_CHAR:
		case IL_META_SERIALTYPE_I4:
		case IL_META_SERIALTYPE_U4:
		{
			ILSerializeWriterSetInt32(writer, argValue->un.i4Value,
									  serialType);
		}
		break;

		case IL_META_SERIALTYPE_I8:
		case IL_META_SERIALTYPE_U8:
		{
			ILSerializeWriterSetInt64(writer, argValue->un.i8Value);
		}
		break;

		case IL_META_SERIALTYPE_R4:
		{
			ILSerializeWriterSetFloat32(writer, argValue->un.r4Value);
		}
		break;

		case IL_META_SERIALTYPE_R8:
		{
			ILSerializeWriterSetFloat64(writer, argValue->un.r8Value);
		}
		break;

		case IL_META_SERIALTYPE_STRING:
		{
			if(argValue->valueType == ILMachineType_String)
			{
				ILSerializeWriterSetString(writer, argValue->un.strValue.str,
										   argValue->un.strValue.len);
			}
			else
			{
				ILSerializeWriterSetString(writer, 0, 0);
			}
		}
		break;

		case IL_META_SERIALTYPE_TYPE:
		{
			const char *name = CSTypeToAttrName
				(info, (ILType *)(argValue->un.strValue.str));
			ILSerializeWriterSetString(writer, name, strlen(name));
		}
		break;

		case IL_META_SERIALTYPE_VARIANT:
		{
			/* Note : We assume the values are castable and
			 * do not provide any checks here */
			if(ILType_IsPrimitive(argType))
			{
				switch(argValue->valueType)
				{	
					case ILMachineType_Boolean:
					case ILMachineType_Int8:
					case ILMachineType_UInt8:
					case ILMachineType_Int16:
					case ILMachineType_UInt16:
					case ILMachineType_Char:
					case ILMachineType_Int32:
					case ILMachineType_UInt32:
					case ILMachineType_Int64:
					case ILMachineType_UInt64:
					case ILMachineType_Float32:
					case ILMachineType_Float64:
					case ILMachineType_Decimal:
					{
						serialType=ILSerializeGetType(argType);
						
						ILSerializeWriterSetBoxedPrefix(writer, 
														   serialType);

						WriteSerializedEntry(info, writer, paramType, 
											 argValue, argType, serialType);
					}
					break;
					
					case ILMachineType_String:
					{
						/* TODO */
					}
					break;
					
					default:
					{
					}
					break;
				}
			}
			else if(ILTypeIdentical(argType, systemType))
			{ 
				ILSerializeWriterSetBoxedPrefix(writer,
										IL_META_SERIALTYPE_TYPE);
				
				WriteSerializedEntry(info, writer, paramType, 
									 argValue, argType,
									 IL_META_SERIALTYPE_TYPE);
			}
			else if(ILTypeIsEnum(argType))
			{
				const char *name = CSTypeToAttrName(info, (ILType *)(argType));
				ILSerializeWriterSetBoxedPrefix(writer,
										IL_META_SERIALTYPE_ENUM);
				ILSerializeWriterSetString(writer, name, strlen(name));

				serialType=ILSerializeGetType(argType);

				WriteSerializedEntry(info, writer, paramType,
										argValue, argType,
										serialType);	
			}
		}
		break;

		default:
		{
			if(ILType_IsArray(paramType))
			{
				/* TODO: arrays */
			}
		}
		break;
	}
}

/*
 * Process a single attribute in a section.
 */
static void ProcessAttr(ILGenInfo *info, ILProgramItem *item,
						int target, ILNode_Attribute *attr)
{
	ILType *type;
	ILClass *classInfo;
	ILNode *argList;
	ILNode *namedArgList;
	int numArgs, numNamedArgs, argNum;
	ILNode_ListIter iter;
	ILNode *arg;
	CSEvalArg *evalArgs;
	ILEvalValue *evalValues;
	CSEvalArg *namedArgs;
	ILEvalValue *namedValues;
	ILProgramItem **namedFields;
	CSSemValue value;
	int haveErrors;
	CSSemValue method;
	unsigned long itemNum;
	int candidateForm;
	ILProgramItem *itemInfo;
	ILMethod *methodInfo;
	ILType *signature;
	ILType *paramType;
	int allowMultiple;
	int inherited;
	ILSerializeWriter *writer = 0;
	ILEvalValue *argValue;
	int serialType;
	const void *blob;
	unsigned long blobLen;
	ILAttribute *attribute;
	int skipConst;
	ILType *argType;

	/* Modify the name so as to correctly handle "Attribute" suffixes,
	   and then perform semantic analysis on the type */
	attr->name = ModifyAttrName(attr->name);
	type = CSSemType(attr->name, info, &(attr->name));

	/* The type must inherit from "System.Attribute" and not be abstract */
	if(!ILTypeAssignCompatible
			(info->image, type, ILFindSystemType(info, "Attribute")))
	{
		CCErrorOnLine(yygetfilename(attr), yygetlinenum(attr),
					  _("`%s' does not inherit from `System.Attribute'"),
					  CSTypeToName(type));
		return;
	}
	classInfo = ILTypeToClass(info, type);
	if(ILClass_IsAbstract(classInfo))
	{
		CCErrorOnLine(yygetfilename(attr), yygetlinenum(attr),
			  _("cannot use the abstract type `%s' as an attribute name"),
			  CSTypeToName(type));
		return;
	}

	/* Check that that the attribute can be applied to this kind of target.
	   We use a different algorithm for "AttributeUsageAttribute", to avoid
	   circularities in the semantic analysis network */
	if(!IsAttributeUsage(classInfo))
	{
		/* Perform semantic analysis on the attribute type, but only
		   if we aren't trying to apply the attribute to itself */
		if(IsAttributeTargetDistinct(info,item,classInfo))
		{
			CSSemProgramItem(info, ILToProgramItem(classInfo));
		}

		/* Get the usage information for the attribute */
		allowMultiple = 1;
		inherited = 1;
	}
	else
	{
		/* We can only use "AttributeUsageAttribute" on classes
		   that inherit from "System.Attribute" */
		classInfo = ILProgramItemToClass(item);
		if(!classInfo)
		{
			CCErrorOnLine(yygetfilename(attr), yygetlinenum(attr),
		     _("`System.AttributeUsageAttribute' may only be used on classes"));
			return;
		}
		if(!ILTypeAssignCompatible(info->image, ILClassToType(classInfo),
		   						   ILFindSystemType(info, "Attribute")))
		{
			CCErrorOnLine(yygetfilename(attr), yygetlinenum(attr),
						  _("`%s' does not inherit from `System.Attribute'"),
						  CSTypeToName(ILClassToType(classInfo)));
			return;
		}
		allowMultiple = 0;
		inherited = 1;
	}

	/* Check the "AllowMultiple" and "Inherited" states of the attribute */
	/* TODO */

	/* Perform semantic analysis on the positional attributes */
	if(attr->args)
	{
		argList = ((ILNode_AttrArgs *)(attr->args))->positionalArgs;
	}
	else
	{
		argList = 0;
	}
	numArgs = ILNode_List_Length(argList);
	haveErrors = 0;
	if(numArgs)
	{
		evalArgs = (CSEvalArg *)ILMalloc(sizeof(CSEvalArg) * numArgs);
		if(!evalArgs)
		{
			CCOutOfMemory();
		}
		evalValues = (ILEvalValue *)ILMalloc(sizeof(ILEvalValue) * numArgs);
		if(!evalValues)
		{
			CCOutOfMemory();
		}
		ILNode_ListIter_Init(&iter, argList);
		argNum = 0;
		while((arg = ILNode_ListIter_Next(&iter)) != 0)
		{
			/* Perform semantic analysis on the argument to get the type.
			   Because the argument is wrapped in "ToConst", we don't
			   have to worry about reporting errors here */
			if(!CSSemExpectValue(arg, info, iter.last, &value))
			{
				if(!CSSemIsType(value))
				{
					haveErrors = 1;
					evalArgs[argNum].type = ILType_Int32;
				}
			}
			else
			{
				evalArgs[argNum].type = CSSemGetType(value);
			}
			evalArgs[argNum].node = *(iter.last);
			evalArgs[argNum].parent = iter.last;
			evalArgs[argNum].modifier = ILParamMod_empty;

			/* Evaluate the constant value of the argument */
			if(CSSemIsType(value))
			{
				evalValues[argNum].valueType = ILMachineType_Void;
				evalValues[argNum].un.oValue = CSSemGetType(value);
				evalArgs[argNum].type = ILFindSystemType(info, "Type");
			}
			else
			{
				if(!haveErrors &&
				   !ILNode_EvalConst(*(iter.last), info, &(evalValues[argNum])))
				{
					haveErrors = 1;
				}
			}

			/* Advance to the next argument */
			++argNum;
		}
	}
	else
	{
		evalArgs = 0;
		evalValues = 0;
	}

	/* Perform semantic analysis on the named arguments */
	if(attr->args)
	{
		namedArgList = ((ILNode_AttrArgs *)(attr->args))->namedArgs;
	}
	else
	{
		namedArgList = 0;
	}
	numNamedArgs = ILNode_List_Length(namedArgList);
	if(numNamedArgs)
	{
		namedArgs = (CSEvalArg *)ILMalloc(sizeof(CSEvalArg) * numNamedArgs);
		if(!namedArgs)
		{
			CCOutOfMemory();
		}
		namedValues = (ILEvalValue *)ILMalloc
			(sizeof(ILEvalValue) * numNamedArgs);
		if(!namedValues)
		{
			CCOutOfMemory();
		}
		namedFields = (ILProgramItem **)ILMalloc
			(sizeof(ILProgramItem *) * numNamedArgs);
		if(!namedFields)
		{
			CCOutOfMemory();
		}
		ILNode_ListIter_Init(&iter, namedArgList);
		argNum = 0;
		while((arg = ILNode_ListIter_Next(&iter)) != 0)
		{
			/* Convert the name into a field or property */
			if((namedFields[argNum] = LookupAttrField
					(info, type, ((ILNode_NamedArg *)arg)->name)) == 0)
			{
				CCErrorOnLine
					(yygetfilename(arg), yygetlinenum(arg),
				     "`%s' is not a valid named argument for `%s'",
				     ILQualIdentName(((ILNode_NamedArg *)arg)->name, 0),
				     CSTypeToName(type));
				haveErrors = 1;
			}

			/* Perform semantic analysis on the argument to get the type.
			   Because the argument is wrapped in "ToConst", we don't
			   have to worry about reporting errors here */
			if(!CSSemExpectValue(((ILNode_NamedArg *)arg)->value, info,
								 &(((ILNode_NamedArg *)arg)->value), &value))
			{
				if(!CSSemIsType(value))
				{
					haveErrors = 1;
					namedArgs[argNum].type = ILType_Int32;
				}
			}
			else
			{
				namedArgs[argNum].type = CSSemGetType(value);
			}
			namedArgs[argNum].node = ((ILNode_NamedArg *)arg)->value;
			namedArgs[argNum].parent = &(((ILNode_NamedArg *)arg)->value);
			namedArgs[argNum].modifier = ILParamMod_empty;

			/* Evaluate the constant value of the argument */
			if(CSSemIsType(value))
			{
				namedValues[argNum].valueType = ILMachineType_Void;
				namedValues[argNum].un.oValue = CSSemGetType(value);
				namedArgs[argNum].type = ILFindSystemType(info, "Type");
				skipConst = 1;
			}
			else
			{
				if(!haveErrors &&
				   !ILNode_EvalConst(namedArgs[argNum].node, info,
				   					 &(namedValues[argNum])))
				{
					haveErrors = 1;
				}
				skipConst = 0;
			}

			/* Cast the constant to the final type */
			if(!haveErrors)
			{
				if(ILCoerce(info, namedArgs[argNum].node,
							namedArgs[argNum].parent,
							namedArgs[argNum].type,
							GetAttrFieldType(namedFields[argNum]),1) &&
				   (skipConst || ILGenCastConst
				   		(info, &(namedValues[argNum]),
				   		 namedValues[argNum].valueType,
						 ILTypeToMachineType
								(GetAttrFieldType(namedFields[argNum])))))
				{
					namedArgs[argNum].node = *(namedArgs[argNum].parent);
				}
				else
				{
					CCErrorOnLine(yygetfilename(namedArgs[argNum].node),
								  yygetlinenum(namedArgs[argNum].node),
								  "cannot coerce from `%s' to `%s'",
								  CSTypeToName(namedArgs[argNum].type),
								  CSTypeToName(GetAttrFieldType
								  		(namedFields[argNum])));
					haveErrors = 1;
				}
			}

			/* Advance to the next argument */
			++argNum;
		}
	}
	else
	{
		namedArgs = 0;
		namedValues = 0;
		namedFields = 0;
	}

	/* Bail out if we had errors during analysis of the arguments */
	if(haveErrors)
	{
		goto cleanup;
	}

	/* Resolve the constructors in the attribute type */
	method = CSResolveConstructor(info, (ILNode *)attr, type);
	if(!CSSemIsMethodGroup(method))
	{
		CCErrorOnLine(yygetfilename(attr), yygetlinenum(attr),
					  "`%s' does not have an accessible constructor",
					  CSTypeToName(type));
		goto cleanup;
	}

	/* Find the set of candidate methods */
	itemNum = 0;
	while((itemInfo = CSGetGroupMember(CSSemGetGroup(method), itemNum)) != 0)
	{
		candidateForm = CSItemIsCandidate(info, itemInfo, evalArgs, numArgs);
		if(candidateForm)
		{
			CSSetGroupMemberForm(CSSemGetGroup(method), itemNum,
								 candidateForm);
			++itemNum;
		}
		else
		{
			CSSemModifyGroup
				(method, CSRemoveGroupMember(CSSemGetGroup(method), itemNum));
		}
	}

	/* If there are no candidates left, then bail out */
	itemNum = 0;
	itemInfo = CSGetGroupMember(CSSemGetGroup(method), itemNum);
	if(!itemInfo)
	{
		CSItemCandidateError((ILNode *)attr, 0, 1,
						     CSSemGetGroup(method), evalArgs, numArgs);
		goto cleanup;
	}

	/* There are two or more candidates, then try to find the best one */
	if(CSGetGroupMember(CSSemGetGroup(method), 1) != 0)
	{
		itemInfo = CSBestCandidate(info, CSSemGetGroup(method),
								   evalArgs, numArgs);
		if(!itemInfo)
		{
			CSItemCandidateError((ILNode *)attr, 0, 1,
							     CSSemGetGroup(method), evalArgs, numArgs);
			goto cleanup;
		}
	}

	/* Import the constructor method into this image */
	methodInfo = (ILMethod *)ILMemberImport
						(info->image, (ILMember *)itemInfo);
	if(!methodInfo)
	{
		CCOutOfMemory();
	}

	/* Coerce the positional arguments to their final types */
	signature = ILMethod_Signature(methodInfo);
	haveErrors = 0;
	for(argNum = 0; argNum < numArgs; ++argNum)
	{
		paramType = ILTypeGetParam(signature, argNum + 1);
		if(evalValues[argNum].valueType != ILMachineType_Void)
		{
			if(!ILGenCastConst(info, &(evalValues[argNum]),
				   evalValues[argNum].valueType,ILTypeToMachineType(paramType))
			   && !ILCanCastKind(info, evalArgs[argNum].type,
								paramType, IL_CONVERT_STANDARD,0))
			{
								
				CCErrorOnLine(yygetfilename(evalArgs[argNum].node),
							  yygetlinenum(evalArgs[argNum].node),
							  _("could not coerce constant argument %d"),
							  argNum + 1);
				haveErrors = 1;
			}
			else if(ILSerializeGetType(paramType) == -1)
			{
				CCErrorOnLine(yygetfilename(evalArgs[argNum].node),
							  yygetlinenum(evalArgs[argNum].node),
							  _("attribute argument %d is not serializable"),
							  argNum + 1);
				haveErrors = 1;
			}
		}
	}
	if(haveErrors)
	{
		goto cleanup;
	}

	/* Build the serialized attribute value */
	writer = ILSerializeWriterInit();
	if(!writer)
	{
		CCOutOfMemory();
	}
	for(argNum = 0; argNum < numArgs; ++argNum)
	{
		paramType = ILTypeGetParam(signature, argNum + 1);
		argValue = &(evalValues[argNum]);
		argType = evalArgs[argNum].type;
		serialType = ILSerializeGetType(paramType);
		WriteSerializedEntry(info,writer,paramType,argValue,argType,serialType);
	}
	ILSerializeWriterSetNumExtra(writer, numNamedArgs);
	for(argNum = 0; argNum < numNamedArgs; ++argNum)
	{
		argValue = &(namedValues[argNum]);
		if(argValue->valueType == ILMachineType_Void)
		{
			serialType = IL_META_SERIALTYPE_TYPE;
			paramType = NULL;
		}
		else
		{
			paramType = ILValueTypeToType(info, argValue->valueType);
			serialType = ILSerializeGetType(paramType);
		}
		if(ILMember_IsField(((ILMember *)(namedFields[argNum]))))
		{
			ILSerializeWriterSetField
				(writer, ILMember_Name((ILMember *)(namedFields[argNum])),
				 serialType);
		}
		else
		{
			ILSerializeWriterSetProperty
				(writer, ILMember_Name((ILMember *)(namedFields[argNum])),
				 serialType);
		}
		argType=namedArgs[argNum].type;
		WriteSerializedEntry(info,writer,paramType,argValue,argType,serialType);
	}
	blob = ILSerializeWriterGetBlob(writer, &blobLen);
	if(!blob)
	{
		CCOutOfMemory();
	}

	/* Add the attribute value to the program item */
	attribute = ILAttributeCreate(info->image, 0);
	if(!attribute)
	{
		CCOutOfMemory();
	}
	ILAttributeSetType(attribute, ILToProgramItem(methodInfo));
	if(!ILAttributeSetValue(attribute, blob, blobLen))
	{
		CCOutOfMemory();
	}
	ILProgramItemAddAttribute(item, attribute);

cleanup:
	if(evalArgs)
	{
		ILFree(evalArgs);
	}
	if(evalValues)
	{
		ILFree(evalValues);
	}
	if(namedArgs)
	{
		ILFree(namedArgs);
	}
	if(namedValues)
	{
		ILFree(namedValues);
	}
	if(namedFields)
	{
		ILFree(namedFields);
	}
	if(writer)
	{
		ILSerializeWriterDestroy(writer);
	}
}

void CSProcessAttrs(ILGenInfo *info, ILProgramItem *mainItem,
					ILNode *attributes, int mainTarget)
{
	ILProgramItem *item;
	int target;
	ILNode_ListIter iter;
	ILNode_ListIter iter2;
	ILNode_AttributeSection *section;
	ILNode_Attribute *attr;
	char *targetName;
	ILClass *classInfo;
	ILMethod *method;
	unsigned long numParams;

	/* Bail out if we don't have any attributes */
	if(!attributes)
	{
		return;
	}

	/* Scan through the attribute sections */
	ILNode_ListIter_Init(&iter, ((ILNode_AttributeTree *)attributes)
										->sections);
	while((section = (ILNode_AttributeSection *)
				ILNode_ListIter_Next(&iter)) != 0)
	{
		/* Skip documentation comments, if present */
		if(yyisa(section, ILNode_DocComment))
		{
			continue;
		}

		/* Resolve the target item */
		item = mainItem;
		target = mainTarget;
		switch(section->type)
		{
			case ILAttrTargetType_None:		break;

			case ILAttrTargetType_Named:
			{
				targetName = ILQualIdentName(section->target, 0);
				if(!strcmp(targetName, "assembly"))
				{
					/* Assembly targets can be applied anywhere */
					item = (ILProgramItem *)ILAssembly_FromToken
						(ILProgramItem_Image(item), IL_META_TOKEN_ASSEMBLY | 1);
					target = CS_ATTR_ASSEMBLY;
				}
				else if(!strcmp(targetName, "module"))
				{
					/* Module targets can be applied anywhere */
					item = (ILProgramItem *)ILModule_FromToken
						(ILProgramItem_Image(item), IL_META_TOKEN_MODULE | 1);
					target = CS_ATTR_MODULE;
				}
				else if(!strcmp(targetName, "field"))
				{
					/* Field targets can apply to fields or events */
					item = GetFieldTarget(info, item);
					target = CS_ATTR_FIELD;
				}
				else if(!strcmp(targetName, "method"))
				{
					/* Method targets can apply to methods,
					   constructors, or operators */
					if(ILProgramItemToMethod(item) == 0)
					{
						item = 0;
					}
				}
				else if(!strcmp(targetName, "param"))
				{
					/* Parameter targets can apply to parameter records,
					   or to the first parameter of a method, event accessor,
					   or property set accessor */
					method = ILProgramItemToMethod(item);
					if(method != 0)
					{
						numParams = ILTypeNumParams(ILMethod_Signature(method));
						if(numParams == 1)
						{
							item = GetParamTarget(info, method, 1);
							target = CS_ATTR_PARAMETER;
						}
						else
						{
							item = 0;
						}
					}
					else if(ILProgramItemToParameter(item) == 0)
					{
						item = 0;
					}
				}
				else if(!strcmp(targetName, "property"))
				{
					/* Property targets can only apply to properties */
					if(ILProgramItemToProperty(item) == 0)
					{
						item = 0;
					}
				}
				else if(!strcmp(targetName, "type"))
				{
					/* Type targets can apply to classes, structures,
					   enumerated types, and delegates */
					if(ILProgramItemToClass(item) == 0)
					{
						item = 0;
					}
				}
				else
				{
					CCErrorOnLine(yygetfilename(section),
								  yygetlinenum(section),
								  _("invalid attribute target type `%s'"),
								  targetName);
					continue;
				}
				if(!item)
				{
					CCErrorOnLine
						(yygetfilename(section),
						 yygetlinenum(section),
						 _("attribute target type `%s' is not appropriate "
						   "in this context"), targetName);
					continue;
				}
			}
			break;

			case ILAttrTargetType_Event:
			{
				/* Event targets can only apply to events */
				if(ILProgramItemToEvent(item) == 0)
				{
					CCErrorOnLine
						(yygetfilename(section),
						 yygetlinenum(section),
						 _("attribute target type `event' is not appropriate "
						   "in this context"));
					continue;
				}
			}
			break;

			case ILAttrTargetType_Return:
			{
				/* Return targets can apply to methods, operators,
				   and delegates that return non-void */
				if(ILProgramItemToMethod(item) != 0)
				{
					item = GetParamTarget
						(info, ILProgramItemToMethod(item), 0);
				}
				else if((classInfo = ILProgramItemToClass(item)) != 0 &&
				        ILTypeIsDelegate(ILType_FromClass(classInfo)))
				{
					item = GetParamTarget
						(info, ILTypeGetDelegateMethod
									(ILType_FromClass(classInfo)), 0);
				}
				else
				{
					item = 0;
				}
				if(!item)
				{
					CCErrorOnLine
						(yygetfilename(section),
						 yygetlinenum(section),
						 _("attribute target type `return' is not appropriate "
						   "in this context"));
					continue;
				}
				target = CS_ATTR_RETURNVALUE;
			}
			break;
		}

		/* Process the attributes in this section */
		ILNode_ListIter_Init(&iter2, section->attrs);
		while((attr = (ILNode_Attribute *)ILNode_ListIter_Next(&iter2)) != 0)
		{
			ProcessAttr(info, item, target, attr);
		}

		/* Convert system library attributes into metadata structures */
		if(!ILProgramItemConvertAttrs(item))
		{
			CCOutOfMemory();
		}
	}
}

void CSProcessAttrsForParam(ILGenInfo *info, ILMethod *method,
							unsigned long paramNum,
							ILNode *attributes)
{
	ILProgramItem *item;

	/* Bail out if there are no parameter attributes */
	if(!attributes)
	{
		return;
	}

	/* Locate the parameter record on the method */
	item = GetParamTarget(info, method, paramNum);
	if(!item)
	{
		/* Shouldn't happen, but do something sane anyway just in case */
		return;
	}

	/* Process the attributes for the parameter */
	return CSProcessAttrs(info, item, attributes, CS_ATTR_PARAMETER);
}

void CSAddDefaultMemberAttr(ILGenInfo *info, ILClass *classInfo,
							const char *name)
{
	ILType *type;
	ILClass *typeInfo;
	ILMethod *ctor;
	ILType *args[1];
	ILSerializeWriter *writer;
	const void *blob;
	unsigned long blobLen;
	ILAttribute *attribute;

	/* Find the constructor for "DefaultMemberAttribute" */
	type = ILFindNonSystemType(info, "DefaultMemberAttribute",
							   "System.Reflection");
	if(!type || !ILType_IsClass(type))
	{
		return;
	}
	typeInfo = ILClassResolve(ILType_ToClass(type));
	args[0] = ILFindSystemType(info, "String");
	ctor = ILResolveConstructor(info, typeInfo, classInfo, args, 1);
	if(!ctor)
	{
		return;
	}

	/* Import the constructor method into this image */
	ctor = (ILMethod *)ILMemberImport(info->image, (ILMember *)ctor);
	if(!ctor)
	{
		CCOutOfMemory();
	}

	/* Build the attribute value blob */
	writer = ILSerializeWriterInit();
	if(!writer)
	{
		CCOutOfMemory();
	}
	ILSerializeWriterSetString(writer, name, strlen(name));
	ILSerializeWriterSetNumExtra(writer, 0);
	blob = ILSerializeWriterGetBlob(writer, &blobLen);
	if(!blob)
	{
		CCOutOfMemory();
	}

	/* Attach the attribute to the class */
	attribute = ILAttributeCreate(info->image, 0);
	if(!attribute)
	{
		CCOutOfMemory();
	}
	ILAttributeSetType(attribute, ILToProgramItem(ctor));
	if(!ILAttributeSetValue(attribute, blob, blobLen))
	{
		CCOutOfMemory();
	}
	ILProgramItemAddAttribute(ILToProgramItem(classInfo), attribute);

	/* Clean up and exit */
	ILSerializeWriterDestroy(writer);
}

#ifdef	__cplusplus
};
#endif
