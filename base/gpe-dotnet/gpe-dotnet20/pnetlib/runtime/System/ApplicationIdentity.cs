/*
 * ApplicationIdentity.cs - Implementation of "System.ApplicationIdentity".
 *
 * Copyright (C) 2004  Southern Storm Software, Pty Ltd.
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

namespace System
{

#if CONFIG_FRAMEWORK_2_0

[TODO]
public sealed class ApplicationIdentity
{
	// TODO

	// for use by Microsoft/Internal/Deployment/InternalApplicationIdentityHelper.cs
	internal Object id;

	private String fullName;

	public ApplicationIdentity(String applicationIdentityFullName)
	{
		// this is the excception thrown in the beta 1 of the SDK
		if(applicationIdentityFullName == null)
		{
			throw new NullReferenceException();
		}
		this.fullName = applicationIdentityFullName;
	}

	[TODO]
	public String CodeBase
	{
		get
		{
			return null;
		}
	}

	[TODO]
	public String FullName
	{
		get
		{
			return fullName;
		}
	}
}; // class ApplicationIdentity

#endif // CONFIG_FRAMEWORK_2_0

}; // namespace System
