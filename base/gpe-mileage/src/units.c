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

#include <glib.h>
#include "units.h"
#include "gpe-mileage-i18n.h"

const gchar*
length_unit_descriptions[] = {
  N_("kilometer"),
  N_("mile")
};

const gchar*
length_unit_short_descriptions[] = {
  N_("km"),
  N_("mi")
};

const gchar*
capacity_unit_descriptions[] = {
  N_("liter"),
  N_("gallon (British imperial)"),
  N_("gallon (US)")
};

const gchar*
capacity_unit_short_descriptions[] = {
  N_("l"),
  N_("gal"),
  N_("gal")
};

const gchar*
monetary_unit_descriptions[] = {
  N_("Euro"),
  N_("British Pound"),
  N_("US Dollar")
};

const gchar*
monetary_unit_short_descriptions[] = {
  N_("EUR"),
  N_("GBP"),
  N_("USD")
};

const gchar*
mileage_unit_descriptions[] = {
  N_("liters per 100km"),
  N_("miles per gallon")
};

const gchar*
mileage_unit_short_descriptions[] = {
  N_("l/100km"),
  N_("MPG")
};

const gchar*
units_description (UnitType type, gint unit)
{
  switch (type)
    {
      case UNITS_LENGTH:
        return _(length_unit_descriptions[unit]);
      case UNITS_CAPACITY:
        return _(capacity_unit_descriptions[unit]);
      case UNITS_MONETARY:
        return _(monetary_unit_descriptions[unit]);
      case UNITS_MILEAGE:
        return _(mileage_unit_descriptions[unit]);
    }
}

const gchar*
units_short_description (UnitType type, gint unit)
{
  switch (type)
    {
      case UNITS_LENGTH:
        return _(length_unit_short_descriptions[unit]);
      case UNITS_CAPACITY:
        return _(capacity_unit_short_descriptions[unit]);
      case UNITS_MONETARY:
        return _(monetary_unit_short_descriptions[unit]);
      case UNITS_MILEAGE:
        return _(mileage_unit_short_descriptions[unit]);
    }
}

gint
units_n_units (UnitType type)
{
  switch (type)
    {
      case UNITS_LENGTH:
        return N_LENGTH_UNITS;
      case UNITS_CAPACITY:
        return N_CAPACITY_UNITS;
      case UNITS_MONETARY:
        return N_MONETARY_UNITS;
      case UNITS_MILEAGE:
        return N_MILEAGE_UNITS;
      default:
        return 0;
    }
}
