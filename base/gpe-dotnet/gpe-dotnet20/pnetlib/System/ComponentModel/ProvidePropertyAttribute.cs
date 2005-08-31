/*
 * ProvidePropertyAttribute.cs - Implementation of the
 *		"System.ComponentModel.ComponentModel.ProvidePropertyAttribute" class.
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

namespace System.ComponentModel
{

#if CONFIG_COMPONENT_MODEL

using System;

[AttributeUsage(AttributeTargets.Class, AllowMultiple=true)]
public sealed class ProvidePropertyAttribute : Attribute
{
	// Internal state.
	private String propertyName;
	private String receiverTypeName;

	// Constructors.
	public ProvidePropertyAttribute(String propertyName,
									String receiverTypeName)
			{
				this.propertyName = propertyName;
				this.receiverTypeName = receiverTypeName;
			}
	public ProvidePropertyAttribute(String propertyName,
									Type receiverType)
			{
				this.propertyName = propertyName;
				this.receiverTypeName = receiverType.AssemblyQualifiedName;
			}

	// Get this attribute's values.
	public String PropertyName
			{
				get
				{
					return propertyName;
				}
			}
	public String ReceiverTypeName
			{
				get
				{
					return receiverTypeName;
				}
			}
	public override Object TypeId
			{
				get
				{
					return GetType().FullName + propertyName;
				}
			}

	// Determine if two objects are equal.
	public override bool Equals(Object obj)
			{
				ProvidePropertyAttribute other =
					(obj as ProvidePropertyAttribute);
				if(other != null)
				{
					return (other.propertyName == propertyName &&
							other.receiverTypeName == receiverTypeName);
				}
				else
				{
					return false;
				}
			}

	// Get the hash code for this object.
	public override int GetHashCode()
			{
				return propertyName.GetHashCode() ^
					   receiverTypeName.GetHashCode();
			}

}; // class ProvidePropertyAttribute

#endif // CONFIG_COMPONENT_MODEL

}; // namespace System.ComponentModel
