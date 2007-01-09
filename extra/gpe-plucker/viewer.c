/* -*- mode: c; indent-tabs-mode: nil; c-indent-level: 4; -*-

 * $Id$
 *
 * Viewer - a part of Plucker, the open-source, open-format ebook system
 * Copyright (c) 2002, Bill Janssen
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include <gtk/gtk.h>
#include <gdk/gdk.h>

/* for GPE specific things */
#include <gpe/init.h>
#include <gpe/pixmaps.h>
#include <gpe/picturebutton.h>
#include <gpe/errorbox.h>
#define _(_x) gettext (_x)

#include <stdlib.h>             /* for exit() */
#include <stdio.h>
#include <unpluck.h>
#include <sys/stat.h>
#include <time.h>
#include <string.h>
#include <ctype.h>
#include <libintl.h>

#include "gtkviewer.h"

#define READ_BIGENDIAN_SHORT(p) (((p)[0] << 8)|((p)[1]))
#define READ_BIGENDIAN_LONG(p)  (((p)[0] << 24)|((p)[1] << 16)|((p)[2] << 8)|((p)[3]))

#define UTF8_CHARSET_MIBENUM    106

#define MAX_RECORD_SIZE         0x10000

#define NEWLINE                 '\n'
#define UNICODE_LINE_SEPARATOR          0x2028
#define UNICODE_PARAGRAPH_SEPARATOR     0x2029

#define UNICODE_LS_STRING       "\xe2\x80\xa8"
#define UNICODE_PS_STRING       "\xe2\x80\xa9"

extern GdkPixbuf *
  GetImageRecord (plkr_Document * doc, int id, double scale);

extern GdkPixbuf *
  GetGlyph (plkr_Document * doc, int record_id, int glyph_index, int desired_dpi);
GdkPixbuf *
  GetSpacerPixbuf (int width_in_pixels);


static GMainLoop *TheMainloop = NULL;

static GtkTextTagTable *TheTagsTable = NULL;

static GTree *CharsetNames = NULL;

static double TheScale;
static int ScreenDPI = 0;

static gboolean UseUnicode = FALSE;
static gboolean UnicodeNewlineSeparatorBroken = TRUE;

static gboolean Verbose = FALSE;

static GtkWidget *TheLibrary = NULL;
static gboolean LibraryFirstSelection = TRUE;

extern void initialize_icons (void);
extern void initialize_library (void);
extern void enumerate_library (GTraverseFunc user_func, gpointer user_data);

typedef enum
  {
      UrlLink,
      MailtoLink,
      RecordLink,
      RecordParaLink
  }
LinkType;

typedef struct
  {
      LinkType type;
      gint start;
      gint end;
      gint record;
      gint para;
      gchar *url;
  }
Link;

typedef struct Page_s
  {
      int index;
      int page_id;
      plkr_Document *doc;
      GtkTextBuffer *formatted_view;

      /* The links tree is keyed by start-end ranges, and the data is Link structs */
      GTree *links;

      /* The paragraphs tree maps keys (para_no << 16 + record_no) to offsets */
      GTree *paragraphs;

      /* a list of all the hrules which need to be inserted for this buffer */
      GSList *hrules;

      Link *current_link;

  }
Page;

typedef struct
  {
      Page *page;
      int offset;
  }
HistoryNode;

#define GLIST_TO_HISTORY_NODE(x)        ((HistoryNode *)((x)->data))

typedef struct
  {
      plkr_Document *doc;
      GtkTextView *textview;
      GtkWindow *window;        /* top-level window */
      GTree *pages;
      GList *history;
      GList *current_node;
      double scale;             /* scaling factor for images */
  }
UI;

UI *MainUi;

static char *link_type_names[] =
{"url-link",
 "mailto-link",
 "record-link",
 "record-para-link"};


struct gpe_icon my_icons[] = {
  { "left", "left" },
  { "home", "home" },
  { "right", "right" },
  { "open", "open" },
  { "exit", "exit" },
  {NULL, NULL}
};


static GtkTextTagTable *
create_tags ()
{
    GtkTextTagTable *table;
    GtkTextTag *tag;

    table = gtk_text_tag_table_new ();

    tag = gtk_text_tag_new ("h1");
    g_object_set (tag, "scale", 2.0, "weight", PANGO_WEIGHT_BOLD, "justification", GTK_JUSTIFY_CENTER, NULL);
    gtk_text_tag_table_add (table, tag);

    tag = gtk_text_tag_new ("h2");
    g_object_set (tag, "scale", 1.8, "weight", PANGO_WEIGHT_BOLD, NULL);
    gtk_text_tag_table_add (table, tag);

    tag = gtk_text_tag_new ("h3");
    g_object_set (tag, "scale", 1.6, "weight", PANGO_WEIGHT_BOLD, NULL);
    gtk_text_tag_table_add (table, tag);

    tag = gtk_text_tag_new ("h4");
    g_object_set (tag, "scale", 1.4, "weight", PANGO_WEIGHT_BOLD, NULL);
    gtk_text_tag_table_add (table, tag);

    tag = gtk_text_tag_new ("h5");
    g_object_set (tag, "scale", 1.2, "weight", PANGO_WEIGHT_BOLD, NULL);
    gtk_text_tag_table_add (table, tag);

    tag = gtk_text_tag_new ("h6");
    g_object_set (tag, "scale", 1.0, "weight", PANGO_WEIGHT_BOLD, NULL);
    gtk_text_tag_table_add (table, tag);

    tag = gtk_text_tag_new ("tt");
    g_object_set (tag, "family", "monospace", NULL);
    gtk_text_tag_table_add (table, tag);

    tag = gtk_text_tag_new ("small");
    g_object_set (tag, "scale", 0.75, NULL);
    gtk_text_tag_table_add (table, tag);

    tag = gtk_text_tag_new ("italic");
    g_object_set (tag, "style", PANGO_STYLE_ITALIC, NULL);
    gtk_text_tag_table_add (table, tag);

    tag = gtk_text_tag_new ("bold");
    g_object_set (tag, "weight", PANGO_WEIGHT_BOLD, NULL);
    gtk_text_tag_table_add (table, tag);

    tag = gtk_text_tag_new ("underline");
    g_object_set (tag, "underline", PANGO_UNDERLINE_SINGLE, NULL);
    gtk_text_tag_table_add (table, tag);

    tag = gtk_text_tag_new ("strikethrough");
    g_object_set (tag, "strikethrough", TRUE, NULL);
    gtk_text_tag_table_add (table, tag);

    tag = gtk_text_tag_new ("centered");
    g_object_set (tag, "justification", GTK_JUSTIFY_CENTER, NULL);
    gtk_text_tag_table_add (table, tag);

    tag = gtk_text_tag_new ("left-justified");
    g_object_set (tag, "justification", GTK_JUSTIFY_LEFT, NULL);
    gtk_text_tag_table_add (table, tag);

    tag = gtk_text_tag_new ("right-justified");
    g_object_set (tag, "justification", GTK_JUSTIFY_RIGHT, NULL);
    gtk_text_tag_table_add (table, tag);

    tag = gtk_text_tag_new ("filled");
    g_object_set (tag, "justification", GTK_JUSTIFY_FILL, NULL);
    gtk_text_tag_table_add (table, tag);

    tag = gtk_text_tag_new ("color-000000");
    g_object_set (tag, "foreground", "#000000", NULL);
    gtk_text_tag_table_add (table, tag);

    tag = gtk_text_tag_new ("margins-03-03");
    g_object_set (tag, "left-margin", 3, "right-margin", 3, NULL);
    gtk_text_tag_table_add (table, tag);

    tag = gtk_text_tag_new ("url-link");
    g_object_set (tag, "underline", PANGO_UNDERLINE_DOUBLE, NULL);
    gtk_text_tag_table_add (table, tag);

    tag = gtk_text_tag_new ("mailto-link");
    g_object_set (tag, "underline", PANGO_UNDERLINE_DOUBLE, NULL);
    gtk_text_tag_table_add (table, tag);

    tag = gtk_text_tag_new ("record-link");
    g_object_set (tag, "underline", PANGO_UNDERLINE_DOUBLE, NULL);
    gtk_text_tag_table_add (table, tag);

    tag = gtk_text_tag_new ("record-para-link");
    g_object_set (tag, "underline", PANGO_UNDERLINE_DOUBLE, NULL);
    gtk_text_tag_table_add (table, tag);

    tag = gtk_text_tag_new ("selected-link");
    g_object_set (tag, "background", "pink", NULL);
    gtk_text_tag_table_add (table, tag);

    tag = gtk_text_tag_new ("overall_attributes");
    g_object_set (tag,
                  "wrap_mode", GTK_WRAP_WORD,
                  "editable", FALSE,
                  "indent", 0,
                  "pixels-above-lines", 8,
                  "pixels-below-lines", 2,
                  NULL);
    gtk_text_tag_table_add (table, tag);

    return table;
}

static gint
compare_links (gconstpointer p1, gconstpointer p2)
{
    if (((Link *) p1)->start < ((Link *) p2)->start)
        return -1;
    else if (((Link *) p1)->start > ((Link *) p2)->start)
        return 1;
    else
        return 0;
}

static gint
link_search (gconstpointer p1, gconstpointer p2)
{
    /* g_tree_search is broken; the direction is wrong.
       So we return the wrong values here to make it work.
     */

    if (((Link *) p1)->start <= GPOINTER_TO_INT (p2) && ((Link *) p1)->end > GPOINTER_TO_INT (p2))
        return 0;
    else if (((Link *) p1)->end <= GPOINTER_TO_INT (p2))
        return 1;
    else
        return -1;
}

static gint
compare_pointer_ints (gconstpointer p1, gconstpointer p2)
{
    if (GPOINTER_TO_UINT (p1) < GPOINTER_TO_UINT (p2))
        return -1;
    else if (GPOINTER_TO_UINT (p1) > GPOINTER_TO_UINT (p2))
        return 1;
    else
        return 0;
}

static int 
pagelist_get_page_id (GTree * list, int record_index)
{
    Page *p = (Page *) g_tree_lookup (list, GINT_TO_POINTER (record_index));

    if (p)
        return p->page_id;
    else
        return record_index;
}

static Page *
pagelist_add_record (GTree * list, int index)
{
    Page *newr;
    Page *ptr = (Page *) g_tree_lookup (list, GINT_TO_POINTER (index));

    if (ptr)
        return ptr;

    newr = (Page *) malloc (sizeof (Page));
    newr->index = index;
    newr->page_id = index;
    newr->formatted_view = NULL;
    newr->links = g_tree_new (compare_links);
    newr->paragraphs = g_tree_new (compare_pointer_ints);
    newr->current_link = NULL;
    newr->doc = NULL;
    newr->hrules = NULL;
    g_tree_insert (list, GINT_TO_POINTER (index), newr);
    return newr;
}

static void 
pagelist_set_page_id (GTree * list, int index, int page_id)
{
    Page *ptr = (Page *) g_tree_lookup (list, GINT_TO_POINTER (index));

    if (ptr == NULL)
        ptr = pagelist_add_record (list, index);

    ptr->page_id = page_id;
}

static void 
pagelist_add_link (GTree * list, int index, LinkType type, int start, int end, char *url, int record, int para)
{
    Page *ptr = (Page *) g_tree_lookup (list, GINT_TO_POINTER (index));
    Link *newl;

    if (ptr == NULL)
        ptr = pagelist_add_record (list, index);

    newl = (Link *) malloc (sizeof (Link));
    newl->start = start;
    newl->end = end;
    newl->type = type;
    newl->record = record;
    newl->para = para;
    newl->url = url;
    g_tree_insert (ptr->links, newl, newl);
}

typedef struct
{
    int size;
    int attributes;
}
ParagraphInfo;

static ParagraphInfo *
ParseParagraphInfo (unsigned char *bytes, int len, int *nparas)
{
    ParagraphInfo *paragraph_info;
    int j, n;

    n = (bytes[2] << 8) + bytes[3];
    paragraph_info = (ParagraphInfo *) malloc (sizeof (ParagraphInfo) * n);
    for (j = 0; j < n; j++)
      {
          paragraph_info[j].size = (bytes[8 + (j * 4) + 0] << 8) + bytes[8 + (j * 4) + 1];
          paragraph_info[j].attributes = (bytes[8 + (j * 4) + 2] << 8) + bytes[8 + (j * 4) + 3];
      }
    *nparas = n;
    return paragraph_info;
}

#define GET_FUNCTION_CODE_TYPE(x)               (((x)>>3) & 0x1F)
#define GET_FUNCTION_CODE_DATALEN(x)            ((x) & 0x7)

typedef struct
{
    char *tag_name;
    gint start;
}
FormatStart;

static GSList *
PushFormat (GSList * current, char *tag_name, GtkTextIter * where)
{
    FormatStart *n = (FormatStart *) malloc (sizeof (FormatStart));

    n->tag_name = g_strdup (tag_name);
    n->start = gtk_text_iter_get_offset (where);
/*
   fprintf (stderr, "Starting format '%s' at %d\n", tag_name, n->start);
 */
    return g_slist_prepend (current, n);
}

static GSList *
ApplyFormatting (GSList * current_formatting, char *tag_name, GtkTextIter * end)
{
    /* find the most recent format start with "tag_name", apply the named tag
       to start:point, and remove the format start from the stack */

    GSList *ptr;
    FormatStart *candidate;
    GtkTextIter start;
    GtkTextBuffer *buffer;

    for (ptr = current_formatting; ptr != NULL; ptr = ptr->next)
      {
          candidate = (FormatStart *) (ptr->data);
          if (strcmp (tag_name, candidate->tag_name) == 0)
            {
/*
   fprintf (stderr, "Applying tag '%s' to %d:%d\n", tag_name,
   candidate->start, gtk_text_iter_get_offset(end));
 */
                buffer = gtk_text_iter_get_buffer (end);
                gtk_text_buffer_get_iter_at_offset (buffer, &start, candidate->start);
                gtk_text_buffer_apply_tag_by_name (buffer, tag_name, &start, end);
                g_free (candidate->tag_name);
                free (candidate);
                return g_slist_remove (current_formatting, candidate);
            }
      }
    g_warning ("No starting tag for tag named '%s' found!\n", tag_name);
    return current_formatting;
}

#if 0
static gboolean
HasTag (GSList * current_formatting, char *tag_name)
{
    GSList *ptr;

    for (ptr = current_formatting; ptr != NULL; ptr = ptr->next)
      {
          if (strcmp (tag_name, ((FormatStart *) (ptr->data))->tag_name) == 0)
              return TRUE;
      }
    return FALSE;
}
#endif

static void
InsertText (GtkTextBuffer * buffer, GtkTextIter * point, char *chars, int charslen, int charset_mibenum)
{
    /* fprintf (stderr, "          inserting '%*.*s'\n", charslen, charslen, chars); */

    if (charset_mibenum != UTF8_CHARSET_MIBENUM)
      {
          gchar *newchars;
          gsize newcharslen, bytesread;
          char *charset_name;

          if (charset_mibenum == 0)
              charset_name = "ISO-8859-1";
          else
              charset_name = g_tree_lookup (CharsetNames, GINT_TO_POINTER (charset_mibenum));
          if (charset_name == NULL)
            {
                fprintf (stderr, "Unknown charset %d encountered.  Skipping text.\n", charset_mibenum);
            }
          else
            {
                newchars = g_convert_with_fallback (chars, charslen, "UTF-8", charset_name, NULL, &bytesread, &newcharslen, NULL);
                if (newchars)
                    gtk_text_buffer_insert (buffer, point, newchars, newcharslen);
                else
                    fprintf (stderr, "No conversion from charset '%s' to UTF-8.  Skipping text.\n", charset_name);
            }
      }
    else
      {
          gtk_text_buffer_insert (buffer, point, chars, charslen);
      }
}

static gboolean 
IsWhitespaceRun (char *ptr, int len)
{
    int i;
    gboolean status = TRUE;

    for (i = 0; i < len; i++)
        if (!isspace (ptr[i]))
          {
              status = FALSE;
              break;
          }
    /* fprintf (stderr, "status on '%*.*s' is %s\n", len, len, ptr, status ? "TRUE" : "FALSE"); */
    return status;
}

static Page *
get_image_page (GTree * pages, plkr_Document * doc, int id, gdouble scale)
{
    Page *page;
    GtkTextBuffer *formatted_view;
    GtkTextIter point, start;
    GdkPixbuf *image;

    if (TheTagsTable == NULL)
        TheTagsTable = create_tags ();

    page = pagelist_add_record (pages, id);

    if (page->formatted_view == NULL)
      {

          image = GetImageRecord (doc, id, scale);
          if (image == NULL)
            {
                g_warning ("Can't get bytes of image record %d", id);
                return NULL;
            }

          page->doc = doc;

          formatted_view = gtk_text_buffer_new (TheTagsTable);
          gtk_text_buffer_get_iter_at_offset (formatted_view, &point, 0);
          gtk_text_buffer_insert_pixbuf (formatted_view, &point, image);
          gtk_text_buffer_get_bounds (formatted_view, &start, &point);
          gtk_text_buffer_apply_tag_by_name (formatted_view, "centered", &start, &point);

          page->formatted_view = formatted_view;
          g_tree_insert (page->paragraphs, GUINT_TO_POINTER (id), GUINT_TO_POINTER (0));
      }

    return page;
}


static Page *
get_text_page (GTree * pages, plkr_Document * doc, int id, double scale)
{
    plkr_DataRecordType type;
    // int home_id = plkr_GetHomeRecordID (doc);
    Page *page;
    unsigned char *ptr, *run, *para_start, *start;
    int fctype, fclen, para_index, para_len, charset;
    gboolean first_record_of_page = TRUE, first_text_of_paragraph;
    int record_index;
    int nparagraphs;
    int current_font_code, current_alignment, current_color, current_margins;
    ParagraphInfo *paragraphs;
    // GtkWidget *separator;
    GtkTextBuffer *formatted_view;
    GtkTextIter point;
    GtkTextIter buffer_start, buffer_end;
    GSList *active_formatting = NULL;
    unsigned char record_bytes[MAX_RECORD_SIZE];
    int record_bytes_len;
    char charbuf[7];
    int utflen;
    gunichar unichar=0;

    LinkType current_link_type=0;
    int current_link_start=0;
    char *current_link_url=NULL;
    int current_link_record=0, current_link_para=0;
    gboolean current_link;
    int current_glyph_page = 0;

    gboolean on_new_line;
    static char *alignment_names[] =
    {"left-justified",
     "right-justified",
     "centered",
     "filled"
    };
    static char *style_names[] =
    {"normal",
     "h1",
     "h2",
     "h3",
     "h4",
     "h5",
     "h6",
     "bold",
     "tt",
     "small"
    };


    if (TheTagsTable == NULL)
        TheTagsTable = create_tags ();

    record_index = id;

    page = pagelist_add_record (pages, id);

    if (page->formatted_view == NULL)
      {

          record_bytes_len = plkr_CopyRecordBytes (doc, id, &record_bytes[0], sizeof (record_bytes), &type);
          if (record_bytes_len == 0)
            {
                g_warning ("Can't get bytes of record %d", id);
                return NULL;
            }

          page->doc = doc;
          charset = plkr_GetRecordCharset (doc, id);

          formatted_view = gtk_text_buffer_new (TheTagsTable);
          gtk_text_buffer_get_iter_at_offset (formatted_view, &point, 0);
          on_new_line = TRUE;

          paragraphs = ParseParagraphInfo (record_bytes, record_bytes_len, &nparagraphs);
          start = &record_bytes[0] + 8 + ((record_bytes[2] << 8) + record_bytes[3]) * 4;

          current_link = FALSE;

          for (para_index = 0, ptr = start, run = start; para_index < nparagraphs; para_index++)
            {

                para_len = paragraphs[para_index].size;

                /* If the paragraph is the last in the record, and it consists
                   of a link to the next record in the logical page, we trim off
                   the paragraph and instead insert the whole page */

                if (((para_index + 1) == nparagraphs) &&
                    (para_len == (sizeof ("Click here for the next part") + 5)) &&
                    (*ptr == 0) && (ptr[1] == ((PLKR_TFC_LINK << 3) + 2)) &&
                    (strcmp (ptr + 4, "Click here for the next part") == 0))
                  {

                      record_index = (ptr[2] << 8) + ptr[3];
                      if ((record_bytes_len = plkr_CopyRecordBytes (doc, record_index, &record_bytes[0], sizeof (record_bytes), &type)) == 0)
                        {
                            g_warning ("Can't open record %d!", record_index);
                            return FALSE;
                        }
                      else if (!(type == PLKR_DRTYPE_TEXT_COMPRESSED ||
                                 type == PLKR_DRTYPE_TEXT))
                        {
                            g_warning ("Bad record type %d in record linked from end of record %d",
                                       type, id);
                            return FALSE;
                        }
                      first_record_of_page = FALSE;
                      para_index = 0;
                      ptr = &record_bytes[0] + 8 + ((record_bytes[2] << 8) + record_bytes[3]) * 4;
                      run = ptr;
                      free (paragraphs);
                      paragraphs = ParseParagraphInfo (record_bytes, record_bytes_len, &nparagraphs);
                      para_len = paragraphs[para_index].size;
                      pagelist_set_page_id (pages, record_index, id);
                  }

                /* If a paragraph is the first in a record, and it consists
                   of a link to the previous record in the logical page, we trim it off */

                if ((para_index == 0) && !first_record_of_page &&
                    (*ptr == 0) && (ptr[1] == ((PLKR_TFC_LINK << 3) + 2)) &&
                (strcmp (ptr + 4, "Click here for the previous part") == 0))
                  {
                      /* throw away this inserted paragraph */
                      ptr += para_len;
                      run = ptr;
                      continue;
                  }

                /* at the beginning of a paragraph, we start with a clean graphics context */

                if (!on_new_line)
                  {
                      InsertText (formatted_view, &point, "\n", 1, charset);
                      on_new_line = TRUE;
                  }

                first_text_of_paragraph = TRUE;

                g_tree_insert (page->paragraphs,
                        GINT_TO_POINTER ((para_index << 16) + record_index),
                       GINT_TO_POINTER (gtk_text_iter_get_offset (&point)));

                current_font_code = 0;
                current_color = 0x0;
                active_formatting = PushFormat (active_formatting, "color-000000", &point);
                current_alignment = 0;
                active_formatting = PushFormat (active_formatting, "left-justified", &point);
                current_margins = (3 << 8) + 3;
                active_formatting = PushFormat (active_formatting, "margins-03-03", &point);

                for (para_start = ptr, run = ptr; (ptr - para_start) < para_len;)
                  {

                      if (*ptr == 0)
                        {
                            /* function code */

                            if ((ptr - run) > 0)
                              {
                                  if (!on_new_line || !IsWhitespaceRun (run, ptr - run))
                                    {
                                        InsertText (formatted_view, &point, run, (ptr - run), charset);
/*
   fprintf (stderr, "para %d:  wrote text:  '%*.*s'\n", para_index,
   (ptr-run), (ptr-run),  run);
 */
                                        on_new_line = FALSE;
                                    }
                                  run = ptr;
                              }

                            ptr++;
                            fctype = GET_FUNCTION_CODE_TYPE (*ptr);
                            fclen = GET_FUNCTION_CODE_DATALEN (*ptr);
                            ptr++;

                            if (fctype == PLKR_TFC_NEWLINE)
                              {

                                  /* skip it if it's at the end of a paragraph */

                                  /* fprintf (stderr, "para %d:  newline\n", para_index); */

                                  if ((ptr - para_start + fclen) < para_len)
                                    {

                                        if (UseUnicode && !UnicodeNewlineSeparatorBroken)
                                          {

                                              InsertText (formatted_view, &point, UNICODE_LS_STRING, -1, UTF8_CHARSET_MIBENUM);

                                          }
                                        else
                                          {

                                              if (!on_new_line)
                                                {
                                                    InsertText (formatted_view, &point, "\n", 1, charset);
                                                    on_new_line = TRUE;
                                                }

                                          }
                                        on_new_line = TRUE;

                                    }

                              }
                            else if (fctype == PLKR_TFC_FONT)
                              {

                                  if (current_font_code != *ptr)
                                    {
                                        if (current_font_code > 0 && current_font_code < 10)
                                            active_formatting = ApplyFormatting (active_formatting,
                                             style_names[current_font_code],
                                                                    &point);
                                        current_font_code = *ptr;
                                        if (current_font_code > 0 && current_font_code < 10)
                                            active_formatting = PushFormat (active_formatting,
                                             style_names[current_font_code],
                                                                    &point);
                                    }

                              }
                            else if (fctype == PLKR_TFC_ALIGN)
                              {

                                  /* fprintf (stderr, "Alignment code %d\n", *ptr); */

                                  if ((current_alignment != *ptr) && (*ptr < 4))
                                    {
                                        if (current_alignment < 4 && current_alignment >= 0)
                                            active_formatting = ApplyFormatting (active_formatting, alignment_names[current_alignment], &point);
                                        current_alignment = *ptr;
                                        if (current_alignment < 4 && current_alignment >= 0)
                                            active_formatting = PushFormat (active_formatting, alignment_names[current_alignment], &point);
                                    }

                              }
                            else if (fctype == PLKR_TFC_BITALIC)
                              {

                                  /* fprintf (stderr, "para %d:  start_italics\n", para_index); */
                                  active_formatting = PushFormat (active_formatting, "italic", &point);

                              }
                            else if (fctype == PLKR_TFC_EITALIC)
                              {

                                  /* fprintf (stderr, "para %d:  end_italics\n", para_index); */
                                  active_formatting = ApplyFormatting (active_formatting, "italic", &point);

                              }
                            else if (fctype == PLKR_TFC_BULINE)
                              {

                                  active_formatting = PushFormat (active_formatting, "underline", &point);

                              }
                            else if (fctype == PLKR_TFC_EULINE)
                              {

                                  active_formatting = ApplyFormatting (active_formatting, "underline", &point);

                              }
                            else if (fctype == PLKR_TFC_BSTRIKE)
                              {

                                  active_formatting = PushFormat (active_formatting, "strikethrough", &point);

                              }
                            else if (fctype == PLKR_TFC_ESTRIKE)
                              {

                                  active_formatting = ApplyFormatting (active_formatting, "strikethrough", &point);

                              }
                            else if (fctype == PLKR_TFC_UCHAR)
                              {

                                  if (UseUnicode)
                                    {

                                        if (fclen == 3)
                                            unichar = READ_BIGENDIAN_SHORT (ptr + 1);
                                        else if (fclen == 5)
                                            unichar = READ_BIGENDIAN_LONG (ptr + 1);

                                        utflen = g_unichar_to_utf8 (unichar, charbuf);
                                        charbuf[utflen] = 0;
                                        InsertText (formatted_view, &point, charbuf, -1, UTF8_CHARSET_MIBENUM);
                                        on_new_line = FALSE;
                                        first_text_of_paragraph = FALSE;

                                        /* skip over alternate text */
                                        ptr += ptr[0];

                                    }

                              }
                            else if (fctype == PLKR_TFC_MARGINS)
                              {

                                  int margins;
                                  char margins_spec[16];
                                  GtkTextTag *margins_tag;

                                  margins = (ptr[0] << 8) + ptr[1];
                                  if (current_margins != margins)
                                    {
                                        sprintf (margins_spec, "margins-%02x-%02x", current_margins >> 8, current_margins & 0xFF);
                                        active_formatting = ApplyFormatting (active_formatting, margins_spec, &point);
                                        sprintf (margins_spec, "margins-%02x-%02x", ptr[0], ptr[1]);
                                        margins_tag = gtk_text_tag_table_lookup (TheTagsTable, margins_spec);
                                        if (margins_tag == NULL)
                                          {
                                              margins_tag = gtk_text_tag_new (margins_spec);
                                              g_object_set (margins_tag,
                                                      "left-margin", ptr[0],
                                                     "right-margin", ptr[1],
                                                            NULL);
                                              gtk_text_tag_table_add (TheTagsTable, margins_tag);
                                          }
                                        current_margins = margins;
                                        active_formatting = PushFormat (active_formatting, margins_spec, &point);
                                    }

                              }
                            else if (fctype == PLKR_TFC_COLOR)
                              {

                                  char color_spec[16];
                                  GtkTextTag *color_tag;
                                  int color;

                                  color = (ptr[0] << 16) + (ptr[1] << 8) + ptr[2];
                                  if (color != current_color)
                                    {
                                        sprintf (color_spec, "color-%06x", current_color);
                                        active_formatting = ApplyFormatting (active_formatting, color_spec, &point);
                                        sprintf (color_spec, "color-%06x", color);
                                        color_tag = gtk_text_tag_table_lookup (TheTagsTable, color_spec);
                                        if (color_tag == NULL)
                                          {
                                              color_tag = gtk_text_tag_new (color_spec);
                                              sprintf (color_spec, "#%06x", color);
                                              g_object_set (color_tag, "foreground", color_spec, NULL);
                                              gtk_text_tag_table_add (TheTagsTable, color_tag);
                                              sprintf (color_spec, "color-%06x", color);
                                          }
                                        current_color = color;
                                        active_formatting = PushFormat (active_formatting, color_spec, &point);
                                    }

                              }
                            else if (fctype == PLKR_TFC_LINK)
                              {

                                  // char link_end_spec[20];
                                  gboolean real_record;
                                  // int record_id, paragraph_id, datalen;
                                  // plkr_DataRecordType type = 0;
                                  // char *url = NULL;
                                  GtkTextIter link_start_iter;

                                  if (fclen == 0)
                                    {
                                        if (current_link)
                                          {
                                              gtk_text_buffer_get_iter_at_offset (formatted_view, &link_start_iter, current_link_start);
                                              gtk_text_buffer_apply_tag_by_name (formatted_view,
                                                                                 link_type_names[current_link_type],
                                                  &link_start_iter, &point);
                                              pagelist_add_link (pages, id, current_link_type, current_link_start, gtk_text_iter_get_offset (&point),
                                                                 current_link_url, current_link_record, current_link_para);
                                              current_link = FALSE;
                                          }
                                        else
                                          {
                                              g_warning ("Link end code encounted while no link is active!");
                                          }
                                    }
                                  else if (current_link)
                                    {

                                        g_warning ("New link encounted while old link is still active!");

                                    }
                                  else
                                    {

                                        current_link = TRUE;
                                        current_link_record = READ_BIGENDIAN_SHORT (ptr);
                                        current_link_start = gtk_text_iter_get_offset (&point);
                                        real_record = plkr_HasRecordWithID (doc, current_link_record);

                                        if (!real_record)
                                          {
                                              /* must be a URL that wasn't plucked */
                                              current_link_url = plkr_GetRecordURL (doc, current_link_record);
                                              current_link_type = UrlLink;

                                          }
                                        else if (fclen == 2)
                                          {
                                              plkr_DataRecordType t = plkr_GetRecordType (doc, current_link_record);
                                              if (t == PLKR_DRTYPE_MAILTO)
                                                  current_link_type = MailtoLink;
                                              else
                                                  current_link_type = RecordLink;

                                          }
                                        else if (fclen == 4)
                                          {
                                              current_link_para = READ_BIGENDIAN_SHORT (ptr + 2);
                                              current_link_type = RecordParaLink;

                                          }
                                        else
                                          {
                                              g_warning ("Bad length in LINK function code encountered (%d)", fclen);
                                          }
                                    }

                              }
                            else if (fctype == PLKR_TFC_IMAGE)
                              {

                                  GdkPixbuf *image;

                                  image = GetImageRecord (doc, READ_BIGENDIAN_SHORT (ptr), scale);
                                  gtk_text_buffer_insert_pixbuf (formatted_view, &point, image);
                                  on_new_line = FALSE;
                                  first_text_of_paragraph = FALSE;

                              }
                            else if (fctype == PLKR_TFC_HRULE)
                              {

                                  GtkTextChildAnchor *anchor;
                                  int start_pos;
                                  GtkTextIter start_iter;

                                  if (!on_new_line)
                                      InsertText (formatted_view, &point, "\n", 1, charset);
                                  if (current_alignment != 2)
                                      active_formatting = ApplyFormatting (active_formatting, alignment_names[current_alignment], &point);
                                  start_pos = gtk_text_iter_get_offset (&point);
                                  anchor = gtk_text_buffer_create_child_anchor (formatted_view, &point);
                                  gtk_text_buffer_get_iter_at_offset (formatted_view, &start_iter, start_pos);
                                  gtk_text_buffer_apply_tag_by_name (formatted_view, "centered", &start_iter, &point);
                                  if (current_alignment != 2)
                                      active_formatting = PushFormat (active_formatting, alignment_names[current_alignment], &point);
                                  page->hrules = g_slist_prepend (page->hrules, anchor);
                                  InsertText (formatted_view, &point, "\n", 1, charset);
                                  on_new_line = TRUE;

                              }
                            else if (fctype == PLKR_TFC_GLYPH)
                              {

                                  if (fclen == 0)
                                    {

                                        /* end of glyph run */
                                        /* fprintf (stderr, "finished with glyph page %d\n", current_glyph_page); */
                                        current_glyph_page = 0;

                                    }
                                  else if (fclen == 2)
                                    {

                                        /* shift to different glyph page, possible start of glyph run */
                                        int val = READ_BIGENDIAN_SHORT (ptr);
                                        /* fprintf (stderr, "finished with glyph page %d, starting with %d\n", current_glyph_page, val); */
                                        current_glyph_page = val;

                                    }
                                  else if (fclen == 4)
                                    {

                                        /* adjust page position -- we'll ignore this for now */

                                    }
                                  else if (fclen == 6)
                                    {

                                        /* start of glyph run */
                                        current_glyph_page = READ_BIGENDIAN_SHORT (ptr);
                                        /* fprintf (stderr, "starting glyph page %d\n", current_glyph_page); */
                                    }

                              }
                            else if (fctype == PLKR_TFC_IMAGE2)
                              {

                                  GdkPixbuf *image;

                                  image = GetImageRecord (doc, READ_BIGENDIAN_SHORT (ptr + 2), scale);
                                  gtk_text_buffer_insert_pixbuf (formatted_view, &point, image);
                                  on_new_line = FALSE;
                                  first_text_of_paragraph = FALSE;

                              }
                            else
                              {

                                  fprintf (stderr, "unhandled function code %d encountered\n", fctype);
                              }

                            ptr += fclen;
                            run = ptr;

                        }
                      else if (current_glyph_page != 0)
                        {

                            GdkPixbuf *image;

                            if (*ptr < 16 && *ptr > 0)
                              {
                                  /* vspace */
                                  int width = (*ptr * ScreenDPI) / 75;
                                  if (width >= 1) {
                                    image = GetSpacerPixbuf (width);
                                    if (!image)
                                      g_warning ("Couldn't get %d-wide spacer pixmap", width);
                                    else
                                      {
                                        gtk_text_buffer_insert_pixbuf (formatted_view, &point, image);
                                        on_new_line = FALSE;
                                        first_text_of_paragraph = FALSE;
                                      }
                                  }
                              }
                            else if (*ptr > 15 && *ptr < 32)
                              {
                                  /* vadjust for following image -- no way to implement this yet */
                              }
                            else
                              {
                                  image = GetGlyph (doc, current_glyph_page, (*ptr) - 32, ScreenDPI);
                                  if (!image)
                                      g_warning ("Couldn't get glyph %d/%d", current_glyph_page, (*ptr) - 32);
                                  else
                                    {
                                        gtk_text_buffer_insert_pixbuf (formatted_view, &point, image);
                                        on_new_line = FALSE;
                                        first_text_of_paragraph = FALSE;
                                    }
                              }
                            ptr++;
                            run = ptr;

                        }
                      else if (first_text_of_paragraph && isspace (*ptr))
                        {

                            /* leading whitespace -- just advance pointer */
                            ptr++;
                            run = ptr;

                        }
                      else
                        {

                            /* regular text character */
                            ptr++;
                            first_text_of_paragraph = FALSE;

                        }

                  }

                /* at the end of the paragraph */
                if ((ptr - run) > 0)
                  {

                      if (!IsWhitespaceRun (run, (ptr - run)))
                        {
                            InsertText (formatted_view, &point, run, (ptr - run), charset);
                            /* fprintf (stderr, "para %d:  wrote text:  '%*.*s'\n", para_index,
                               (ptr-run), (ptr-run),  run);
                             */
                            on_new_line = FALSE;
                        }

                      run = ptr;
                  }

                if (current_glyph_page > 0)
                  {
                      /* end of glyph run */
                      /* fprintf (stderr, "finished with glyph page %d\n", current_glyph_page); */
                      current_glyph_page = 0;
                  }

                /* finish up any active formatting */
                while (active_formatting != NULL)
                  {
                      active_formatting = ApplyFormatting (active_formatting,
                      ((FormatStart *) (active_formatting->data))->tag_name,
                                                           &point);
                  }

                /* put in a paragraph separator */
                if (UseUnicode)
                  {

                      InsertText (formatted_view, &point, UNICODE_PS_STRING, -1, UTF8_CHARSET_MIBENUM);

                  }
                else
                  {

                      if (!on_new_line)
                        {
                            InsertText (formatted_view, &point, "\n", 1, charset);
                            on_new_line = TRUE;
                        }

                  }

            }

          gtk_text_buffer_get_bounds (formatted_view, &buffer_start, &buffer_end);
          gtk_text_buffer_apply_tag_by_name (formatted_view, "overall_attributes", &buffer_start, &buffer_end);

          page->formatted_view = formatted_view;
      }

    return page;
}


static void
add_hrule_to_view (gpointer data, gpointer data2)
{
    GtkTextView *textview = (GtkTextView *) data2;
    GtkTextChildAnchor *anchor = (GtkTextChildAnchor *) data;
    GtkWidget *separator;

    separator = gtk_hseparator_new ();
    gtk_text_view_add_child_at_anchor (textview, separator, anchor);
    gtk_widget_set_size_request (separator, 70, -1);
    gtk_widget_show_all (separator);
}


static void 
AddHistoryNode (UI * ui, Page * page, int offset)
{
    HistoryNode *hnode;

    hnode = (HistoryNode *) malloc (sizeof (HistoryNode));
    hnode->page = page;
    hnode->offset = offset;
    ui->history = g_list_append (ui->history, hnode);
    ui->current_node = g_list_last (ui->history);
}

static gboolean 
show_page (UI * ui, Page * page, int offset)
{
    // GtkWidget *separator;
    // HistoryNode *hnode;
    GtkTextIter offset_iter;
    GtkTextMark *offset_mark;

    /* fprintf (stderr, "show page:  %d, %d\n", page->index, offset); */

    gtk_text_view_set_buffer (GTK_TEXT_VIEW (ui->textview), page->formatted_view);

    g_slist_foreach (page->hrules, add_hrule_to_view, ui->textview);

    /* we want to do this right, so we need to use a GtkTextMark instead of
       a simple GtkTextIter, because using a mark apparently forces the calculation
       of actual line heights, etc. */
    gtk_text_buffer_get_iter_at_offset (page->formatted_view, &offset_iter, offset);
    offset_mark = gtk_text_buffer_create_mark (page->formatted_view, NULL, &offset_iter, TRUE);
    gtk_text_view_scroll_to_mark (ui->textview, offset_mark, 0.0, TRUE, 0.0, 0.0);
    gtk_text_buffer_delete_mark (page->formatted_view, offset_mark);

    return TRUE;
}

static gboolean
show_record (plkr_Document * doc, int record_index, UI * ui, int offset)
{
    plkr_DataRecordType type;
    // unsigned char *data;
    // int data_len;
    gboolean status = TRUE;
    Page *page;

    type = plkr_GetRecordType (doc, record_index);

    if (type == PLKR_DRTYPE_TEXT_COMPRESSED || type == PLKR_DRTYPE_TEXT)
      {

          status = (page = get_text_page (ui->pages, doc, record_index, ui->scale)) &&
              show_page (ui, page, offset);
          if (status)
              AddHistoryNode (ui, page, offset);

      }
    else if (type == PLKR_DRTYPE_IMAGE_COMPRESSED || type == PLKR_DRTYPE_IMAGE)
      {

          status = (page = get_image_page (ui->pages, doc, record_index, ui->scale)) &&
              show_page (ui, page, offset);
          if (status)
              AddHistoryNode (ui, page, offset);

      }
    else
      {

          fprintf (stderr, "Invalid record type %d for record %d\n", type, record_index);
          status = FALSE;

      }

    return status;
}

static char *
url_escape_text (char *text_to_escape)
{
    char *retval = g_strdup (text_to_escape);
    int i, j, inlen, outlen;
    char *ret;

    for (i = 0, j = 0, inlen = strlen (text_to_escape), outlen = inlen; i < inlen; i++)
      {
          if (g_ascii_isalnum (text_to_escape[i]))
            {
                retval[j] = text_to_escape[i];
                j += 1;
            }
          else
            {
                ret = (char *) malloc (outlen + 2 + 1);
                strncpy (ret, retval, j);
                sprintf (ret + j, "%%%02x", text_to_escape[i]);
                j += 3;
                free (retval);
                retval = ret;
                outlen += 2;
            }
      }
    retval[j] = 0;
    return retval;
}

static char *
get_mailto_link_as_URL (plkr_Document * doc, int record_number)
{
    int to_offset, cc_offset, subject_offset, body_offset, record_bytes_len;
    // int url_size;
    unsigned char record_bytes[MAX_RECORD_SIZE];
    char *url, *old_url, *escaped_text;
    unsigned char *bytes;
    plkr_DataRecordType type;
    gboolean first = TRUE;


    record_bytes_len = plkr_CopyRecordBytes (doc, record_number, &record_bytes[0], sizeof (record_bytes), &type);
    if (type != PLKR_DRTYPE_MAILTO)
        return NULL;

    bytes = &record_bytes[0] + 8;

    to_offset = (bytes[0] << 8) + bytes[1];
    cc_offset = (bytes[2] << 8) + bytes[3];
    subject_offset = (bytes[4] << 8) + bytes[5];
    body_offset = (bytes[6] << 8) + bytes[7];

    url = g_strdup ("mailto:");

    if (to_offset != 0)
      {
          old_url = url;
          escaped_text = url_escape_text (bytes + to_offset);
          url = g_strdup_printf ("%s%s", url, escaped_text);
          free (old_url);
          free (escaped_text);
      }

    if (cc_offset != 0)
      {
          old_url = url;
          escaped_text = url_escape_text (bytes + cc_offset);
          url = g_strdup_printf ("%s%scc=%s", url, (first ? "?" : "&"), escaped_text);
          free (old_url);
          free (escaped_text);
          first = FALSE;
      }

    if (subject_offset != 0)
      {
          old_url = url;
          escaped_text = url_escape_text (bytes + subject_offset);
          url = g_strdup_printf ("%s%ssubject=%s", url, (first ? "?" : "&"), escaped_text);
          free (old_url);
          free (escaped_text);
          first = FALSE;
      }

    if (body_offset != 0)
      {
          old_url = url;
          escaped_text = url_escape_text (bytes + body_offset);
          url = g_strdup_printf ("%s%sbody=%s", url, (first ? "?" : "&"), escaped_text);
          free (old_url);
          free (escaped_text);
          first = FALSE;
      }

    return url;
}


static void
follow_link (Link * link, UI * ui)
{
    int id, para_offset;
    Page *page;
    plkr_Document *doc;
    GtkTextIter where;
    int buffer_x, buffer_y;
    GList *freednode;

    doc = GLIST_TO_HISTORY_NODE (ui->current_node)->page->doc;

    if (link->type == RecordLink || link->type == RecordParaLink)
      {
          /* free tail of history list -- taking a new branch */
          while ((freednode = ui->current_node->next) != NULL)
            {
                free (GLIST_TO_HISTORY_NODE (freednode));
                ui->history = g_list_remove_link (ui->history, freednode);
                g_list_free_1 (freednode);
            }
      }

    if (link->type == RecordLink)
      {

          /* fprintf (stderr, "Following link to record %d\n", link->record); */

          gtk_text_view_window_to_buffer_coords (ui->textview, GTK_TEXT_WINDOW_WIDGET, 0, 0, &buffer_x, &buffer_y);
          gtk_text_view_get_iter_at_location (ui->textview, &where, buffer_x, buffer_y);
          GLIST_TO_HISTORY_NODE (ui->current_node)->offset = gtk_text_iter_get_offset (&where);
          show_record (doc, link->record, ui, 0);

      }
    else if (link->type == RecordParaLink)
      {

          id = pagelist_get_page_id (ui->pages, link->record);

          page = get_text_page (ui->pages, doc, id, ui->scale);
          para_offset = GPOINTER_TO_UINT (g_tree_lookup (page->paragraphs,
                      GINT_TO_POINTER ((link->para << 16) + link->record)));

          /* fprintf (stderr, "Following link to record %d (%d), para %d; offset is %d\n", link->record, id, link->para, para_offset); */

          gtk_text_view_window_to_buffer_coords (ui->textview, GTK_TEXT_WINDOW_WIDGET, 0, 0, &buffer_x, &buffer_y);
          gtk_text_view_get_iter_at_location (ui->textview, &where, buffer_x, buffer_y);
          GLIST_TO_HISTORY_NODE (ui->current_node)->offset = gtk_text_iter_get_offset (&where);

          show_record (doc, id, ui, para_offset);

      }
    else if (link->type == UrlLink)
      {

          GtkWidget *popup;
          // char *urltext;

          if (link->url)
              popup = gtk_message_dialog_new (GTK_WINDOW (ui->window), 0, GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
                                        "URL not included:\n%s", link->url);
          else
              popup = gtk_message_dialog_new (GTK_WINDOW (ui->window), 0, GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
                           "No such record in document:  %d", link->record);
          (void) gtk_dialog_run (GTK_DIALOG (popup));
          gtk_widget_destroy (popup);

      }
    else if (link->type == MailtoLink)
      {

          GtkWidget *popup;
          char *urltext;

          urltext = get_mailto_link_as_URL (GLIST_TO_HISTORY_NODE (ui->current_node)->page->doc, link->record);
          popup = gtk_message_dialog_new (GTK_WINDOW (ui->window), 0, GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
                                          "'mailto' link to:\n%s", urltext);
          (void) gtk_dialog_run (GTK_DIALOG (popup));
          gtk_widget_destroy (popup);
          free (urltext);
      }

}

static void
back_callback (void)
{
    GList *previous;
    GtkTextIter where;
    int buffer_x, buffer_y;

    UI *ui = (UI *) MainUi;

    if ((previous = g_list_previous (ui->current_node)))
      {
          gtk_text_view_window_to_buffer_coords (ui->textview, GTK_TEXT_WINDOW_WIDGET, 0, 0, &buffer_x, &buffer_y);
          gtk_text_view_get_iter_at_location (ui->textview, &where, buffer_x, buffer_y);
          GLIST_TO_HISTORY_NODE (ui->current_node)->offset = gtk_text_iter_get_offset (&where);
          ui->current_node = previous;
          show_page (ui, GLIST_TO_HISTORY_NODE (previous)->page,
                     GLIST_TO_HISTORY_NODE (previous)->offset);
      }
}

static void
home_callback (void)
{
    UI *ui = (UI *) MainUi;
    GtkTextIter where;
    int buffer_x, buffer_y;
    plkr_Document *doc;

    gtk_text_view_window_to_buffer_coords (ui->textview, GTK_TEXT_WINDOW_WIDGET, 0, 0, &buffer_x, &buffer_y);
    gtk_text_view_get_iter_at_location (ui->textview, &where, buffer_x, buffer_y);
    GLIST_TO_HISTORY_NODE (ui->current_node)->offset = gtk_text_iter_get_offset (&where);
    doc = GLIST_TO_HISTORY_NODE (ui->current_node)->page->doc;

    show_record (doc, plkr_GetHomeRecordID (doc), ui, 0);
}

static void
next_callback (void)
{
    GList *next;
    GtkTextIter where;
    int buffer_x, buffer_y;
    UI *ui = (UI *) MainUi;

    if ((next = g_list_next (ui->current_node)))
      {
          gtk_text_view_window_to_buffer_coords (ui->textview, GTK_TEXT_WINDOW_WIDGET, 0, 0, &buffer_x, &buffer_y);
          gtk_text_view_get_iter_at_location (ui->textview, &where, buffer_x, buffer_y);
          GLIST_TO_HISTORY_NODE (ui->current_node)->offset = gtk_text_iter_get_offset (&where);
          ui->current_node = next;
          show_page (ui, GLIST_TO_HISTORY_NODE (next)->page,
                     GLIST_TO_HISTORY_NODE (next)->offset);
      }
}


#if 0
static void
reload_callback (GtkButton * button, gpointer * user_data)
{
    // UI *ui = (UI *) user_data;
    fprintf (stderr, "Reload:  %p\n", user_data);
}
#endif


static void
quit_callback (void /*GtkButton * button, gpointer * user_data*/)
{
    // fprintf (stderr, "Quit:  %p\n", user_data);
    exit (0);
}

#if 0
static void
scale_callback (GtkAdjustment * adjustment, gpointer * user_data)
{
    UI *ui = (UI *) user_data;

    ui->scale = gtk_adjustment_get_value (adjustment);
}
#endif

static gboolean
mouse_click_callback (GtkWidget * widget,
                      GdkEventButton * event,
                      gpointer user_data)
{
    gint buffer_x, buffer_y;
    GtkTextBuffer *buffer;
    GtkTextIter where, start, end;
    // GSList *tags, *ptr;
    // GtkTextTag *tag;
    int offset;
    Page *rnode;
    Link *link;
    UI *ui;

    /*
       fprintf (stderr, "State is %d, type is %d, button is %d, x is %f, y is %f\n",
       event->state, event->type, event->button, event->x, event->y);
     */

    ui = (UI *) user_data;
    rnode = GLIST_TO_HISTORY_NODE (ui->current_node)->page;
    gtk_text_view_window_to_buffer_coords (GTK_TEXT_VIEW (widget), GTK_TEXT_WINDOW_WIDGET, (gint) (event->x), (gint) (event->y), &buffer_x, &buffer_y);
    buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (widget));
    gtk_text_view_get_iter_at_location (GTK_TEXT_VIEW (widget), &where, buffer_x, buffer_y);
    offset = gtk_text_iter_get_offset (&where);
/*
   fprintf (stderr, "Buffer 0x%p coords are %d, %d; offset is %d\n", buffer,
   buffer_x, buffer_y, offset);
 */
    link = (Link *) g_tree_search (rnode->links, link_search, GINT_TO_POINTER (offset));
    if (rnode->current_link == NULL && link && event->type == GDK_BUTTON_PRESS)
      {
          /* fprintf (stderr, "** Link of type %s, from %d to %d\n", link_type_names[link->type], link->start, link->end); */
          gtk_text_buffer_get_iter_at_offset (rnode->formatted_view, &start, link->start);
          gtk_text_buffer_get_iter_at_offset (rnode->formatted_view, &end, link->end);
          gtk_text_buffer_apply_tag_by_name (rnode->formatted_view, "selected-link", &start, &end);
          gtk_widget_queue_draw (widget);
          rnode->current_link = link;
      }
    else if (link && rnode->current_link == link && event->type == GDK_BUTTON_RELEASE)
      {
          /* fprintf (stderr, "== Invoke link ==\n"); */
          follow_link (link, ui);
      }
    if (event->type == GDK_BUTTON_RELEASE && rnode->current_link)
      {
          gtk_text_buffer_get_iter_at_offset (rnode->formatted_view, &start, rnode->current_link->start);
          gtk_text_buffer_get_iter_at_offset (rnode->formatted_view, &end, rnode->current_link->end);
          gtk_text_buffer_remove_tag_by_name (rnode->formatted_view, "selected-link", &start, &end);
          rnode->current_link = NULL;
      }
    return FALSE;
}

static void 
ui_free_document (UI * ui)
{
}


static void 
ui_show_document (UI * ui, char *docpath)
{
    plkr_Document *doc;
    GTree *tree;
    int i;

    if (ui->current_node)
        ui_free_document (ui);

    doc = plkr_OpenDBFile (docpath);
    if (!doc)
      {
          fprintf (stderr, "Error opening document %s\n", docpath);
	  gpe_error_box("Error opening document! Exiting!");
          exit(1);
      }

    i = plkr_GetHomeRecordID (doc);
    tree = g_tree_new (compare_pointer_ints);
    pagelist_add_record (tree, i);
    ui->pages = tree;

    gtk_window_set_title (GTK_WINDOW (ui->window), plkr_GetName (doc));

    (void) show_record (doc, i, ui, 0);
}


extern GtkWidget *
  create_image_button (char *, void (*)(gpointer), gpointer);

static gboolean
add_library_element (char *docname, DocInfo * info, GtkListStore * list)
{
    GtkTreeIter iter;

    gtk_list_store_append (list, &iter);
    gtk_list_store_set (list, &iter, 0, (info->title ? info->title : docname), 1, info->path, -1);

    return FALSE;
}

static void library_callback (void);
static UI *ui_create ();

static void
doc_selection_callback (GtkTreeSelection * selection, gpointer user_data)
{
    UI *newui;
    GtkTreeIter iter;
    GtkTreeModel *model;
    gchar *name, *path;

    if (gtk_tree_selection_get_selected (selection, &model, &iter))
      {
          gtk_tree_model_get (model, &iter, 0, &name, 1, &path, -1);
          fprintf (stderr, "Selection changed to %s, %s\n", name, path);

          if (LibraryFirstSelection)
            {
                /* ignore first selection, which is automatic */
                LibraryFirstSelection = FALSE;
            }
          else
            {
                newui = ui_create ();
                ui_show_document (newui, path);
            }

          g_free (name);
          g_free (path);
      }
}

static void
library_callback (void)
{
    // UI *ui = (UI *) MainUi;
    GtkWidget *window;
    GtkScrolledWindow *sw;
    GtkListStore *list;
    // GtkTreeIter iter;
    GtkTreeView *view;
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;
    GtkTreeSelection *select;

    if (TheLibrary == NULL)
      {

          initialize_library ();
          LibraryFirstSelection = TRUE;

          window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
          gtk_window_set_default_size (GTK_WINDOW (window), 240, 300);
          gtk_window_set_title (GTK_WINDOW (window), "Document Library");

          g_signal_connect (window, "destroy",
                            G_CALLBACK (gtk_widget_destroyed), &window);

          gtk_widget_set_name (window, "PluckerLibrary");
          gtk_container_set_border_width (GTK_CONTAINER (window), 0);

          list = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_STRING);
          enumerate_library ((GTraverseFunc) add_library_element, list);
          view = (GtkTreeView *) gtk_tree_view_new_with_model (GTK_TREE_MODEL (list));

          renderer = gtk_cell_renderer_text_new ();
          column = gtk_tree_view_column_new_with_attributes ("Document",
                                                             renderer,
                                                             "text",
                                                             0, NULL);
          gtk_tree_view_append_column (GTK_TREE_VIEW (view), column);

          renderer = gtk_cell_renderer_text_new ();
          column = gtk_tree_view_column_new_with_attributes ("Path",
                                                             renderer,
                                                             "text",
                                                             1, NULL);
          gtk_tree_view_append_column (GTK_TREE_VIEW (view), column);

          select = gtk_tree_view_get_selection (GTK_TREE_VIEW (view));
          gtk_tree_selection_set_mode (select, GTK_SELECTION_BROWSE);
          /* gtk_tree_selection_unselect_all (select); */
          g_signal_connect (G_OBJECT (select), "changed",
                            G_CALLBACK (doc_selection_callback), NULL);

          sw = GTK_SCROLLED_WINDOW (gtk_scrolled_window_new (NULL, NULL));
          gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw),
                                          GTK_POLICY_AUTOMATIC,
                                          GTK_POLICY_AUTOMATIC);

          gtk_container_add (GTK_CONTAINER (sw), GTK_WIDGET (view));

          gtk_container_add (GTK_CONTAINER (window), GTK_WIDGET (sw));

          gtk_widget_show_all (GTK_WIDGET (sw));

          if (!GTK_WIDGET_VISIBLE (window))
              gtk_widget_show (window);

          TheLibrary = window;

      }
    else
      {

          fprintf (stderr, "Calling present on the library window");
          gtk_window_present (GTK_WINDOW (TheLibrary));

      }
}

static UI *
ui_create ()
{
    static GtkWidget *window = NULL;
    GtkWidget *vbox;
    GtkWidget *view;
    GtkWidget *sw;
    // GtkWidget *buttons, *minibuffer, *frame;
    // GtkWidget *back_button, *quit_button, *next_button, *home_button, *reload_button, *library_button;
    // GtkTextTagTable *tags;
    // GtkObject *adjustment;
    // GtkWidget *hscale;
    UI *ui;

    initialize_icons ();

    ui = (UI *) malloc (sizeof (UI));

    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_default_size (GTK_WINDOW (window), 240, 300);

    //g_signal_connect (window, "destroy",
    //                  G_CALLBACK (gtk_widget_destroyed), &window);
    g_signal_connect (window, "destroy",
                      G_CALLBACK (quit_callback), &window);

    gtk_widget_set_name (window, "PluckerView");
    gtk_container_set_border_width (GTK_CONTAINER (window), 0);

    /* the top bar is for buttons; the bottom is the view of the document */
    vbox = gtk_vbox_new (FALSE, 2);
    //gtk_container_set_border_width (GTK_CONTAINER (vpaned), 5);
    gtk_container_add (GTK_CONTAINER (window), vbox);

/************************************************/
#if 0
    /* buttons = gtk_hbutton_box_new(); */
    buttons = gtk_hbox_new (FALSE, 4);

    back_button = create_image_button ("plucker-left-icon", &back_callback, ui);
    gtk_box_pack_start (GTK_BOX (buttons), back_button, FALSE, TRUE, 0);
    /* gtk_container_add (GTK_CONTAINER (buttons), back_button); */

    home_button = create_image_button ("plucker-home-icon", &home_callback, ui);
    gtk_box_pack_start (GTK_BOX (buttons), home_button, FALSE, TRUE, 0);

    next_button = create_image_button ("plucker-right-icon", &next_callback, ui);
    gtk_box_pack_start (GTK_BOX (buttons), next_button, FALSE, TRUE, 0);

    library_button = create_image_button ("plucker-library-icon", &library_callback, ui);
    gtk_box_pack_start (GTK_BOX (buttons), library_button, FALSE, TRUE, 10);

/*
   minibuffer = gtk_entry_new ();
   frame = gtk_frame_new(NULL);
   gtk_frame_set_shadow_type (GTK_FRAME(frame), GTK_SHADOW_ETCHED_IN);
   gtk_container_add (GTK_CONTAINER(frame), minibuffer);
   gtk_box_pack_start (GTK_BOX(buttons), frame, TRUE, TRUE, 0);
 */
/*
   adjustment = gtk_adjustment_new (1.0, 0.0, 4.0, 0.1, 1.0, 0.0);
   gtk_signal_connect (GTK_OBJECT(adjustment), "value-changed",
   GTK_SIGNAL_FUNC(scale_callback), ui);
   hscale = gtk_hscale_new(GTK_ADJUSTMENT(adjustment));
   gtk_container_add (GTK_CONTAINER (buttons), hscale);

   reload_button = gtk_button_new_with_label("Reload");
   gtk_signal_connect (GTK_OBJECT(reload_button), "clicked",
   GTK_SIGNAL_FUNC(reload_callback), ui);
   gtk_container_add (GTK_CONTAINER (buttons), reload_button);
 */

    quit_button = gtk_button_new_with_label ("Quit");
    g_signal_connect (GTK_OBJECT (quit_button), "clicked",
                        GTK_SIGNAL_FUNC (quit_callback), ui);
    gtk_box_pack_end (GTK_BOX (buttons), quit_button, FALSE, TRUE, 0);

    //gtk_paned_add1 (GTK_PANED (vbox), buttons);
    gtk_box_pack_start (GTK_BOX (vbox), buttons, FALSE, FALSE, 0);

/*************************************/
#endif

    {
      GtkWidget *toolbar, *toolbar_icon;

      toolbar = gtk_toolbar_new ();
      gtk_toolbar_set_orientation (GTK_TOOLBAR (toolbar), GTK_ORIENTATION_HORIZONTAL);
      gtk_toolbar_set_style (GTK_TOOLBAR (toolbar), GTK_TOOLBAR_ICONS);

      toolbar_icon = gtk_image_new_from_stock (GTK_STOCK_GO_BACK, GTK_ICON_SIZE_SMALL_TOOLBAR);
      gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Prev"), 
           _("Previous Chapter"), _("Previous Chapter"), toolbar_icon, back_callback, ui);

      toolbar_icon = gtk_image_new_from_stock (GTK_STOCK_HOME, GTK_ICON_SIZE_SMALL_TOOLBAR);
      gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Index"), 
           _("Goto Index"), _("Goto Index"), toolbar_icon, home_callback, ui);

      toolbar_icon = gtk_image_new_from_stock (GTK_STOCK_GO_FORWARD, GTK_ICON_SIZE_SMALL_TOOLBAR);
      gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Next"), 
           _("Next Chapter"), _("Next Chapter"), toolbar_icon, next_callback, ui);

      toolbar_icon = gtk_image_new_from_stock (GTK_STOCK_OPEN, GTK_ICON_SIZE_SMALL_TOOLBAR);
      gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Library"), 
           _("Library"), _("Library"), toolbar_icon, library_callback, ui);

      toolbar_icon = gtk_image_new_from_stock (GTK_STOCK_QUIT, GTK_ICON_SIZE_SMALL_TOOLBAR);
      gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Quit"), 
           _("Quit Plucker"), _("Quit Plucker"), toolbar_icon, quit_callback, ui);

      gtk_box_pack_start (GTK_BOX (vbox), toolbar, FALSE, FALSE, 0);
    }

    ui->scale = TheScale;
    view = gtk_text_view_new ();

    sw = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw),
                                    GTK_POLICY_AUTOMATIC,
                                    GTK_POLICY_AUTOMATIC);
    //gtk_paned_add2 (GTK_PANED (vbox), sw);
    gtk_box_pack_start (GTK_BOX (vbox), sw, TRUE, TRUE, 0);

    gtk_container_add (GTK_CONTAINER (sw), view);

    gtk_widget_show_all (vbox);

    if (!GTK_WIDGET_VISIBLE (window))
        gtk_widget_show (window);

    ui->window = GTK_WINDOW (window);
    ui->textview = GTK_TEXT_VIEW (view);
    ui->current_node = NULL;
    ui->history = NULL;

    g_signal_connect (G_OBJECT (view), "button-press-event",
                      G_CALLBACK (mouse_click_callback), ui);
    g_signal_connect (G_OBJECT (view), "button-release-event",
                      G_CALLBACK (mouse_click_callback), ui);

    return ui;
}

static void
initialize_charset_names ()
{
    /* The names here are from the Linux I18N work; see
       http://www.li18nux.org/docs/html/CodesetAliasTable-V10.html */

    CharsetNames = g_tree_new (compare_pointer_ints);
    g_tree_insert (CharsetNames, GINT_TO_POINTER (4), "ISO-8859-1");
    g_tree_insert (CharsetNames, GINT_TO_POINTER (106), "UTF-8");
    g_tree_insert (CharsetNames, GINT_TO_POINTER (3), "ISO-646-US");    /* ASCII */
}


static void
initialize_properties ()
{
    if (Verbose)
        plkr_ShowMessages (1);

    if (ScreenDPI == 0)
      {
          ScreenDPI = gdk_screen_width () / (gdk_screen_width_mm () / 25.4);
      }
    fprintf (stderr, "dpi is %d\n", ScreenDPI);
}


int 
main (ac, av, envp)

     int ac;
     char **av;
     char **envp;

{
    int i,pdb;
    struct stat buf;
    char *document_path=NULL,*pdb_check;
    // gboolean status;
    char *usage_format = "Usage:  %s [--unicode] [--dpi=N] [--verbose] [--scale=SCALE] DOCUMENT-FILE\n";


    TheScale = 1.0;
    UseUnicode = FALSE;

    //gtk_init (&ac, &av);
    if (gpe_application_init (&ac, &av) == FALSE)
      exit (1);
    if (gpe_load_icons (my_icons) == FALSE)
      exit (1);

    for (i = 1; i < ac; i++)
      {
          if (strncmp (av[i], "--scale=", 8) == 0)
            {
                TheScale = atof (av[i] + 8);
            }
          else if (strncmp (av[i], "--unicode", 12) == 0)
            {
                UseUnicode = TRUE;
            }
          else if (strncmp (av[i], "--verbose", 9) == 0)
            {
                Verbose = TRUE;
            }
          else if (strncmp (av[i], "--dpi=", 6) == 0)
            {
                ScreenDPI = atoi (av[i] + 6);
            }
          else if (strncmp (av[i], "--", 2) == 0)
            {
                fprintf (stderr, usage_format, av[0]);
                return 1;
            }
          else
            {
                break;
            }
      }
    if ((ac - i) != 1)
      {
      	GtkWidget *fs;
      	gint result;
      	
      	fs = gtk_file_selection_new("Open file");
      	result = gtk_dialog_run (GTK_DIALOG(fs));
      	switch (result) {
      		case GTK_RESPONSE_OK:
		      	result = strlen (gtk_file_selection_get_filename (GTK_FILE_SELECTION(fs)));
		      	document_path = (char *)malloc(result * sizeof(char));
		      	strcpy (document_path, gtk_file_selection_get_filename (GTK_FILE_SELECTION(fs)));
		      	gtk_widget_destroy (fs);
      			g_print("accepted\n");
      			break;
      		case GTK_RESPONSE_CANCEL:
      			gtk_widget_destroy (fs);
      			exit(1);
      			break;
      		default:
      			g_print("res=%d\n",result);
      			break;
      	}

//          fprintf (stderr, usage_format, av[0]);
//          return 1;
	
      } else
          document_path = av[i];

    pdb = strlen(document_path);
    if (document_path != NULL)
	{
	    pdb_check = document_path;
    	    pdb_check = pdb_check + (sizeof(char)*(pdb -3));
            if(strcmp(pdb_check, "pdb"))
		{
		 gpe_error_box("This file is not a pdb file! Exiting!");
		 return 1;
		}
	}
    

    if (stat (document_path, &buf) != 0)
      {
          fprintf (stderr, "Can't access document %s\n", document_path);
          return 1;
      }
    else if (!S_ISREG (buf.st_mode))
      {
          fprintf (stderr, "Document file %s must be a regular file.\n", document_path);
          return 1;
      }

    initialize_charset_names ();
    initialize_properties ();

    MainUi = ui_create ();

    if (document_path)
        ui_show_document (MainUi, document_path);

    if (TheMainloop == NULL)
        TheMainloop = g_main_loop_new (NULL, TRUE);
    g_main_loop_run (TheMainloop);

    return 0;
}
