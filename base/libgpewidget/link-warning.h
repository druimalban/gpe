/* Support macros for making weak and strong aliases for symbols,
   and for using symbol sets and linker warnings with GNU ld.
   Copyright (C) 1995-1998,2000,2001,2002,2003 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */

/**
 * link_warning:
 * @symbol: Symbol to mark.
 * @msg: Warning message to emit on link.
 *
 * Function to mark deprecated or obsolete symbol to emit a warning
 * at link time.
 */
#  define link_warning(symbol, msg) \
  static const char __evoke_link_warning_##symbol[]     \
    __attribute__ ((unused, section (".gnu.warning." #symbol ))) \
    = msg;
