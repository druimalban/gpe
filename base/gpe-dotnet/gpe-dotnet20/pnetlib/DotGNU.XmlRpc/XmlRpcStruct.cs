/*
 * DotGNU XmlRpc implementation
 * 
 * Copyright (C) 2003  Free Software Foundation, Inc.
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 *
 * $Revision$  $Date$
 * 
 * --------------------------------------------------------------------------
 */
using System.Collections;

namespace DotGNU.XmlRpc
{
  public sealed class XmlRpcStruct : Hashtable
  {
    // no, we are not using properties here caus we don't want the
    // function call overhead. 
    public object value;
    public string key;

    public void Commit()
    {
      Add( key, value );
    }
    
    // returns the XML representation of this structure TODO:
    // formatting options for indented format.  This may have to be
    // moved into the XmlRpcWriter...  we'll see
    //public override string ToString()
    //{
    // string members;
    //foreach( DictionaryEntry entry in this) {
    //members += String.Format( "Member Name:{0}, Value Type: {1}, Value: {2}\n",
    //			  entry.Key, entry.Value.GetType(), entry.Value );
    //}
    //return members;
    //}
  }
}


