/*
 * ApplicationCrmEnabledAttribute.cs - Implementation of the
 *			"System.EnterpriseServices.CompensatingResourceManager."
 *			"ApplicationCrmEnabledAttribute" class.
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

namespace System.EnterpriseServices.CompensatingResourceManager
{

using System.Runtime.InteropServices;

#if !ECMA_COMPAT
[ComVisible(false)]
#endif
#if CONFIG_COM_INTEROP
[ProgId("System.EnterpriseServices.Crm.ApplicationCrmEnabledAttribute")]
#endif
[AttributeUsage(AttributeTargets.Assembly, Inherited=true)]
public sealed class ApplicationCrmEnabledAttribute : Attribute
{
	// Internal state.
	private bool val;

	// Constructors.
	public ApplicationCrmEnabledAttribute() : this(true) {}
	public ApplicationCrmEnabledAttribute(bool val)
			{
				this.val = val;
			}

	// Get this attribute's value.
	public bool Value
			{
				get
				{
					return val;
				}
			}

}; // class ApplicationCrmEnabledAttribute

}; // namespace System.EnterpriseServices.CompensatingResourceManager
