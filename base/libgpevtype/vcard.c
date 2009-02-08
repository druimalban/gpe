/*
 * Copyright (C) 2004 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <glib.h>
#include <assert.h>

#include "priv.h"

struct tag_map
{
  gchar *tag;
  gchar *vc;
};

static struct tag_map map[] =
  {
    { "name", NULL },
    { "given_name", "givenname" },
    { "middle_name", "middlename" },
    { "family_name", "familyname" },
    { "title", "prefix" },
    { "honorific_suffix", "suffix" },
    { "nickname", NULL },
    { "comment", "note" },
    { "work_organization", "organization"},
    { "company", "organization"},
    { "notes", "note"},
    { "work.title", "jobtitle"},
    //categories handled separately    { "category", "categories"},
    { "work.www", "url" },
    //birthday handled specially { "birthday", NULL },
    //{ "photo", NULL }, /* FIXME -- pointer */
    { "photo_uri", "photo-uri" },
    { "mailer", NULL },
    //{ "timezone", NULL }, /* FIXME -- int, or do we use timezone-string?*/
    //{ "latitude", NULL }, /* FIXME -- double */
    //{ "longitude", NULL }, /* FIXME -- double */
    { "role", NULL },
    //{ "logo", NULL }, /* FIXME -- pointer */
    { "logo_uri", "logo-uri" },
    //{ "agent", NULL }, /* FIXME -- object or do we use agent-string? */
    { "agent_uri", "agent-uri" },
    { "sort_string", "sort-string" },
    //{ "sound", NULL }, /* FIXME -- pointer */
    { "sound_uri", "sound-uri" },
    { "uid", NULL },
    { "class", NULL },
    //{ "keys", "key-list" }, /* FIXME -- list */
    { NULL, NULL }
  };

static void
set_type (GObject *o, const gchar *type)
{
  assert (!strcasecmp (type, "work") || !strcasecmp (type, "home"));

  g_object_set (o, "work", FALSE, "home", FALSE, NULL);
  g_object_set (o, type, TRUE, NULL);
}

static gboolean
tag_with_type (MIMEDirVCard *card, const char *tag, const char *value, const char *type)
{
  if (!strcasecmp (tag, "address"))
    {
      MIMEDirVCardAddress *a = mimedir_vcard_address_new ();
      set_type (G_OBJECT (a), type);

      g_object_set (G_OBJECT (a), "full", value, NULL);
      g_object_set (G_OBJECT (a), "international", FALSE, NULL);
      g_object_set (G_OBJECT (a), "postal", FALSE, NULL);
      g_object_set (G_OBJECT (a), "parcel", FALSE, NULL);

      mimedir_vcard_append_address (card, a);
      return TRUE;
    }
  else if (!strcasecmp (tag, "telephone"))
    {
      MIMEDirVCardPhone *a = mimedir_vcard_phone_new ();
      set_type (G_OBJECT (a), type);
      g_object_set (G_OBJECT (a), "number", value, NULL);
      mimedir_vcard_append_phone (card, a);
      return TRUE;
    }
  else if (!strcasecmp (tag, "mobile"))
    {
      MIMEDirVCardPhone *a = mimedir_vcard_phone_new ();
      set_type (G_OBJECT (a), type);
      g_object_set (G_OBJECT (a), "voice", FALSE, NULL);
      g_object_set (G_OBJECT (a), "cell", TRUE, NULL);
      g_object_set (G_OBJECT (a), "number", value, NULL);
      mimedir_vcard_append_phone (card, a);
      return TRUE;
    }
  else if ((!strcasecmp (tag, "fax")) 
           || (!strcasecmp (tag, "modem"))
           || (!strcasecmp (tag, "bbs"))
           || (!strcasecmp (tag, "isdn"))
           || (!strcasecmp (tag, "car"))
           || (!strcasecmp (tag, "pager"))
          )
    {
      gchar *dtag = g_ascii_strdown (tag, -1);
      MIMEDirVCardPhone *a = mimedir_vcard_phone_new ();
      set_type (G_OBJECT (a), type);
      g_object_set (G_OBJECT (a), "voice", FALSE, NULL);
      g_object_set (G_OBJECT (a), dtag, TRUE, NULL);
      g_object_set (G_OBJECT (a), "number", value, NULL);
      mimedir_vcard_append_phone (card, a);
      g_free (dtag);
      return TRUE;
    }
  else if (!strcasecmp (tag, "email"))
    {
      MIMEDirVCardEMail *a = mimedir_vcard_email_new ();
      g_object_set (G_OBJECT (a), "address", value, NULL);
      mimedir_vcard_append_email (card, a);
      return TRUE;
    }

  return FALSE;
}

static gboolean
vcard_interpret_tag (MIMEDirVCard *card, const char *tag, const char *value)
{
  struct tag_map *t = &map[0];
    
  while (t->tag)
    {
      if (!strcasecmp (t->tag, tag))
	{
	  g_object_set (G_OBJECT (card), t->vc ? t->vc : t->tag, value, NULL);
	  return TRUE;
	}
      t++;
    }

  if (!strcasecmp(tag,"category"))
    {
      const gchar *catname = gpe_pim_category_name(atoi(value));
      // If we can't find the name the category has probably been deleted -- ignore it
      if (catname) {
	// There is no "add category" operation so we have to get the current category list
	// add the new one and set the new list
	GList *old_categories = NULL;
	// Note: the following g_object_get provides a pointer to the
	// actual category list, not a copy.  So it must not be modified or free'd!
	g_object_get (G_OBJECT (card), "category-list", &old_categories, NULL);

	// Create a new category list.
	GList *i, *new_categories = NULL;
	for (i = old_categories; i; i = i->next)
	  {new_categories = g_list_append(new_categories, g_strdup(i->data));}

	new_categories = g_list_append(new_categories, g_strdup(catname));

	// Setting the category-list attribute copies the whole list
	g_object_set(G_OBJECT (card), "category-list", new_categories, NULL);
	for (i = new_categories; i; i = i->next)
	  {g_free(i->data);}
	g_list_free(new_categories);

	return TRUE;
      }
    }

  if (!strncasecmp (tag, "home.", 5))
    return tag_with_type (card, tag + 5, value, "home");
  else if (!strncasecmp (tag, "work.", 5))
    return tag_with_type (card, tag + 5, value, "work");

  if (!strcasecmp (tag, "birthday")) {
    /* Need to convert between YYYYMMDD format used in tag and MIMEDirDateTime format used in VCARD */

    guint year, month, day;
    MIMEDirDateTime *dtime;

    sscanf (value, "%04d%02d%02d", &year, &month, &day);

    dtime = mimedir_datetime_new_from_date((GDateYear)year, (GDateMonth)month, (GDateDay)day);
    if (!dtime) return FALSE;

    g_object_set(G_OBJECT (card), "birthday", dtime, NULL);

    g_object_unref(G_OBJECT (dtime));
    return TRUE;
  }

  return FALSE;
}

MIMEDirVCard *
vcard_from_tags (GSList *tags)
{
  MIMEDirVCard *vcard = mimedir_vcard_new ();

  while (tags)
    {
      gpe_tag_pair *p = tags->data;

      vcard_interpret_tag (vcard, p->tag, p->value);

      tags = tags->next;
    }

  return vcard;
}

static gchar *
append_str (gchar *old, gchar *new)
{
  gchar *ret;

  if (old)
    {
      ret = g_strdup_printf ("%s\n%s", old, new);
      g_free (old);
      g_free (new);
    }
  else
    ret = new;

  return ret;
}

GSList *
vcard_to_tags (MIMEDirVCard *vcard)
{
  gboolean home = FALSE, work = FALSE;
  struct tag_map *t = &map[0];
  GSList *data = NULL;
  GList *l;
  gchar *fn, *ln, *name;
  gboolean have_adr_home = FALSE, have_adr_work = FALSE;
  int num_emails=0;
  
  while (t->tag)
    {
      gchar *value = NULL;

      g_object_get (G_OBJECT (vcard), t->vc ? t->vc : t->tag, &value, NULL);

      if (value)
        {
          data = gpe_tag_list_prepend (data, t->tag, g_strstrip (value));
          value = NULL;
        }

      t++;
    }

  l = NULL;
  g_object_get (G_OBJECT (vcard), "address-list", &l, NULL);
  while (l)
    {
      MIMEDirVCardAddress *address;
      gchar *s = NULL;
      address = l->data;
      g_object_get (G_OBJECT (address), "full", &s, NULL);
      if (s == NULL)
	{
	  gchar *pobox = NULL, *street = NULL, *region = NULL, *locality = NULL, *code = NULL, *country = NULL, *ext = NULL;

	  g_object_get (G_OBJECT (address), "pobox", &pobox, NULL);
	  g_object_get (G_OBJECT (address), "extended", &ext, NULL);
	  g_object_get (G_OBJECT (address), "street", &street, NULL);
	  g_object_get (G_OBJECT (address), "locality", &locality, NULL);
	  g_object_get (G_OBJECT (address), "region", &region, NULL);
	  g_object_get (G_OBJECT (address), "pcode", &code, NULL);
	  g_object_get (G_OBJECT (address), "country", &country, NULL);

	  if (pobox)
	    s = append_str (s, pobox);
	  if (ext)
	    s = append_str (s, ext);
	  if (street)
	    s = append_str (s, street);
	  if (locality)
	    s = append_str (s, locality);
	  if (region)
	    s = append_str (s, region);
	  if (code)
	    s = append_str (s, code);
	  if (country)
	    s = append_str (s, country);
	}

      if (s)
	{
	  g_object_get (G_OBJECT (address), "home", &home, NULL);
	  g_object_get (G_OBJECT (address), "work", &work, NULL);
	  if ( (home || !work) && (!have_adr_home)) 
            {
              /* we want prevent gpe from importing more than two
	         adresses */
	      data = gpe_tag_list_prepend (data, "home.address", s);
	      have_adr_home = TRUE;
	    }
	  if (work && !have_adr_work) 
            {
	      data = gpe_tag_list_prepend (data, "work.address", s);
	      have_adr_work = TRUE;
	    }
	}
      else
	fprintf (stderr, "Unable to retrieve address.\n");
      l = g_list_next (l);
    }
  l = NULL;
  g_object_get (G_OBJECT (vcard), "email-list", &l, NULL);
  while (l)
    {
      MIMEDirVCardEMail *email = l->data;
      gchar *s = NULL;

      g_object_get (G_OBJECT (email), "address", &s, NULL);

      if (s) /* vcard doesn't tell us if it is home or work */
        {
	  if (num_emails == 1)
            {
              /* we want prevent gpe from importing more
                 than two email adresses. The first wil
                 be the one from work, the secont the
                 home email. */
              data = gpe_tag_list_prepend (data, "home.email", g_strdup(s));/*!*/
	      num_emails++;
	    }
	  if (num_emails == 0) 
            {
              data = gpe_tag_list_prepend (data, "work.email", s);
	      num_emails++;;
	    }
        }

      l = g_list_next (l);
    }

  l = NULL;
  g_object_get (G_OBJECT (vcard), "phone-list", &l, NULL);
  while (l)
    {
      MIMEDirVCardPhone *phone = l->data;
      gchar *s = NULL;
      gboolean home = FALSE, work = FALSE;
      gboolean fax = FALSE, cell = FALSE, voice = FALSE;

      g_object_get (G_OBJECT (phone), "number", &s, NULL);

      if (s)
	{
	  g_object_get (G_OBJECT (phone), "home", &home, NULL);
	  g_object_get (G_OBJECT (phone), "work", &work, NULL);
	  g_object_get (G_OBJECT (phone), "fax", &fax, NULL);
	  g_object_get (G_OBJECT (phone), "cell", &cell, NULL);
	  g_object_get (G_OBJECT (phone), "voice", &voice, NULL);

	  if (fax)
	    {		  
	      if (work)
		data = gpe_tag_list_prepend (data, "work.fax", s);
	      else
		data = gpe_tag_list_prepend (data, "home.fax", s);
	    }
	  if (cell)
	    {		  
	      if (work)
		data = gpe_tag_list_prepend (data, "work.mobile", s);
	      else
		data = gpe_tag_list_prepend (data, "home.mobile", s);
	    }
	  if ((voice && !cell) || (!fax && !cell))
	    {		  
	      if (work)
		data = gpe_tag_list_prepend (data, "work.telephone", s);
	      else
		data = gpe_tag_list_prepend (data, "home.telephone", s);
	    }
	}
      
      l = g_list_next (l);
    }
  
  /* populate name field */
  g_object_get (G_OBJECT (vcard), "givenname", &fn, NULL);
  g_object_get (G_OBJECT (vcard), "familyname", &ln, NULL);
  if (fn)
    g_strstrip (fn);
  if (ln)
    g_strstrip (ln);
  
  name = NULL;
  if (fn && ln)
    name = g_strdup_printf ("%s %s", fn, ln);
  else if (ln)
    name = g_strdup (ln);
  else if (fn)
    name = g_strdup (ln);

  if (name)
    gpe_tag_list_prepend (data, "NAME", name);

  g_free (fn);
  g_free (ln);

  /* CATEGORIES */
  GList *categories = NULL;
  // Note: the following g_object_get provides a pointer to the
  // actual category list, not a copy.  So it must not be free'd.
  g_object_get (G_OBJECT (vcard), "category-list", &categories, NULL);
  GList *i;
  for (i = categories; i; i = i->next)
    {
      int catid;
      const gchar *name = (const gchar *)i->data;
      // Lookup category name
      catid = gpe_pim_category_id(name);
      if (catid < 0) gpe_pim_category_new(name, &catid); // Create category
      data = gpe_tag_list_prepend (data, "category", g_strdup_printf ("%d", catid));     
    }

  /* Birthday */
  // Note that mimedir_vcard_get_birthday just returns the pointer to the object
  // without adding a reference, so we must not remove the reference
  MIMEDirDateTime *bday = mimedir_vcard_get_birthday(vcard);
  if (bday) {
    // Convert from MIMEDirDateTime to YYYYMMDD format
    GDateYear year;
    GDateMonth month;
    GDateDay day;

    mimedir_datetime_get_date (bday, &year, &month, &day);

    data = gpe_tag_list_prepend (data, "birthday", g_strdup_printf ("%04d%02d%02d", (int)year, (int)month, (int)day));
  }
	
  return data;
}
