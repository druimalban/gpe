/*
 * ScrollableControl.cs - Implementation of the
 *			"System.Windows.Forms.ScrollableControl" class.
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

using System.Drawing;
using System.ComponentModel;
using System.Drawing.Toolkit;

#if CONFIG_COMPONENT_MODEL
[Designer("System.Windows.Forms.Design.ScrollableControlDesigner, System.Design")]
#endif
public class ScrollableControl : Control
{
	// Internal state.
	private bool autoScroll=false;
	private bool hscroll=true;
	private bool vscroll=true;
	private Size autoScrollMargin=new Size(0,0);
	private Size autoScrollMinSize=new Size(0,0);

	// An assumption has been made about this co-ordinate in that
	// it serves as the origin of the screen as per the display
	// rectangle and will always be positive during scrolling
	private Point autoScrollPosition=new Point(0,0);
	private DockPaddingEdges dockPadding;
	private ScrollBar vScrollBar;
   	private ScrollBar hScrollBar;

	// Constructor.
	public ScrollableControl()
			{
				base.SetStyle(ControlStyles.ContainerControl, true);
				HandleCreated+=new EventHandler(ScrollableControl_HandleCreated);
			}

	protected virtual void AdjustFormScrollbars(bool displayScrollbars)
			{
				UpdateScrollBars(); 
			}

	// Get or set this control's properties.
	public virtual bool AutoScroll
			{
				get
				{
					return autoScroll;
				}
				set
				{
					if (autoScroll == value)
						return;
					autoScroll = value;
					if (autoScroll)
					{
						if (IsHandleCreated)
							CreateScrollBars();
					}
					else
					{
						vScrollBar.Dispose();
						vScrollBar = null;
						hScrollBar.Dispose();
						hScrollBar = null;
					}
				}
			}

	public Size AutoScrollMargin
			{
				get
				{
					return autoScrollMargin;
				}
				set
				{
					if(value.Width < 0)
					{
						throw new ArgumentOutOfRangeException
							("value.Width", S._("SWF_NonNegative"));
					}
					if(value.Height < 0)
					{
						throw new ArgumentOutOfRangeException
							("value.Height", S._("SWF_NonNegative"));
					}
					autoScrollMargin = value;
				}
			}
	public Size AutoScrollMinSize
			{
				get
				{
					return autoScrollMinSize;
				}
				set
				{
					autoScrollMinSize = value;
				}
			}
	public Point AutoScrollPosition
			{
				get
				{
					return autoScrollPosition;
				}
				set
				{
					autoScrollPosition = value;
				}
			}
	protected override CreateParams CreateParams
			{
				get
				{
					return base.CreateParams;
				}
			}
	public override Rectangle DisplayRectangle
			{
				get
				{
					// subtract the scroll bars from the DisplayRectangle
					Rectangle displayRect = base.DisplayRectangle;
					Rectangle scrollArea = ScrollArea;
					bool vert, horiz;
					
					vert = autoScroll && vscroll && 
							((displayRect.Height + autoScrollMargin.Height) 
										< scrollArea.Height);

					horiz = autoScroll && hscroll && 
							((displayRect.Width + autoScrollMargin.Width)
										< scrollArea.Width);
					
					// Note: for all the people who wonder about the
					// following expression , it's the *optimised* version
					// aka obfuscated
					//	vert = vert && (!horiz || (displayRect.Height < (scrollArea.Height + hScrollBar.Height)));
					// horiz = horiz && (!vert || (displayRect.Width < (scrollArea.Width + vScrollBar.Width)));
					// I need to find out if this compiles to better
					// code of the clean version or not :)
					
					if(horiz)
					{
						vert = vert && (displayRect.Width < (scrollArea.Width+vScrollBar.Width));
					}

					if(vert)
					{
						horiz = horiz && (displayRect.Height < (scrollArea.Height+hScrollBar.Height));
					}

					// I could keep doing this on and on .... but two
					// iterations and that's it. Someday some dude is
					// going to put a loop over it and figure out how
					// to calculate this to perfection.
					
					Size scrollbarsize=new Size(
									vert ? vScrollBar.Width : 0,
									horiz ? hScrollBar.Height : 0);
					displayRect.Size-=scrollbarsize;
					return displayRect;
				}
			}
	public DockPaddingEdges DockPadding
			{
				get
				{
					if (dockPadding == null)
					{
						dockPadding = new DockPaddingEdges(this);
					}
					return dockPadding;
				}
				set
				{
					dockPadding = value;
				}
			}
	protected bool HScroll
			{
				get
				{
					return hscroll;
				}
				set
				{
					hscroll = value;
				}
			}
	protected bool VScroll
			{
				get
				{
					return vscroll;
				}
				set
				{
					vscroll = value;
				}
			}

	// Set the auto scroll margin.
	public void SetAutoScrollMargin(int x, int y)
			{
				if(x < 0)
				{
					x = 0;
				}
				if(y < 0)
				{
					y = 0;
				}
				AutoScrollMargin = new Size(x, y);
			}

	private void UpdateScrollBars()
			{
				Rectangle rect = DisplayRectangle;
				vScrollBar.SetBounds(rect.Right, 0, vScrollBar.Width, rect.Height);
				hScrollBar.SetBounds(0, rect.Bottom, rect.Width, hScrollBar.Height);

				if(DisplayRectangle.Height >= ScrollArea.Height)
				{
					vScrollBar.Visible = false;
				}
				else
				{
					vScrollBar.LargeChange = DisplayRectangle.Height;
					vScrollBar.SmallChange = (DisplayRectangle.Height + 9 )/ 10;
					vScrollBar.Value = -(autoScrollPosition.Y);
				/* note: I don't exactly remember how I got this , but
				 * it seemed to work sometime near 8 Pm ... so I left it
				 * in */
					vScrollBar.Maximum = ScrollArea.Height - 1;
					vScrollBar.Visible = vscroll;					
				}	

				if(DisplayRectangle.Width >= ScrollArea.Width)
				{
					hScrollBar.Visible = false;
				}
				else
				{
					hScrollBar.LargeChange = DisplayRectangle.Width;			
					hScrollBar.SmallChange = (DisplayRectangle.Width + 9) / 10;
					hScrollBar.Value = -(autoScrollPosition.X);
					hScrollBar.Maximum = ScrollArea.Width - 1;
					hScrollBar.Visible = hscroll;
				}
			}

	protected override void SetBoundsCore(int x, int y, int width, int height, BoundsSpecified specified)
			{
				base.SetBoundsCore (x, y, width, height, specified);
				if (vScrollBar != null)
					UpdateScrollBars();
			}

	protected override void SetVisibleCore(bool value)
			{
				base.SetVisibleCore (value);
				if (vScrollBar != null)
					UpdateScrollBars();
			}

	protected override void OnResize(EventArgs e)
			{
				base.OnResize(e);
				int xOffset,yOffset;
				if(autoScrollPosition.Y==0 && autoScrollPosition.X==0 || !autoScroll)
				{
					/* We're done already */
					return;
				}

				yOffset = ScrollArea.Bottom - DisplayRectangle.Bottom;
				xOffset = ScrollArea.Right - DisplayRectangle.Right;

				if(autoScrollPosition.Y < 0 && yOffset < 0)
				{
					yOffset = Math.Max( yOffset, autoScrollPosition.Y);
				}
				else
				{
					yOffset = 0;
				}

				if(autoScrollPosition.X < 0 && xOffset < 0) 
				{
					xOffset = Math.Max(xOffset, autoScrollPosition.X);
				}
				else
				{
					xOffset = 0;
				}
				ScrollByOffset(new Size(xOffset,yOffset));
			}

	[TODO]
	// Handle a mouse wheel event.
	protected override void OnMouseWheel(MouseEventArgs e)
			{
				base.OnMouseWheel(e);
			}

	[TODO]
	// Inner core of "Scale".
	protected override void ScaleCore(float dx, float dy)
			{
				base.ScaleCore(dx, dy);
			}

	private void ScrollByOffset(Size offset)
			{
				//Console.WriteLine("Scroll by " + offset);
				if(offset.IsEmpty)
				{
					return;
				}

				this.SuspendLayout();
				foreach(Control child in Controls)
				{
					if(child!=vScrollBar && child!=hScrollBar)
					{
						// NOTE: the offset is subtracted from the location
						child.Location-=offset;
						/* x = x +old - new; */
					}
				}

				/*
				 * offset = autoScrollPosition+newvalue
				 * so :
				 * newvalue = offset - autoScrollPosition
				 */
				autoScrollPosition=	new Point(
										autoScrollPosition.X - offset.Width,
									   	autoScrollPosition.Y - offset.Height);
			
				Invalidate();
				this.ResumeLayout();
			}
			

	private void HandleScroll(Object sender, ScrollEventArgs e)
			{
				// TODO: Optimize this function to make use of EndScroll
				// events to avoid massive redraws during many
				// smallincrement scrolls
				if(e.Type == ScrollEventType.EndScroll)
				{
					return;
				}

				if(sender == hScrollBar)
				{
					ScrollByOffset(new Size(autoScrollPosition.X+ e.NewValue,0));
				}
				else if(sender == vScrollBar)
				{
					ScrollByOffset(new Size(0, autoScrollPosition.Y + e.NewValue));
				}
			}

	/// <summary>
	///Total area of all visible controls which are scrolled with this container
	///</summary>
	private Rectangle ScrollArea
			{
				get
				{
					Rectangle total=new Rectangle(0,0,0,0);
					bool first=true;
					foreach(Control child in Controls)
					{
						if(child==vScrollBar || child==hScrollBar)
						{
							continue;
						}
						if(child.visible==false) continue;						
						if(first)
						{
							total=child.Bounds;
							first=false;
						}
						Rectangle bounds=child.Bounds;
						total=Rectangle.Union(bounds,total);
					}
					
					total = Rectangle.Union(total, 
											new Rectangle(autoScrollPosition,
														autoScrollMinSize));
					return total;					
				}
			}

	[TODO]
	public void ScrollControlIntoView(Control activeControl)
	{
		return;
	}

	/// <summary>
	/// current visible area of scrollarea
	///</summary>
	private Rectangle ViewPortRectangle
			{
				get
				{					
					return  new Rectangle(-autoScrollPosition.X,
										  -autoScrollPosition.Y,
										  DisplayRectangle.Width,
										  DisplayRectangle.Height);
				}
			}

	private void ScrollableControl_HandleCreated(object sender, EventArgs e)
			{
				// We now have a handle, so create the scrollbars if needed.
				if (autoScroll)
					CreateScrollBars();
			}

	// Create the scrollBars but dont add them to the control, just parent them using the toolkit.
	private void CreateScrollBars()
			{
				hScrollBar=new HScrollBar();	
				hScrollBar.Scroll+=new ScrollEventHandler(HandleScroll);
				hScrollBar.CreateControl();
				hScrollBar.toolkitWindow.Reparent(toolkitWindow, 0, 0);
				hScrollBar.toolkitWindow.Raise();

				vScrollBar=new VScrollBar();
				vScrollBar.Scroll+=new ScrollEventHandler(HandleScroll);
				vScrollBar.CreateControl();
				vScrollBar.toolkitWindow.Reparent(toolkitWindow, 0, 0);
				vScrollBar.toolkitWindow.Raise();
				UpdateScrollBars();
			}

	// Dock padding edge definitions.
	public class DockPaddingEdges: ICloneable
	{
		private ScrollableControl owner;
		internal bool all;
		internal int top;
		internal int left;
		internal int right;
		internal int bottom;

		public int All
		{
			get
			{
				if (all)
				{
					return top;
				}
				else
				{
					return 0;
				}
			}

			set
			{
				if (!all || top != value)
				{
					all = true;
					top = left = right = bottom = value;
					owner.PerformLayout();
				}
			}
		}

		public int Bottom
		{
			get
			{
				if (all)
				{
					return top;
				}
				else
				{
					return bottom;
				}
			}

			set
			{
				if (all || bottom != value)
				{
					all = false;
					bottom = value;
					owner.PerformLayout();
				}
			}
		}

		public int Left
		{
			get
			{
				if (all)
				{
					return top;
				}
				else
				{
					return left;
				}
			}

			set
			{
				if (all || left != value)
				{
					all = false;
					left = value;
					owner.PerformLayout();
				}
			}
		}

		public int Right
		{
			get
			{
				if (all)
				{
					return top;
				}
				else
				{
					return right;
				}
			}

			set
			{
				if (all || right != value)
				{
					all = false;
					right = value;
					owner.PerformLayout();
				}
			}
		}

		public int Top
		{
			get
			{
				return top;
			}

			set
			{
				if (all || top != value)
				{
					all = false;
					top = value;
					owner.PerformLayout();
				}
			}
		}

		internal DockPaddingEdges(ScrollableControl owner)
		{
			this.owner = owner;
			all = true;
			top = left = right = bottom = 0;
		}

		public override bool Equals(object other)
		{
			DockPaddingEdges otherEdge = other as DockPaddingEdges;
			if (otherEdge == null)
			{
				return false;
			}
			return (otherEdge.all == all && otherEdge.top == top && otherEdge.left == left && otherEdge.bottom == bottom && otherEdge.right == right);
		}

		public override int GetHashCode()
		{
			return base.GetHashCode();
		}

		public override string ToString()
		{
			return "";
		}

		object ICloneable.Clone()
		{
			DockPaddingEdges dockPaddingEdges = new DockPaddingEdges(owner);
			dockPaddingEdges.all = all;
			dockPaddingEdges.top = top;
			dockPaddingEdges.right = right;
			dockPaddingEdges.bottom = bottom;
			dockPaddingEdges.left = left;
			return dockPaddingEdges;
		}
	} // class DockPaddingEdges


	public class DockPaddingEdgesConverter : TypeConverter
	{

		public override PropertyDescriptorCollection GetProperties(ITypeDescriptorContext context, object value, Attribute[] attributes)
		{
			return TypeDescriptor.GetProperties(typeof(DockPaddingEdges), attributes).Sort(new string[]{"All", "Left", "Top", "Right", "Bottom"});
		}

		public override bool GetPropertiesSupported(ITypeDescriptorContext context)
		{
			return true;
		}
	}

#if !CONFIG_COMPACT_FORMS

	// Process a message.
	protected override void WndProc(ref Message m)
			{
				base.WndProc(ref m);
			}

#endif // !CONFIG_COMPACT_FORMS
}; // class ScrollableControl

}; // namespace System.Windows.Forms
