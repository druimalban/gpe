/* GPE SCAP
 * Copyright (C) 2005  Rene Wagner <rw@handhelds.org>
 * Copyright (C) 2006  Florian Boor <florian@linuxtogo.org>
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

#include <string.h>
#include <sys/utsname.h>
#include <libsoup/soup.h>

#include "scr-shot.h"
#include "scr-i18n.h"

#define SEP "--2643816081578981947558109727"

#define FAMILIAR_VINFO 	"/etc/familiar-version"
#define OPENZAURUS_VINFO 	"/etc/openzaurus-version"
#define ANGSTROM_VINFO 	"/etc/angstrom-version"
#define DEBIAN_VINFO 	"/etc/debian_version"
#define FAMILIAR_TIME 	"/etc/familiar-timestamp"
#define OE_VERSION 		"/etc/version"
#define P_CPUINFO 		"/proc/cpuinfo"

gchar *
get_device_model (void)
{
  gchar *result;
  struct utsname uinfo;
  gchar **strv;
  gint i = 0;
  gchar *str = NULL;

  uname (&uinfo);

  /* get cpu info, only ARM for now */
  if (g_file_get_contents (P_CPUINFO, &str, NULL, NULL))
    {
      strv = g_strsplit (str, "\n", 128);
      g_free (str);
      while (strv[i])
        {
          if (strstr (strv[i], "Hardware"))
            {
              result = g_strdup (strchr (strv[i], ':') + 1);
              g_strstrip (result);
              break;
            }
          i++;
        }
      g_strfreev (strv);
    }
#ifdef __arm__
  result = g_strdup_printf ("%s,%s",_("ARM"), uinfo.machine);
#endif
#ifdef __i386__
  result =
    g_strdup_printf ("%s, %s", _("IBM PC or compatible"), uinfo.machine);
#endif
#ifdef __mips__
#ifdef __sgi__
  result = g_strdup (_("Silicon Graphics Machine"));
#else
  result = g_strdup (_("MIPS, %s"), uinfo.machine);
#endif
#endif
#ifdef _POWER
#endif
  if (!result)
    result = g_strdup_printf ("%s", uinfo.machine);

  return result;
}


gchar *
get_distribution_version (void)
{
  gchar *result = NULL;
  gchar *tmp = NULL;

  /* check for Familiar */
  if (g_file_get_contents (FAMILIAR_VINFO, &tmp, NULL, NULL))
    {
      if (strchr (tmp, '\n'))
        strchr (tmp, '\n')[0] = 0;
      /*TRANSLATORS: "Familiar" is the name of a linux distribution. */
      result = g_strdup_printf ("%s %s", _("Familiar"),
			                    g_strstrip (strstr (tmp, " ")));
      g_free (tmp);
      return result;
    }

  /* check for OpenZaurus */
  if (g_file_get_contents (OPENZAURUS_VINFO, &tmp, NULL, NULL))
    {
      if (strchr (tmp, '\n'))
        strchr (tmp, '\n')[0] = 0;
      /*TRANSLATORS: "OpenZaurus" is the name of a linux distribution. */
      result =	g_strdup_printf ("%s %s", _("OpenZaurus"),
                                 g_strstrip (strstr (tmp, " ")));
      g_free (tmp);
      return result;
    }

  /* check for Debian */
  if (g_file_get_contents (DEBIAN_VINFO, &tmp, NULL, NULL))
    {
      /*TRANSLATORS: "Debian" is the name of a linux distribution. */
      result = g_strdup_printf ("%s %s", _("Debian"), g_strstrip (tmp));
      g_free (tmp);
      return result;
    }

  /* check for Angstrom */
  if (g_file_get_contents (ANGSTROM_VINFO, &tmp, NULL, NULL))
    {
      if (strchr (tmp, '\n'))
          strchr (tmp, '\n')[0] = 0;
      /*TRANSLATORS: "Ångström" is the name of a linux distribution. */
      result = g_strdup_printf ("%s %s", _("Ångström"),
			                    g_strstrip (strstr (tmp, " ")));
      g_free (tmp);
      return result;
    }

  /* check for OpenEmbedded */
  if (g_file_get_contents (OE_VERSION, &tmp, NULL, NULL))
    {
      /*TRANSLATORS: "OpenEmbedded" is the name of a linux distribution. */
      result = g_strdup_printf (_("OpenEmbedded"));
      g_free (tmp);
      return (result);
    }

  return result;
}


GQuark
scr_shot_error_quark (void)
{
  static GQuark q = 0;

  if (q == 0)
    q = g_quark_from_static_string ("scr-shot-error-quark");

  return q;
}

gboolean
scr_shot_upload_from_file (const gchar * path, const gchar * url,
			   gchar ** response, GError ** error)
{
  gchar *cmd, *tail, *sdata = NULL;
  gchar *start, *end;
  gsize len, content_len;
  char *body;
  gchar *model = get_device_model ();
  gchar *description = get_distribution_version ();

  SoupSession *session;
  SoupMessage *message;

  /* read file into memory again */
  g_file_get_contents (path, &sdata, &len, NULL);

  /* create session and message */
  session = soup_session_sync_new ();
  message = soup_message_new ("POST", url);
  soup_message_add_header (message->request_headers, "User-Agent", "gpe-scap/" VERSION);
  soup_message_add_header (message->request_headers, "Accept",
			   "text/xml,application/xml,application/xhtml+xml,text/html;q=0.9,text/plain;q=0.8,image/png,*/*;q=0.5");

  /* build message body */
  cmd = g_strdup_printf("--" SEP "\nContent-Disposition: form-data; name=\"model\"\n\n%s"
       "\n" "--" SEP "\nContent-Disposition: form-data; name=\"text\"\n\n%s" 
    "\n" "--" SEP "\nContent-Disposition: form-data; name=\"key\"\n\nsecret"
    "\n" "--" SEP "\nContent-Disposition: form-data; name=\"submit\"\n\nUpload" 
    "\n" "--" SEP "\nContent-Disposition: form-data; name=\"file\"; filename=\"/tmp/screenshot.png\""
    "\nContent-Type: image/png\nContent-Transfer-Encoding: binary\n\n",model, description);

  tail = "\n" SEP "--";

  content_len = strlen (tail) + strlen (cmd) + len;
  body = g_malloc (content_len);
  memcpy (body, cmd, strlen (cmd));
  memcpy (body + strlen (cmd), sdata, len);
  memcpy (body + strlen (cmd) + len, tail, strlen (tail));

  soup_message_set_request (message, "multipart/form-data; boundary=" SEP,
                            SOUP_BUFFER_SYSTEM_OWNED, body, content_len);

  soup_session_send_message (session, message);

  g_free (cmd);
  g_free (sdata);
  g_free (model);
  g_free (description);

  /* Check the return value from the server. */
  if (!SOUP_STATUS_IS_SUCCESSFUL (message->status_code))
    {

      *error = g_error_new (SCR_SHOT_ERROR, SCR_SHOT_ERROR_UPLOAD,
			    "%s (%i)", _("Unable to upload screenshot"),
			    message->status_code);
      g_object_unref (message);
      return FALSE;
    }

  if (response)
    *response =
       g_strdup (_("Your screenshot is available at http://scap.linuxtogo.org"));
  
  g_object_unref (message);
  return TRUE;
}
