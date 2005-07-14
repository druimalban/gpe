/* GPE Mileage
 * Copyright (C) 2005  Rene Wagner <rw@handhelds.org>
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

#ifndef __GPE_MILEAGE_UNITS_H__
#define __GPE_MILEAGE_UNITS_H__

#include <glib.h>

enum {
  UNITS_DEFAULT
};

enum _UnitType
{
  UNITS_LENGTH,
  UNITS_CAPACITY,
  UNITS_MONETARY,
  UNITS_MILEAGE,
  N_UNIT_TYPES
};
typedef enum _UnitType UnitType;

enum _LengthUnit
{
  UNITS_KILOMETER,
  UNITS_MILE,
  N_LENGTH_UNITS
};
typedef enum _LengthUnit LengthUnit;

enum _CapacityUnit
{
  UNITS_LITER,
  UNITS_GB_GALLON,
  UNITS_US_GALLON,
  N_CAPACITY_UNITS
};
typedef enum _CapacityUnit CapacityUnit;

enum _MonetaryUnit
{
  UNITS_EUR,
  UNITS_GBP,
  UNITS_USD,
  N_MONETARY_UNITS
};
typedef enum _MonetaryUnit MonetaryUnit;

enum _MileageUnit
{
  UNITS_LP100KM,
  UNITS_MPG,
  N_MILEAGE_UNITS
};
typedef enum _MileageUnit MileageUnit;

const gchar* units_description (UnitType, gint);
const gchar* units_short_description (UnitType, gint);
gint units_n_units (UnitType);

#endif /*__GPE_MILEAGE_UNITS_H__*/
