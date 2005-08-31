/*
 * Types.cs - Declare system-dependent type sizes.
 *
 * Copyright (C) 2004  Southern Storm Software, Pty Ltd.
 *
 * This program is free software, you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY, without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program, if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

namespace OpenSystem.Platform
{

// Types that are intended to be the same size as the corresponding
// C types in the underlying native operating system.

public enum CChar     : byte      { Zero }
public enum SChar     : sbyte     { Zero }
public enum UChar     : byte     { Zero }
public enum Short     : short     { Zero }
public enum UShort    : ushort    { Zero }
public enum Int       : int       { Zero }
public enum UInt      : uint      { Zero }
public enum Long      : int      { Zero }
public enum ULong     : uint     { Zero }
public enum LongLong  : long  { Zero }
public enum ULongLong : ulong { Zero }
public enum size_t    : uint    { Zero }
public enum ssize_t   : int   { Zero }
public enum off_t     : int     { Zero }
public enum off64_t   : long   { Zero }
public enum time_t    : int    { Zero }
public enum ino_t     : uint     { Zero }
public enum ino64_t   : ulong   { Zero }
public enum uid_t     : uint     { Zero }
public enum gid_t     : uint     { Zero }
public enum pid_t     : int     { Zero }
public enum mode_t    : uint    { Zero }
public enum dev_t     : ulong     { Zero }
public enum nlink_t   : uint   { Zero }

}; // namespace OpenSystem.Platform

namespace OpenSystem.Platform.X11
{

// Types that are useful to applications that use "libX11" and friends.

public enum XPixel    : uint     { Zero }
public enum XID       : uint     { Zero }
public enum XMask     : uint     { Zero }
public enum XAtom     : uint     { Zero }
public enum XVisualID : uint     { Zero }
public enum XTime     : uint     { CurrentTime }
public enum XWindow   : uint     { Zero }
public enum XDrawable : uint     { Zero }
public enum XFont     : uint     { Zero }
public enum XPixmap   : uint     { Zero, ParentRelative }
public enum XCursor   : uint     { Zero }
public enum XColormap : uint     { Zero }
public enum XGContext : uint     { Zero }
public enum XAppGroup : uint     { Zero }
public enum XKeySym   : uint     { Zero }
public enum XKeyCode  : byte     { Zero }
public enum XBool     : int       { False, True }
public enum XStatus   : int       { Zero }

}; // namespace OpenSystem.Platform.X11
