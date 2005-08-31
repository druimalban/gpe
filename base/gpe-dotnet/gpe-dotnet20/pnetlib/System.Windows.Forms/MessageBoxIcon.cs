/*
 * MessageBoxIcon.cs - Implementation of the
 *			"System.Windows.Forms.MessageBoxIcon" class.
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

namespace System.Windows.Forms
{

public enum MessageBoxIcon
{
	None		= 0x0000,
	Hand		= 0x0010,
	Question	= 0x0020,
	Exclamation	= 0x0030,
	Asterisk	= 0x0040,
#if !CONFIG_COMPACT_FORMS
	Error		= Hand,
	Information	= Asterisk,
	Stop		= Hand,
	Warning		= Exclamation
#endif

}; // enum MessageBoxIcon

}; // namespace System.Windows.Forms
