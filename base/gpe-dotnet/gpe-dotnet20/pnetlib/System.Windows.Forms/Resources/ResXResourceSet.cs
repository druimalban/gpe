/*
 * ResXResourceSet.cs - Implementation of the
 *			"System.Resources.ResXResourceSet" class. 
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

namespace System.Resources
{

#if !ECMA_COMPAT && CONFIG_SERIALIZATION

using System;
using System.IO;
using System.Collections;

public class ResXResourceSet : ResourceSet
{
	// Constructors.
	public ResXResourceSet(Stream stream)
			{
				Reader = new ResXResourceReader(stream);
				Table = new Hashtable();
				ReadResources();
			}
	public ResXResourceSet(String fileName)
			{
				Reader = new ResXResourceReader(fileName);
				Table = new Hashtable();
				ReadResources();
			}

	// Get the preferred reader and writer classes for this kind of set.
	public override Type GetDefaultReader()
			{
				return typeof(ResXResourceReader);
			}
	public override Type GetDefaultWriter()
			{
				return typeof(ResXResourceWriter);
			}

}; // class ResXResourceSet

#endif // !ECMA_COMPAT && CONFIG_SERIALIZATION

}; // namespace System.Resources
