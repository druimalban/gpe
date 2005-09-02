/*
 * JapaneseCalendar.cs - Implementation of the
 *        "System.Globalization.JapaneseCalendar" class.
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

namespace System.Globalization
{

using System;

public class JapaneseCalendar : Calendar
{
	// Useful constants.
	private const int DefaultTwoDigitMax = 99;

	// Internal state.
	private GregorianVariantCalendar variant;

	// Constructor.
	public JapaneseCalendar()
			{
				GregorianVariantCalendar.EraRule[] rules =
					new GregorianVariantCalendar.EraRule[] {
						new GregorianVariantCalendar.EraRule
							(1,
							 new DateTime(1868, 9, 8),
							 new DateTime(1912, 7, 30),
							 1),
						new GregorianVariantCalendar.EraRule
							(2,
							 new DateTime(1912, 7, 30),
							 new DateTime(1926, 12, 25),
							 1),
						new GregorianVariantCalendar.EraRule
							(3,
							 new DateTime(1926, 12, 25),
							 new DateTime(1989, 1, 7),
							 1),
						new GregorianVariantCalendar.EraRule
							(4,
							 new DateTime(1989, 1, 7),
							 DateTime.MaxValue,
							 1)
					};
				variant = new GregorianVariantCalendar
					(DefaultTwoDigitMax, rules, 4);
			}

	// Get a list of eras for the calendar.
	public override int[] Eras
			{
				get
				{
					return variant.Eras;
				}
			}

	// Set the last year of a 100-year range for 2-digit processing.
	public override int TwoDigitYearMax
			{
				get
				{
					return variant.TwoDigitYearMax;
				}
				set
				{
					variant.TwoDigitYearMax = value;
				}
			}

#if CONFIG_FRAMEWORK_1_2

	// Get the minimum DateTime value supported by this calendar.
	public override DateTime MinValue
			{
				get
				{
					// Return the first day in era 1 as the minimum.
					return ToDateTime(1, 1, 1, 0, 0, 0, 0, 1);
				}
			}

	// Get the maximum DateTime value supported by this calendar.
	public override DateTime MaxValue
			{
				get
				{
					return DateTime.MaxValue;
				}
			}

#endif

	// Add a time period to a DateTime value.
	public override DateTime AddMonths(DateTime time, int months)
			{
				return variant.AddMonths(time, months);
			}
	public override DateTime AddYears(DateTime time, int years)
			{
				return variant.AddYears(time, years);
			}

	// Extract the components from a DateTime value.
	public override int GetDayOfMonth(DateTime time)
			{
				return variant.GetDayOfMonth(time);
			}
	public override System.DayOfWeek GetDayOfWeek(DateTime time)
			{
				return variant.GetDayOfWeek(time);
			}
	public override int GetDayOfYear(DateTime time)
			{
				return variant.GetDayOfYear(time);
			}
	public override int GetMonth(DateTime time)
			{
				return variant.GetMonth(time);
			}
	public override int GetYear(DateTime time)
			{
				return variant.GetYear(time);
			}

	// Get the number of days in a particular month.
	public override int GetDaysInMonth(int year, int month, int era)
			{
				return variant.GetDaysInMonth(year, month, era);
			}

	// Get the number of days in a particular year.
	public override int GetDaysInYear(int year, int era)
			{
				return variant.GetDaysInYear(year, era);
			}

	// Get the era for a specific DateTime value.
	public override int GetEra(DateTime time)
			{
				return variant.GetEra(time);
			}

	// Get the number of months in a specific year.
	public override int GetMonthsInYear(int year, int era)
			{
				return variant.GetMonthsInYear(year, era);
			}

	// Determine if a particular day is a leap day.
	public override bool IsLeapDay(int year, int month, int day, int era)
			{
				return variant.IsLeapDay(year, month, day, era);
			}

	// Determine if a particular month is a leap month.
	public override bool IsLeapMonth(int year, int month, int era)
			{
				return variant.IsLeapMonth(year, month, era);
			}

	// Determine if a particular year is a leap year.
	public override bool IsLeapYear(int year, int era)
			{
				return variant.IsLeapYear(year, era);
			}

	// Convert a particular time into a DateTime value.
	public override DateTime ToDateTime(int year, int month, int day,
										int hour, int minute, int second,
										int millisecond, int era)
			{
				return variant.ToDateTime(year, month, day, hour, minute,
										  second, millisecond, era);
			}

	// Convert a two-digit year value into a four-digit year value.
	public override int ToFourDigitYear(int year)
			{
				return variant.ToFourDigitYear(year);
			}

}; // class JapaneseCalendar

}; // namespace System.Globalization