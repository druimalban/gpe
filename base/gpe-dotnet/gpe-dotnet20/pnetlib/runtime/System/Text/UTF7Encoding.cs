/*
 * UTF7Encoding.cs - Implementation of the
 *		"System.Text.UTF7Encoding" class.
 *
 * Copyright (C) 2001  Southern Storm Software, Pty Ltd.
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

[Serializable]
#if ECMA_COMPAT
internal
#else
public
#endif
class UTF7Encoding : Encoding
{
	// Magic number used by Windows for UTF-7.
	internal const int UTF7_CODE_PAGE = 65000;

	// Internal state.
	private bool allowOptionals;

	// Encoding rule table for 0x00-0x7F.
	// 0 - full encode, 1 - direct, 2 - optional, 3 - encode plus.
	private static readonly byte[] encodingRules = {
		0, 0, 0, 0, 0, 0, 0, 0,   0, 1, 1, 0, 0, 1, 0, 0,	// 00
		0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,	// 10
		1, 2, 2, 2, 2, 2, 2, 1,   1, 1, 2, 3, 1, 1, 1, 1,	// 20
		1, 1, 1, 1, 1, 1, 1, 1,   1, 1, 1, 2, 2, 2, 2, 1,	// 30

		2, 1, 1, 1, 1, 1, 1, 1,   1, 1, 1, 1, 1, 1, 1, 1,	// 40
		1, 1, 1, 1, 1, 1, 1, 1,   1, 1, 1, 2, 0, 2, 2, 2,	// 50
		2, 1, 1, 1, 1, 1, 1, 1,   1, 1, 1, 1, 1, 1, 1, 1,	// 60
		1, 1, 1, 1, 1, 1, 1, 1,   1, 1, 1, 2, 2, 2, 0, 0,	// 70
	};

	// Characters to use to encode 6-bit values in base64.
	private const String base64Chars =
		"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

	// Map bytes in base64 to 6-bit values.
	private static readonly sbyte[] base64Values = {
		-1, -1, -1, -1, -1, -1, -1, -1,   -1, -1, -1, -1, -1, -1, -1, -1, // 00
		-1, -1, -1, -1, -1, -1, -1, -1,   -1, -1, -1, -1, -1, -1, -1, -1, // 10
		-1, -1, -1, -1, -1, -1, -1, -1,   -1, -1, -1, -1, 62, -1, -1, 63, // 20
		52, 53, 54, 55, 56, 57, 58, 59,   60, 61, -1, -1, -1, -1, -1, -1, // 30

		 0,  1,  2,  3,  4,  5,  6,  7,    8,  9, 10, 11, 12, 13, 14, 15, // 40
		16, 17, 18, 19, 20, 21, 22, 23,   24, 25, -1, -1, -1, -1, -1, -1, // 50
		26, 27, 28, 29, 30, 31, 32, 33,   34, 35, 36, 37, 38, 39, 40, 41, // 60
		42, 43, 44, 45, 46, 47, 48, 49,   50, 51, -1, -1, -1, -1, -1, -1, // 70

		-1, -1, -1, -1, -1, -1, -1, -1,   -1, -1, -1, -1, -1, -1, -1, -1, // 80
		-1, -1, -1, -1, -1, -1, -1, -1,   -1, -1, -1, -1, -1, -1, -1, -1, // 90
		-1, -1, -1, -1, -1, -1, -1, -1,   -1, -1, -1, -1, -1, -1, -1, -1, // A0
		-1, -1, -1, -1, -1, -1, -1, -1,   -1, -1, -1, -1, -1, -1, -1, -1, // B0

		-1, -1, -1, -1, -1, -1, -1, -1,   -1, -1, -1, -1, -1, -1, -1, -1, // C0
		-1, -1, -1, -1, -1, -1, -1, -1,   -1, -1, -1, -1, -1, -1, -1, -1, // D0
		-1, -1, -1, -1, -1, -1, -1, -1,   -1, -1, -1, -1, -1, -1, -1, -1, // E0
		-1, -1, -1, -1, -1, -1, -1, -1,   -1, -1, -1, -1, -1, -1, -1, -1, // F0
	};

	// Constructors.
	public UTF7Encoding()
			: base(UTF7_CODE_PAGE)
			{
				allowOptionals = false;
			}
	public UTF7Encoding(bool allowOptionals)
			: base(UTF7_CODE_PAGE)
			{
				this.allowOptionals = allowOptionals;
			}

	// Internal version of "GetByteCount" that can handle
	// a rolling state between calls.
	private static int InternalGetByteCount
				(char[] chars, int index, int count, bool flush,
				 int leftOver, bool allowOptionals)
			{
				// Validate the parameters.
				if(chars == null)
				{
					throw new ArgumentNullException("chars");
				}
				if(index < 0 || index > chars.Length)
				{
					throw new ArgumentOutOfRangeException
						("index", _("ArgRange_Array"));
				}
				if(count < 0 || count > (chars.Length - index))
				{
					throw new ArgumentOutOfRangeException
						("count", _("ArgRange_Array"));
				}

				// Determine the length of the output.
				int length = 0;
				int leftOverSize = (leftOver >> 8);
				byte[] rules = encodingRules;
				int ch, rule;
				while(count > 0)
				{
					ch = (int)(chars[index++]);
					--count;
					if(ch < 0x0080)
					{
						rule = rules[ch];
					}
					else
					{
						rule = 0;
					}
					switch(rule)
					{
						case 0:
						{
							// Handle characters that must be fully encoded.
							if(leftOverSize == 0)
							{
								++length;
							}
							leftOverSize += 16;
							while(leftOverSize >= 6)
							{
								++length;
								leftOverSize -= 6;
							}
						}
						break;

						case 1:
						{
							// The character is encoded as itself.
							if(leftOverSize != 0)
							{
								// Flush the previous encoded sequence.
								length += 2;
								leftOverSize = 0;
							}
							++length;
						}
						break;

						case 2:
						{
							// The character may need to be encoded.
							if(allowOptionals)
							{
								goto case 1;
							}
							else
							{
								goto case 0;
							}
						}
						// Not reached.

						case 3:
						{
							// Encode the plus sign as "+-".
							if(leftOverSize != 0)
							{
								// Flush the previous encoded sequence.
								length += 2;
								leftOverSize = 0;
							}
							length += 2;
						}
						break;
					}
				}
				if(leftOverSize != 0 && flush)
				{
					length += 2;
				}

				// Return the length to the caller.
				return length;
			}

	// Get the number of bytes needed to encode a character buffer.
	public override int GetByteCount(char[] chars, int index, int count)
			{
				return InternalGetByteCount(chars, index, count,
											true, 0, allowOptionals);
			}

	// Internal version of "GetBytes" that can handle a
	// rolling state between calls.
	private static int InternalGetBytes
				(char[] chars, int charIndex, int charCount,
				 byte[] bytes, int byteIndex, bool flush,
				 ref int leftOver, bool allowOptionals)
			{
				// Validate the parameters.
				if(chars == null)
				{
					throw new ArgumentNullException("chars");
				}
				if(bytes == null)
				{
					throw new ArgumentNullException("bytes");
				}
				if(charIndex < 0 || charIndex > chars.Length)
				{
					throw new ArgumentOutOfRangeException
						("charIndex", _("ArgRange_Array"));
				}
				if(charCount < 0 || charCount > (chars.Length - charIndex))
				{
					throw new ArgumentOutOfRangeException
						("charCount", _("ArgRange_Array"));
				}
				if(byteIndex < 0 || byteIndex > bytes.Length)
				{
					throw new ArgumentOutOfRangeException
						("byteIndex", _("ArgRange_Array"));
				}

				// Convert the characters.
				int posn = byteIndex;
				int byteLength = bytes.Length;
				int leftOverSize = (leftOver >> 8);
				int leftOverBits = (leftOver & 0xFF);
				byte[] rules = encodingRules;
				String base64 = base64Chars;
				int ch, rule;
				while(charCount > 0)
				{
					ch = (int)(chars[charIndex++]);
					--charCount;
					if(ch < 0x0080)
					{
						rule = rules[ch];
					}
					else
					{
						rule = 0;
					}
					switch(rule)
					{
						case 0:
						{
							// Handle characters that must be fully encoded.
							if(leftOverSize == 0)
							{
								if(posn >= byteLength)
								{
									throw new ArgumentException
										(_("Arg_InsufficientSpace"), "bytes");
								}
								bytes[posn++] = (byte)'+';
							}
							leftOverBits = ((leftOverBits << 16) | ch);
							leftOverSize += 16;
							while(leftOverSize >= 6)
							{
								if(posn >= byteLength)
								{
									throw new ArgumentException
										(_("Arg_InsufficientSpace"), "bytes");
								}
								leftOverSize -= 6;
								bytes[posn++] = (byte)(base64
									[leftOverBits >> leftOverSize]);
								leftOverBits &= ((1 << leftOverSize) - 1);
							}
						}
						break;

						case 1:
						{
							// The character is encoded as itself.
							if(leftOverSize != 0)
							{
								// Flush the previous encoded sequence.
								if((posn + 2) > byteLength)
								{
									throw new ArgumentException
										(_("Arg_InsufficientSpace"), "bytes");
								}
								bytes[posn++] = (byte)(base64
									[leftOverBits << (6 - leftOverSize)]);
								bytes[posn++] = (byte)'-';
								leftOverSize = 0;
								leftOverBits = 0;
							}
							if(posn >= byteLength)
							{
								throw new ArgumentException
									(_("Arg_InsufficientSpace"), "bytes");
							}
							bytes[posn++] = (byte)ch;
						}
						break;

						case 2:
						{
							// The character may need to be encoded.
							if(allowOptionals)
							{
								goto case 1;
							}
							else
							{
								goto case 0;
							}
						}
						// Not reached.

						case 3:
						{
							// Encode the plus sign as "+-".
							if(leftOverSize != 0)
							{
								// Flush the previous encoded sequence.
								if((posn + 2) > byteLength)
								{
									throw new ArgumentException
										(_("Arg_InsufficientSpace"), "bytes");
								}
								bytes[posn++] = (byte)(base64
									[leftOverBits << (6 - leftOverSize)]);
								bytes[posn++] = (byte)'-';
								leftOverSize = 0;
								leftOverBits = 0;
							}
							if((posn + 2) > byteLength)
							{
								throw new ArgumentException
									(_("Arg_InsufficientSpace"), "bytes");
							}
							bytes[posn++] = (byte)'+';
							bytes[posn++] = (byte)'-';
						}
						break;
					}
				}
				if(leftOverSize != 0 && flush)
				{
					if((posn + 2) > byteLength)
					{
						throw new ArgumentException
							(_("Arg_InsufficientSpace"), "bytes");
					}
					bytes[posn++] = (byte)(base64
						[leftOverBits << (6 - leftOverSize)]);
					bytes[posn++] = (byte)'-';
					leftOverSize = 0;
					leftOverBits = 0;
				}
				leftOver = ((leftOverSize << 8) | leftOverBits);

				// Return the length to the caller.
				return posn - byteIndex;
			}

	// Get the bytes that result from encoding a character buffer.
	public override int GetBytes(char[] chars, int charIndex, int charCount,
								 byte[] bytes, int byteIndex)
			{
				int leftOver = 0;
				return InternalGetBytes(chars, charIndex, charCount,
									    bytes, byteIndex, true,
										ref leftOver, allowOptionals);
			}

	// Internal version of "GetCharCount" that can handle
	// a rolling state between call.s
	private static int InternalGetCharCount
					(byte[] bytes, int index, int count, int leftOver)
			{
				// Validate the parameters.
				if(bytes == null)
				{
					throw new ArgumentNullException("bytes");
				}
				if(index < 0 || index > bytes.Length)
				{
					throw new ArgumentOutOfRangeException
						("index", _("ArgRange_Array"));
				}
				if(count < 0 || count > (bytes.Length - index))
				{
					throw new ArgumentOutOfRangeException
						("count", _("ArgRange_Array"));
				}

				// Determine the length of the result.
				int length = 0;
				int byteval, b64value;
				bool normal = ((leftOver & 0x01000000) == 0);
				bool prevIsPlus = ((leftOver & 0x02000000) != 0);
				int leftOverSize = ((leftOver >> 16) & 0xFF);
				sbyte[] base64 = base64Values;
				while(count > 0)
				{
					byteval = (int)(bytes[index++]);
					--count;
					if(normal)
					{
						if(byteval != '+')
						{
							// Directly-encoded character.
							++length;
						}
						else
						{
							// Start of a base64-encoded character.
							normal = false;
							prevIsPlus = true;
						}
					}
					else
					{
						// Process the next byte in a base64 sequence.
						if(byteval == (int)'-')
						{
							// End of a base64 sequence.
							if(prevIsPlus || leftOverSize > 0)
							{
								++length;
								leftOverSize = 0;
							}
							normal = true;
						}
						else if((b64value = base64[byteval]) != -1)
						{
							// Extra character in a base64 sequence.
							leftOverSize += 6;
							if(leftOverSize >= 16)
							{
								++length;
								leftOverSize -= 16;
							}
						}
						else
						{
							// Normal character terminating a base64 sequence.
							if(leftOverSize > 0)
							{
								++length;
								leftOverSize = 0;
							}
							++length;
							normal = true;
						}
						prevIsPlus = false;
					}
				}

				// Return the final length to the caller.
				return length;
			}

	// Get the number of characters needed to decode a byte buffer.
	public override int GetCharCount(byte[] bytes, int index, int count)
			{
				return InternalGetCharCount(bytes, index, count, 0);
			}

	// Internal version of "GetChars" that can handle a
	// rolling state between calls.
	private static int InternalGetChars
				(byte[] bytes, int byteIndex, int byteCount,
				 char[] chars, int charIndex, ref int leftOver)
			{
				// Validate the parameters.
				if(bytes == null)
				{
					throw new ArgumentNullException("bytes");
				}
				if(chars == null)
				{
					throw new ArgumentNullException("chars");
				}
				if(byteIndex < 0 || byteIndex > bytes.Length)
				{
					throw new ArgumentOutOfRangeException
						("byteIndex", _("ArgRange_Array"));
				}
				if(byteCount < 0 || byteCount > (bytes.Length - byteIndex))
				{
					throw new ArgumentOutOfRangeException
						("byteCount", _("ArgRange_Array"));
				}
				if(charIndex < 0 || charIndex > chars.Length)
				{
					throw new ArgumentOutOfRangeException
						("charIndex", _("ArgRange_Array"));
				}

				// Convert the bytes into characters.
				int posn = charIndex;
				int charLength = chars.Length;
				int byteval, b64value;
				bool normal = ((leftOver & 0x01000000) == 0);
				bool prevIsPlus = ((leftOver & 0x02000000) != 0);
				int leftOverSize = ((leftOver >> 16) & 0xFF);
				int leftOverBits = (leftOver & 0xFFFF);
				sbyte[] base64 = base64Values;
				while(byteCount > 0)
				{
					byteval = (int)(bytes[byteIndex++]);
					--byteCount;
					if(normal)
					{
						if(byteval != '+')
						{
							// Directly-encoded character.
							if(posn >= charLength)
							{
								throw new ArgumentException
									(_("Arg_InsufficientSpace"), "chars");
							}
							chars[posn++] = (char)byteval;
						}
						else
						{
							// Start of a base64-encoded character.
							normal = false;
							prevIsPlus = true;
						}
					}
					else
					{
						// Process the next byte in a base64 sequence.
						if(byteval == (int)'-')
						{
							// End of a base64 sequence.
							if(prevIsPlus)
							{
								if(posn >= charLength)
								{
									throw new ArgumentException
										(_("Arg_InsufficientSpace"), "chars");
								}
								chars[posn++] = '+';
							}
							else if(leftOverSize > 0)
							{
								if(posn >= charLength)
								{
									throw new ArgumentException
										(_("Arg_InsufficientSpace"), "chars");
								}
								chars[posn++] =
									(char)(leftOverBits << (16 - leftOverSize));
								leftOverSize = 0;
								leftOverBits = 0;
							}
							normal = true;
						}
						else if((b64value = base64[byteval]) != -1)
						{
							// Extra character in a base64 sequence.
							leftOverBits = (leftOverBits << 6) | b64value;
							leftOverSize += 6;
							if(leftOverSize >= 16)
							{
								if(posn >= charLength)
								{
									throw new ArgumentException
										(_("Arg_InsufficientSpace"), "chars");
								}
								leftOverSize -= 16;
								chars[posn++] =
									(char)(leftOverBits >> leftOverSize);
								leftOverBits &= ((1 << leftOverSize) - 1);
							}
						}
						else
						{
							// Normal character terminating a base64 sequence.
							if(leftOverSize > 0)
							{
								if(posn >= charLength)
								{
									throw new ArgumentException
										(_("Arg_InsufficientSpace"), "chars");
								}
								chars[posn++] =
									(char)(leftOverBits << (16 - leftOverSize));
								leftOverSize = 0;
								leftOverBits = 0;
							}
							if(posn >= charLength)
							{
								throw new ArgumentException
									(_("Arg_InsufficientSpace"), "chars");
							}
							chars[posn++] = (char)byteval;
							normal = true;
						}
						prevIsPlus = false;
					}
				}
				leftOver = (leftOverBits | (leftOverSize << 16) |
						    (normal ? 0 : 0x01000000) |
						    (prevIsPlus ? 0x02000000 : 0));

				// Return the final length to the caller.
				return posn - charIndex;
			}

	// Get the characters that result from decoding a byte buffer.
	public override int GetChars(byte[] bytes, int byteIndex, int byteCount,
								 char[] chars, int charIndex)
			{
				int leftOver = 0;
				return InternalGetChars(bytes, byteIndex, byteCount,
										chars, charIndex, ref leftOver);
			}

	// Get the maximum number of bytes needed to encode a
	// specified number of characters.
	public override int GetMaxByteCount(int charCount)
			{
				if(charCount < 0)
				{
					throw new ArgumentOutOfRangeException
						("charCount", _("ArgRange_NonNegative"));
				}
				return charCount * 5;
			}

	// Get the maximum number of characters needed to decode a
	// specified number of bytes.
	public override int GetMaxCharCount(int byteCount)
			{
				if(byteCount < 0)
				{
					throw new ArgumentOutOfRangeException
						("byteCount", _("ArgRange_NonNegative"));
				}
				return byteCount;
			}

	// Get a UTF7-specific decoder that is attached to this instance.
	public override Decoder GetDecoder()
			{
				return new UTF7Decoder();
			}

	// Get a UTF7-specific encoder that is attached to this instance.
	public override Encoder GetEncoder()
			{
				return new UTF7Encoder(allowOptionals);
			}

#if !ECMA_COMPAT

	// Get the mail body name for this encoding.
	internal override String InternalBodyName
			{
				get
				{
					return "utf-7";
				}
			}

	// Get the human-readable name for this encoding.
	internal override String InternalEncodingName
			{
				get
				{
					return "Unicode (UTF-7)";
				}
			}

	// Get the mail agent header name for this encoding.
	internal override String InternalHeaderName
			{
				get
				{
					return "utf-7";
				}
			}

	// Determine if this encoding can be displayed in a mail/news agent.
	internal override bool InternalIsMailNewsDisplay
			{
				get
				{
					return true;
				}
			}

	// Determine if this encoding can be saved from a mail/news agent.
	internal override bool InternalIsMailNewsSave
			{
				get
				{
					return true;
				}
			}

	// Get the IANA-preferred Web name for this encoding.
	internal override String InternalWebName
			{
				get
				{
					return "utf-7";
				}
			}

	// Get the Windows code page represented by this object.
	internal override int InternalWindowsCodePage
			{
				get
				{
					return UnicodeEncoding.UNICODE_CODE_PAGE;
				}
			}

#endif // !ECMA_COMPAT

	// UTF-7 decoder implementation.
	private sealed class UTF7Decoder : Decoder
	{
		// Internal state.
		private int leftOver;

		// Constructor.
		public UTF7Decoder()
				{
					leftOver = 0;
				}

		// Override inherited methods.
		public override int GetCharCount(byte[] bytes, int index, int count)
				{
					return InternalGetCharCount(bytes, index, count, leftOver);
				}
		public override int GetChars(byte[] bytes, int byteIndex,
									 int byteCount, char[] chars,
									 int charIndex)
				{
					return InternalGetChars(bytes, byteIndex, byteCount,
											chars, charIndex, ref leftOver);
				}

	} // class UTF7Decoder

	// UTF-7 encoder implementation.
	private sealed class UTF7Encoder : Encoder
	{
		private bool allowOptionals;
		private int leftOver;

		// Constructor.
		public UTF7Encoder(bool allowOptionals)
				{
					this.allowOptionals = allowOptionals;
					this.leftOver = 0;
				}

		// Override inherited methods.
		public override int GetByteCount(char[] chars, int index,
										 int count, bool flush)
				{
					return InternalGetByteCount
						(chars, index, count, flush, leftOver, allowOptionals);
				}
		public override int GetBytes(char[] chars, int charIndex,
									 int charCount, byte[] bytes,
									 int byteIndex, bool flush)
				{
					return InternalGetBytes(chars, charIndex, charCount,
											bytes, byteIndex, flush,
											ref leftOver, allowOptionals);
				}

	} // class UTF7Encoder

}; // class UTF7Encoding

}; // namespace System.Text
