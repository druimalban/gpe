/*
 * FUNCFLAGS.cs - Implementation of the
 *			"System.Runtime.InteropServices.ComTypes.FUNCFLAGS" class.
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

namespace System.Runtime.InteropServices.ComTypes
{

#if CONFIG_COM_INTEROP && CONFIG_FRAMEWORK_1_2

[ComVisible(false)]
[Serializable]
[Flags]
public enum FUNCFLAGS : short
{
	FUNCFLAG_FRESTRICTED		= 0x0001,
	FUNCFLAG_FSOURCE			= 0x0002,
	FUNCFLAG_FBINDABLE			= 0x0004,
	FUNCFLAG_FREQUESTEDIT		= 0x0008,
	FUNCFLAG_FDISPLAYBIND		= 0x0010,
	FUNCFLAG_FDEFAULTBIND		= 0x0020,
	FUNCFLAG_FHIDDEN			= 0x0040,
	FUNCFLAG_FUSESGETLASTERROR	= 0x0080,
	FUNCFLAG_FDEFAULTCOLLELEM	= 0x0100,
	FUNCFLAG_FUIDEFAULT			= 0x0200,
	FUNCFLAG_FNONBROWSABLE		= 0x0400,
	FUNCFLAG_FREPLACEABLE		= 0x0800,
	FUNCFLAG_FIMMEDIATEBIND		= 0x1000

}; // enum FUNCFLAGS

#endif // CONFIG_COM_INTEROP && CONFIG_FRAMEWORK_1_2

}; // namespace System.Runtime.InteropServices.ComTypes
