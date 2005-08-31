/*
 * Encoding.cs - Implementation of the "System.Text.Encoding" class.
 *
 * Copyright (C) 2001, 2002  Southern Storm Software, Pty Ltd.
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

namespace System.Text
{

using System;
using System.IO;
using System.Reflection;
using System.Globalization;
using System.Security;

[Serializable]
public abstract class Encoding
{
	// Code page used by this encoding.
	internal int codePage;

	// Constructor.
	protected Encoding()
			{
				codePage = 0;
			}
#if ECMA_COMPAT
	protected internal
#else
	protected
#endif
	Encoding(int codePage)
			{
				this.codePage = codePage;
			}

	// Convert between two encodings.
	public static byte[] Convert(Encoding srcEncoding, Encoding dstEncoding,
								 byte[] bytes)
			{
				if(srcEncoding == null)
				{
					throw new ArgumentNullException("srcEncoding");
				}
				if(dstEncoding == null)
				{
					throw new ArgumentNullException("dstEncoding");
				}
				if(bytes == null)
				{
					throw new ArgumentNullException("bytes");
				}
				return dstEncoding.GetBytes(srcEncoding.GetChars
							(bytes, 0, bytes.Length));
			}
	public static byte[] Convert(Encoding srcEncoding, Encoding dstEncoding,
								 byte[] bytes, int index, int count)
			{
				if(srcEncoding == null)
				{
					throw new ArgumentNullException("srcEncoding");
				}
				if(dstEncoding == null)
				{
					throw new ArgumentNullException("dstEncoding");
				}
				if(bytes == null)
				{
					throw new ArgumentNullException("bytes");
				}
				if(index < 0 || index > bytes.Length)
				{
					throw new ArgumentOutOfRangeException
						("index", _("ArgRange_Array"));
				}
				if(count < 0 || (bytes.Length - index) < count)
				{
					throw new ArgumentOutOfRangeException
						("count", _("ArgRange_Array"));
				}
				return dstEncoding.GetBytes(srcEncoding.GetChars
							(bytes, index, count));
			}

	// Determine if two Encoding objects are equal.
	public override bool Equals(Object obj)
			{
				Encoding enc = (obj as Encoding);
				if(enc != null)
				{
					return (codePage == enc.codePage);
				}
				else
				{
					return false;
				}
			}

	// Get the number of characters needed to encode a character buffer.
	public abstract int GetByteCount(char[] chars, int index, int count);

	// Convenience wrappers for "GetByteCount".
	public virtual int GetByteCount(String s)
			{
				if(s != null)
				{
					char[] chars = s.ToCharArray();
					return GetByteCount(chars, 0, chars.Length);
				}
				else
				{
					throw new ArgumentNullException("s");
				}
			}
	public virtual int GetByteCount(char[] chars)
			{
				if(chars != null)
				{
					return GetByteCount(chars, 0, chars.Length);
				}
				else
				{
					throw new ArgumentNullException("chars");
				}
			}

	// Get the bytes that result from encoding a character buffer.
	public abstract int GetBytes(char[] chars, int charIndex, int charCount,
								 byte[] bytes, int byteIndex);

	// Convenience wrappers for "GetBytes".
	public virtual int GetBytes(String s, int charIndex, int charCount,
								byte[] bytes, int byteIndex)
			{
				if(s == null)
				{
					throw new ArgumentNullException("s");
				}
				return GetBytes(s.ToCharArray(), charIndex, charCount,
								bytes, byteIndex);
			}
	public virtual byte[] GetBytes(String s)
			{
				if(s == null)
				{
					throw new ArgumentNullException("s");
				}
				char[] chars = s.ToCharArray();
				int numBytes = GetByteCount(chars, 0, chars.Length);
				byte[] bytes = new byte [numBytes];
				GetBytes(chars, 0, chars.Length, bytes, 0);
				return bytes;
			}
	public virtual byte[] GetBytes(char[] chars, int index, int count)
			{
				int numBytes = GetByteCount(chars, index, count);
				byte[] bytes = new byte [numBytes];
				GetBytes(chars, index, count, bytes, 0);
				return bytes;
			}
	public virtual byte[] GetBytes(char[] chars)
			{
				int numBytes = GetByteCount(chars, 0, chars.Length);
				byte[] bytes = new byte [numBytes];
				GetBytes(chars, 0, chars.Length, bytes, 0);
				return bytes;
			}

	// Get the number of characters needed to decode a byte buffer.
	public abstract int GetCharCount(byte[] bytes, int index, int count);

	// Convenience wrappers for "GetCharCount".
	public virtual int GetCharCount(byte[] bytes)
			{
				if(bytes == null)
				{
					throw new ArgumentNullException("bytes");
				}
				return GetCharCount(bytes, 0, bytes.Length);
			}

	// Get the characters that result from decoding a byte buffer.
	public abstract int GetChars(byte[] bytes, int byteIndex, int byteCount,
								 char[] chars, int charIndex);

	// Convenience wrappers for "GetChars".
	public virtual char[] GetChars(byte[] bytes, int index, int count)
			{
				int numChars = GetCharCount(bytes, index, count);
				char[] chars = new char [numChars];
				GetChars(bytes, index, count, chars, 0);
				return chars;
			}
	public virtual char[] GetChars(byte[] bytes)
			{
				if(bytes == null)
				{
					throw new ArgumentNullException("bytes");
				}
				int numChars = GetCharCount(bytes, 0, bytes.Length);
				char[] chars = new char [numChars];
				GetChars(bytes, 0, bytes.Length, chars, 0);
				return chars;
			}

	// Get a decoder that forwards requests to this object.
	public virtual Decoder GetDecoder()
			{
				return new ForwardingDecoder(this);
			}

	// Get an encoder that forwards requests to this object.
	public virtual Encoder GetEncoder()
			{
				return new ForwardingEncoder(this);
			}

#if CONFIG_REFLECTION
	// Loaded copy of the "I18N" assembly.  We need to move
	// this into a class in "System.Private" eventually.
	private static Assembly i18nAssembly;
	private static bool i18nDisabled;
#endif

	// Invoke a specific method on the "I18N" manager object.
	// Returns NULL if the method failed.
	internal static Object InvokeI18N(String name, params Object[] args)
			{
			#if CONFIG_REFLECTION
				lock(typeof(Encoding))
				{
					// Bail out if we previously detected that there
					// is insufficent engine support for I18N handling.
					if(i18nDisabled)
					{
						return null;
					}

					// Find or load the "I18N" assembly.
					if(i18nAssembly == null)
					{
						try
						{
							try
							{
								i18nAssembly = Assembly.Load("I18N");
							}
							catch(NotImplementedException)
							{
								// Assembly loading unsupported by the engine.
								i18nDisabled = true;
								return null;
							}
							catch(FileNotFoundException)
							{
								// Could not locate the I18N assembly.
								i18nDisabled = true;
								return null;
							}
							catch(BadImageFormatException)
							{
								// Something was wrong with the I18N assembly.
								i18nDisabled = true;
								return null;
							}
							catch(SecurityException)
							{
								// The engine refused to load I18N.
								i18nDisabled = true;
								return null;
							}
							if(i18nAssembly == null)
							{
								return null;
							}
						}
						catch(SystemException)
						{
							return null;
						}
					}

					// Find the "I18N.Common.Manager" class.
					Type managerClass;
					try
					{
						managerClass =
							i18nAssembly.GetType("I18N.Common.Manager");
					}
					catch(NotImplementedException)
					{
						// "GetType" is not supported by the engine.
						i18nDisabled = true;
						return null;
					}
					if(managerClass == null)
					{
						return null;
					}

					// Get the value of the "PrimaryManager" property.
					Object manager;
					try
					{
						manager = managerClass.InvokeMember
								("PrimaryManager",
								 BindingFlags.GetProperty |
								 	BindingFlags.Static |
									BindingFlags.Public,
								 null, null, null, null, null, null);
						if(manager == null)
						{
							return null;
						}
					}
					catch(MissingMethodException)
					{
						return null;
					}
					catch(SecurityException)
					{
						return null;
					}
					catch(NotImplementedException)
					{
						// "InvokeMember" is not supported by the engine.
						i18nDisabled = true;
						return null;
					}

					// Invoke the requested method on the manager.
					try
					{
						return managerClass.InvokeMember
								(name,
								 BindingFlags.InvokeMethod |
								 	BindingFlags.Instance |
									BindingFlags.Public,
								 null, manager, args, null, null, null);
					}
					catch(MissingMethodException)
					{
						return null;
					}
					catch(SecurityException)
					{
						return null;
					}
				}
			#else
				return null;
			#endif
			}

	// Get an encoder for a specific code page.
#if ECMA_COMPAT
	private
#else
	public
#endif
	static Encoding GetEncoding(int codePage)
			{
				// Check for the builtin code pages first.
				switch(codePage)
				{
					case 0: return Default;

					case ASCIIEncoding.ASCII_CODE_PAGE:
						return ASCII;

					case UTF7Encoding.UTF7_CODE_PAGE:
						return UTF7;

					case UTF8Encoding.UTF8_CODE_PAGE:
						return UTF8;

					case UnicodeEncoding.UNICODE_CODE_PAGE:
						return Unicode;

					case UnicodeEncoding.BIG_UNICODE_CODE_PAGE:
						return BigEndianUnicode;

					case Latin1Encoding.ISOLATIN_CODE_PAGE:
						return ISOLatin1;

					case UTF32Encoding.UTF32_CODE_PAGE:
						return UTF32;

					case UTF32Encoding.UTF32_BIG_ENDIAN_CODE_PAGE:
						return BigEndianUTF32;

					default: break;
				}

				// Try to obtain a code page handler from the I18N handler.
				Encoding enc = (Encoding)(InvokeI18N("GetEncoding", codePage));
				if(enc != null)
				{
					return enc;
				}

#if false
				// Build a code page class name.
				String cpName = "System.Text.CP" + codePage.ToString();

				// Look for a code page converter in this assembly.
				Assembly assembly = Assembly.GetExecutingAssembly();
				Type type = assembly.GetType(cpName);
				if(type != null)
				{
					return (Encoding)(Activator.CreateInstance(type));
				}

				// Look in any assembly, in case the application
				// has provided its own code page handler.
				type = Type.GetType(cpName);
				if(type != null)
				{
					return (Encoding)(Activator.CreateInstance(type));
				}
#endif

				// We have no idea how to handle this code page.
				throw new NotSupportedException
					(String.Format
						(_("NotSupp_CodePage"), codePage.ToString()));
			}

#if !ECMA_COMPAT

	// Table of builtin web encoding names and the corresponding code pages.
	private static readonly String[] encodingNames =
		{"us-ascii", "utf-7", "utf-8", "utf-16",
		 "unicodeFFFE", "iso-8859-1", "ucs-4", "ucs-4-be"};
	private static readonly int[] encodingCodePages =
		{ASCIIEncoding.ASCII_CODE_PAGE,
		 UTF7Encoding.UTF7_CODE_PAGE,
		 UTF8Encoding.UTF8_CODE_PAGE,
		 UnicodeEncoding.UNICODE_CODE_PAGE,
		 UnicodeEncoding.BIG_UNICODE_CODE_PAGE,
		 Latin1Encoding.ISOLATIN_CODE_PAGE,
		 UTF32Encoding.UTF32_CODE_PAGE,
		 UTF32Encoding.UTF32_BIG_ENDIAN_CODE_PAGE};

	// Get an encoding object for a specific web encoding name.
	public static Encoding GetEncoding(String name)
			{
				// Validate the parameters.
				if(name == null)
				{
					throw new ArgumentNullException("name");
				}

				// Search the table for a name match.
				int posn;
				for(posn = 0; posn < encodingNames.Length; ++posn)
				{
					if(String.Compare(name, encodingNames[posn], true,
									  CultureInfo.InvariantCulture) == 0)
					{
						return GetEncoding(encodingCodePages[posn]);
					}
				}

				// Try to obtain a web encoding handler from the I18N handler.
				Encoding enc = (Encoding)(InvokeI18N("GetEncoding", name));
				if(enc != null)
				{
					return enc;
				}

#if false
				// Build a web encoding class name.
				String encName = "System.Text.ENC" +
								 name.ToLower(CultureInfo.InvariantCulture)
								 	.Replace('-', '_');

				// Look for a code page converter in this assembly.
				Assembly assembly = Assembly.GetExecutingAssembly();
				Type type = assembly.GetType(encName);
				if(type != null)
				{
					return (Encoding)(Activator.CreateInstance(type));
				}

				// Look in any assembly, in case the application
				// has provided its own code page handler.
				type = Type.GetType(encName);
				if(type != null)
				{
					return (Encoding)(Activator.CreateInstance(type));
				}
#endif

				// We have no idea how to handle this encoding name.
				throw new NotSupportedException
					(String.Format(_("NotSupp_EncodingName"), name));
			}

#endif // !ECMA_COMPAT

	// Get a hash code for this instance.
	public override int GetHashCode()
			{
				return codePage;
			}

	// Get the maximum number of bytes needed to encode a
	// specified number of characters.
	public abstract int GetMaxByteCount(int charCount);

	// Get the maximum number of characters needed to decode a
	// specified number of bytes.
	public abstract int GetMaxCharCount(int byteCount);

	// Get the identifying preamble for this encoding.
	public virtual byte[] GetPreamble()
			{
				return new byte [0];
			}

	// Decode a buffer of bytes into a string.
	public virtual String GetString(byte[] bytes, int index, int count)
			{
				return new String(GetChars(bytes, index, count));
			}
	public virtual String GetString(byte[] bytes)
			{
				return new String(GetChars(bytes));
			}

#if !ECMA_COMPAT

	// Get the mail body name for this encoding.
	public virtual String BodyName
			{
				get
				{
					return InternalBodyName;
				}
			}

	// Get the code page represented by this object.
	public virtual int CodePage
			{
				get
				{
					return InternalCodePage;
				}
			}

	// Get the human-readable name for this encoding.
	public virtual String EncodingName
			{
				get
				{
					return InternalEncodingName;
				}
			}

	// Get the mail agent header name for this encoding.
	public virtual String HeaderName
			{
				get
				{
					return InternalHeaderName;
				}
			}

	// Determine if this encoding can be displayed in a Web browser.
	public virtual bool IsBrowserDisplay
			{
				get
				{
					return InternalIsBrowserDisplay;
				}
			}

	// Determine if this encoding can be saved from a Web browser.
	public virtual bool IsBrowserSave
			{
				get
				{
					return InternalIsBrowserSave;
				}
			}

	// Determine if this encoding can be displayed in a mail/news agent.
	public virtual bool IsMailNewsDisplay
			{
				get
				{
					return InternalIsMailNewsDisplay;
				}
			}

	// Determine if this encoding can be saved from a mail/news agent.
	public virtual bool IsMailNewsSave
			{
				get
				{
					return InternalIsMailNewsSave;
				}
			}

	// Get the IANA-preferred Web name for this encoding.
	public virtual String WebName
			{
				get
				{
					return InternalWebName;
				}
			}

	// Get the Windows code page represented by this object.
	public virtual int WindowsCodePage
			{
				get
				{
					return InternalWindowsCodePage;
				}
			}

	// Get the mail body name for this encoding.
	internal virtual String InternalBodyName
			{
				get
				{
					return null;
				}
			}

	// Get the code page represented by this object.
	internal virtual int InternalCodePage
			{
				get
				{
					return codePage;
				}
			}

	// Get the human-readable name for this encoding.
	internal virtual String InternalEncodingName
			{
				get
				{
					return null;
				}
			}

	// Get the mail agent header name for this encoding.
	internal virtual String InternalHeaderName
			{
				get
				{
					return null;
				}
			}

	// Determine if this encoding can be displayed in a Web browser.
	internal virtual bool InternalIsBrowserDisplay
			{
				get
				{
					return false;
				}
			}

	// Determine if this encoding can be saved from a Web browser.
	internal virtual bool InternalIsBrowserSave
			{
				get
				{
					return false;
				}
			}

	// Determine if this encoding can be displayed in a mail/news agent.
	internal virtual bool InternalIsMailNewsDisplay
			{
				get
				{
					return false;
				}
			}

	// Determine if this encoding can be saved from a mail/news agent.
	internal virtual bool InternalIsMailNewsSave
			{
				get
				{
					return false;
				}
			}

	// Get the IANA-preferred Web name for this encoding.
	internal virtual String InternalWebName
			{
				get
				{
					return null;
				}
			}

	// Get the Windows code page represented by this object.
	internal virtual int InternalWindowsCodePage
			{
				get
				{
					// We make no distinction between normal and
					// Windows code pages in this implementation.
					return codePage;
				}
			}

#endif // !ECMA_COMPAT

	// Storage for standard encoding objects.
	private static Encoding asciiEncoding = null;
	private static Encoding bigEndianEncoding = null;
	private static Encoding defaultEncoding = null;
	private static Encoding utf7Encoding = null;
	private static Encoding utf8Encoding = null;
	private static Encoding unicodeEncoding = null;
	private static Encoding isoLatin1Encoding = null;
	private static Encoding utf32Encoding = null;
	private static Encoding bigEndianUtf32Encoding = null;

	// Get the standard ASCII encoding object.
	public static Encoding ASCII
			{
				get
				{
					lock(typeof(Encoding))
					{
						if(asciiEncoding == null)
						{
							asciiEncoding = new ASCIIEncoding();
						}
						return asciiEncoding;
					}
				}
			}

	// Get the standard big-endian Unicode encoding object.
	public static Encoding BigEndianUnicode
			{
				get
				{
					lock(typeof(Encoding))
					{
						if(bigEndianEncoding == null)
						{
							bigEndianEncoding = new UnicodeEncoding(true, true);
						}
						return bigEndianEncoding;
					}
				}
			}

	// Get the default encoding object.
	public static Encoding Default
			{
				get
				{
					lock(typeof(Encoding))
					{
						if(defaultEncoding == null)
						{
							// See if the underlying system knows what
							// code page handler we should be using.
							int codePage = DefaultEncoding.InternalCodePage();
							if(codePage != 0)
							{
								try
								{
									defaultEncoding = GetEncoding(codePage);
								}
								catch(NotSupportedException)
								{
									defaultEncoding = new DefaultEncoding();
								}
							}
							else
							{
								defaultEncoding = new DefaultEncoding();
							}
						}
						return defaultEncoding;
					}
				}
			}

	// Get the ISO Latin1 encoding object.
	private static Encoding ISOLatin1
			{
				get
				{
					lock(typeof(Encoding))
					{
						if(isoLatin1Encoding == null)
						{
							isoLatin1Encoding = new Latin1Encoding();
						}
						return isoLatin1Encoding;
					}
				}
			}

	// Get the standard UTF-7 encoding object.
#if ECMA_COMPAT
	private
#else
	public
#endif
	static Encoding UTF7
			{
				get
				{
					lock(typeof(Encoding))
					{
						if(utf7Encoding == null)
						{
							utf7Encoding = new UTF7Encoding();
						}
						return utf7Encoding;
					}
				}
			}

	// Get the standard UTF-8 encoding object.
	public static Encoding UTF8
			{
				get
				{
					lock(typeof(Encoding))
					{
						if(utf8Encoding == null)
						{
							utf8Encoding = new UTF8Encoding(true);
						}
						return utf8Encoding;
					}
				}
			}

	// Get the standard little-endian Unicode encoding object.
	public static Encoding Unicode
			{
				get
				{
					lock(typeof(Encoding))
					{
						if(unicodeEncoding == null)
						{
							unicodeEncoding = new UnicodeEncoding();
						}
						return unicodeEncoding;
					}
				}
			}

	// Get the standard UTF-32 encoding object.
#if !ECMA_COMPAT && CONFIG_FRAMEWORK_1_2
	public
#else
	internal
#endif
	static Encoding UTF32
			{
				get
				{
					lock(typeof(Encoding))
					{
						if(utf32Encoding == null)
						{
							utf32Encoding = new UTF32Encoding();
						}
						return utf32Encoding;
					}
				}
			}

	// Get the big-endian UTF-32 encoding object.
	private static Encoding BigEndianUTF32
			{
				get
				{
					lock(typeof(Encoding))
					{
						if(bigEndianUtf32Encoding == null)
						{
							bigEndianUtf32Encoding = new UTF32Encoding
								(true, true);
						}
						return bigEndianUtf32Encoding;
					}
				}
			}

	// Forwarding decoder implementation.
	private sealed class ForwardingDecoder : Decoder
	{
		private Encoding encoding;

		// Constructor.
		public ForwardingDecoder(Encoding enc)
				{
					encoding = enc;
				}

		// Override inherited methods.
		public override int GetCharCount(byte[] bytes, int index, int count)
				{
					return encoding.GetCharCount(bytes, index, count);
				}
		public override int GetChars(byte[] bytes, int byteIndex,
									 int byteCount, char[] chars,
									 int charIndex)
				{
					return encoding.GetChars(bytes, byteIndex, byteCount,
											 chars, charIndex);
				}

	} // class ForwardingDecoder

	// Forwarding encoder implementation.
	private sealed class ForwardingEncoder : Encoder
	{
		private Encoding encoding;

		// Constructor.
		public ForwardingEncoder(Encoding enc)
				{
					encoding = enc;
				}

		// Override inherited methods.
		public override int GetByteCount(char[] chars, int index,
										 int count, bool flush)
				{
					return encoding.GetByteCount(chars, index, count);
				}
		public override int GetBytes(char[] chars, int charIndex,
									 int charCount, byte[] bytes,
									 int byteCount, bool flush)
				{
					return encoding.GetBytes(chars, charIndex, charCount,
											 bytes, byteCount);
				}

	} // class ForwardingEncoder

}; // class Encoding

}; // namespace System.Text
