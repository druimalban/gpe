/*
 * QueryContinueDragEventArgs.cs - Implementation of the
 *			"System.Windows.Forms.QueryContinueDragEventArgs" class.
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

#if !CONFIG_COMPACT_FORMS

using System.Runtime.InteropServices;

#if !ECMA_COMPAT
[ComVisible(true)]
#endif
public class QueryContinueDragEventArgs : EventArgs
{
	// Internal state.
	private int keyState;
	private bool escapePressed;
	private DragAction action;

	// Constructor.
	public QueryContinueDragEventArgs
				(int keyState, bool escapePressed, DragAction action)
			{
				this.keyState = keyState;
				this.escapePressed = escapePressed;
				this.action = action;
			}

	// Get this object's properties.
	public DragAction Action
			{
				get
				{
					return action;
				}
				set
				{
					action = value;
				}
			}
	public bool EscapePressed
			{
				get
				{
					return escapePressed;
				}
			}
	public int KeyState
			{
				get
				{
					return keyState;
				}
			}

}; // class QueryContinueDragEventArgs

#endif // !CONFIG_COMPACT_FORMS

}; // namespace System.Windows.Forms