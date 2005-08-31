/*
 * marshal.c - Determine how to marshal PInvoke method parameters.
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

#include "program.h"

#ifdef	__cplusplus
extern	"C" {
#endif

#ifdef IL_CONFIG_PINVOKE

/*
 * Determine the character set to use for strings.
 */
static ILUInt32 StringCharSet(ILPInvoke *pinvoke, ILMethod *method)
{
	/* Check the PInvoke record for character set information */
	if(pinvoke)
	{
		switch(pinvoke->member.attributes & IL_META_PINVOKE_CHAR_SET_MASK)
		{
			case IL_META_PINVOKE_CHAR_SET_ANSI:
			case IL_META_PINVOKE_CHAR_SET_AUTO:
			{
				return IL_META_MARSHAL_ANSI_STRING;
			}
			/* Not reached */
	
			case IL_META_PINVOKE_CHAR_SET_UNICODE:
			{
				return IL_META_MARSHAL_UTF8_STRING;
			}
			/* Not reached */
		}
	}

	/* Check the class for character set information */
	if(method && (method->member.owner->attributes &
						IL_META_TYPEDEF_STRING_FORMAT_MASK) ==
			IL_META_TYPEDEF_UNICODE_CLASS)
	{
		return IL_META_MARSHAL_UTF8_STRING;
	}
	else
	{
		return IL_META_MARSHAL_ANSI_STRING;
	}
}

/*
 * Extract a string from a custom marshaling declaration.
 */
#define	ExtractCustomString(name,namelen)	\
			do { \
				unsigned long size = ILMetaUncompressData(&reader); \
				*(name) = (char *)(reader.data); \
				*(namelen) = (int)size; \
				reader.data += size; \
				reader.len -= size; \
			} while (0)

ILUInt32 ILPInvokeGetMarshalType(ILPInvoke *pinvoke, ILMethod *method,
								 unsigned long param, char **customName,
								 int *customNameLen, char **customCookie,
								 int *customCookieLen, ILType *type)
{
	ILParameter *parameter;
	ILFieldMarshal *marshal;
	const unsigned char *nativeType;
	unsigned long nativeTypeLen;
	int nativeTypeCode;

	/* Find the parameter information block */
	if(method)
	{
		if(!(method->parameters))
		{
			_ILMethodLoadParams(method);
		}
		parameter = method->parameters;
		while(parameter != 0 && parameter->paramNum != param)
		{
			parameter = parameter->next;
		}
	}
	else
	{
		parameter = 0;
	}

	/* See if we have native type information for the parameter */
	nativeType = 0;
	nativeTypeLen = 0;
	nativeTypeCode = IL_META_NATIVETYPE_END;
	if(parameter != 0)
	{
		marshal = ILFieldMarshalGetFromOwner(&(parameter->programItem));
		if(marshal != 0)
		{
			nativeType = (const unsigned char *)
				ILFieldMarshalGetType(marshal, &nativeTypeLen);
			if(nativeTypeLen > 0)
			{
				nativeTypeCode = (int)(nativeType[0]);
			}
		}
	}

	/* Initialize the custom name return information to nothing */
	*customName = 0;
	*customNameLen = 0;
	*customCookie = 0;
	*customCookieLen = 0;

	/* If the native type is "interface", then always marshal directly.
	   This is normally used to force strings, delegates, and arrays
	   to be passed as objects */
	if(nativeTypeCode == IL_META_NATIVETYPE_INTF)
	{
		return IL_META_MARSHAL_DIRECT;
	}

	/* Check for custom marshalling */
	if(nativeTypeCode == IL_META_NATIVETYPE_CUSTOMMARSHALER &&
	   ILTypeIsReference(type))
	{
		ILMetaDataRead reader;
		reader.data = nativeType + 1;
		reader.len = nativeTypeLen - 1;
		reader.error = 0;
		ExtractCustomString(customName, customNameLen);	/* Unused GUID */
		ExtractCustomString(customName, customNameLen);	/* Unused native name */
		ExtractCustomString(customName, customNameLen);
		ExtractCustomString(customCookie, customCookieLen);
		if(*customNameLen > 0)
		{
			return IL_META_MARSHAL_CUSTOM;
		}
		else
		{
			return IL_META_MARSHAL_DIRECT;
		}
	}

	/* Determine what to do based on the parameter type */
	if(ILTypeIsStringClass(type))
	{
		/* Value string type */
		if(nativeTypeCode == IL_META_NATIVETYPE_LPWSTR)
		{
			return IL_META_MARSHAL_UTF16_STRING;
		}
		else
		{
			return StringCharSet(pinvoke, method);
		}
	}
	else if(ILTypeIsDelegateSubClass(type))
	{
		/* Delegate type */
		return IL_META_MARSHAL_FNPTR;
	}
	else if(ILType_IsSimpleArray(type))
	{
		if(ILTypeIsStringClass(ILTypeGetElemType(type)))
		{
			/* Array of strings, passed as "char **" */
			if(StringCharSet(pinvoke, method) == IL_META_MARSHAL_ANSI_STRING)
			{
				return IL_META_MARSHAL_ANSI_ARRAY;
			}
			else
			{
				return IL_META_MARSHAL_UTF8_ARRAY;
			}
		}
		else
		{
			/* Array type, passed in by pointer to the first element */
			return IL_META_MARSHAL_ARRAY;
		}
	}
	else if(type != 0 && ILType_IsComplex(type) &&
			ILType_Kind(type) == IL_TYPE_COMPLEX_BYREF)
	{
		/* Check for "ref String[]", which is used by Gtk# when
		   calling the "gtk_init" function */
		if(ILType_IsSimpleArray(ILType_Ref(type)) &&
		   ILTypeIsStringClass(ILTypeGetElemType(ILType_Ref(type))))
		{
			if(StringCharSet(pinvoke, method) == IL_META_MARSHAL_ANSI_STRING)
			{
				return IL_META_MARSHAL_REF_ANSI_ARRAY;
			}
			else
			{
				return IL_META_MARSHAL_REF_UTF8_ARRAY;
			}
		}
	}

	/* Marshal the parameter directly */
	return IL_META_MARSHAL_DIRECT;
}

#else	/* !IL_CONFIG_PINVOKE */

ILUInt32 ILPInvokeGetMarshalType(ILPInvoke *pinvoke, ILMethod *method,
								 unsigned long param, char **customName,
								 int *customNameLen, char **customCookie,
								 int *customCookieLen, ILType *type)
{
	return IL_META_MARSHAL_DIRECT;
}

#endif	/* !IL_CONFIG_PINVOKE */

#ifdef	__cplusplus
};
#endif
