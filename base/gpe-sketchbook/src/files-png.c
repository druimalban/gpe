/* gpe-sketchbook -- a sketches notebook program for PDA
 * Copyright (C) 2002 Luc Pionchon
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <png.h>

#include "files.h"
#include "files-png.h"

#include "gpe-sketchbook.h"
#include "sketchpad.h"

//--------------------------------------------------------------------
// Adjustments to use GTK+ 2.0 png save function (gdkpixbuf/io-png.c)
//
#define png_simple_error_callback   NULL //NOTE: may use gpe_error_box
#define png_simple_warning_callback NULL //NOTE: may use gpe_warning_box
#define g_set_error(e, GDK_PIXBUF_ERROR, GDK_PIXBUF_ERROR_BAD_OPTION, text) {}
#define g_convert(value, a, b, c, d, e, f) value 
#define GError gpointer

static gboolean
gdk_pixbuf__png_image_save (FILE          *f, 
                            GdkPixbuf     *pixbuf, 
                            gchar        **keys,
                            gchar        **values,
                            GError       **error);
//----------------------------------------------------------------------------

gint file_save_png(const gchar * fullpath_filename){
    FILE * fp;

    GdkColormap * colormap;
    GdkPixbuf   * pixbuf;

    gboolean success;

    fp = fopen(fullpath_filename, "wb");
    if (!fp) return 0;

    //--retrieve image data
    colormap = gdk_colormap_get_system();
    pixbuf = gdk_pixbuf_get_from_drawable(NULL,//GdkPixbuf *dest,
                                          drawing_area_pixmap_buffer,//GdkDrawable *src,
                                          colormap,//GdkColormap *cmap,
                                          0,//int src_x,
                                          0,//int src_y,
                                          0,//int dest_x,
                                          0,//int dest_y,
                                          drawing_area_width,//int width,
                                          drawing_area_height);//int height);

    success = gdk_pixbuf__png_image_save (fp, pixbuf, NULL, NULL, NULL);
    if(!success){
      return 0;
    }

    fclose(fp);
    gdk_pixbuf_unref(pixbuf);

    return 1;
}//file_save_png()

gint file_load_png(const gchar * fullpath_filename){
  //FIXME: drawing_area/drawing_area_pixmap_buffer should be a parameter
  GdkGC * gc;
  GdkPixbuf * pixbuf = NULL;
  gint width, height;

  gc = gdk_gc_new(drawing_area->window);

#ifdef GTK2
  pixbuf = gdk_pixbuf_new_from_file(fullpath_filename, NULL); //GError **error
#else
  pixbuf = gdk_pixbuf_new_from_file(fullpath_filename);
#endif
  if(pixbuf == NULL){
    return 0;
  }
  width  = gdk_pixbuf_get_width (pixbuf);
  height = gdk_pixbuf_get_height(pixbuf);
  gdk_pixbuf_render_to_drawable(pixbuf,//GdkPixbuf *pixbuf,
                                drawing_area_pixmap_buffer,//GdkDrawable *drawable,
                                gc,//GdkGC *gc,
                                0,//int src_x,
                                0,//int src_y,
                                0,//int dest_x,
                                0,//int dest_y,
                                width,//int width,
                                height,//int height,
                                GDK_RGB_DITHER_NONE,//GdkRgbDither dither,
                                0,//int x_dither,
                                0);//int y_dither);

  sketchpad_refresh_drawing_area(drawing_area);
  gdk_pixbuf_unref(pixbuf);

  return 1; 
}//file_load_png()


#define _(x) (x) //do not translate gdk-pixbuf error messages
//-----------------------------------------------------------
// The following is borrowed to GTK+ 2.0 (gdkpixbuf/io-png.c)
// to be removed when using GTK+ 2.0
// Copyright (C) 1999 Mark Crichton
// Copyright (C) 1999 The Free Software Foundation
// Authors: Mark Crichton <crichton@gimp.org>
//          Federico Mena-Quintero <federico@gimp.org>
//
static gboolean
gdk_pixbuf__png_image_save (FILE          *f, 
                            GdkPixbuf     *pixbuf, 
                            gchar        **keys,
                            gchar        **values,
                            GError       **error)
{
       png_structp png_ptr;
       png_infop info_ptr;
       png_textp text_ptr = NULL;
       guchar *ptr;
       guchar *pixels;
       int y;
       int i;
       png_bytep row_ptr;
       png_color_8 sig_bit;
       int w, h, rowstride;
       int has_alpha;
       int bpc;
       int num_keys;
       gboolean success = TRUE;

       num_keys = 0;

       if (keys && *keys) {
               gchar **kiter;
               gchar  *key;
               int     len;

               for (kiter = keys; *kiter; kiter++) {
                       if (strncmp (*kiter, "tEXt::", 6) != 0) {
                                g_warning ("Bad option name '%s' passed to PNG saver", *kiter);
                                return FALSE;
                       }
                       key = *kiter + 6;
                       len = strlen (key);
                       if (len <= 1 || len > 79) {
                               g_set_error (error,
                                            GDK_PIXBUF_ERROR,
                                            GDK_PIXBUF_ERROR_BAD_OPTION,
                                            _("Keys for PNG tEXt chunks must have at least 1 and at most 79 characters."));
                               return FALSE;
                       }
                       for (i = 0; i < len; i++) {
                               if ((guchar) key[i] > 127) {
                                       g_set_error (error,
                                                    GDK_PIXBUF_ERROR,
                                                    GDK_PIXBUF_ERROR_BAD_OPTION,
                                                    _("Keys for PNG tEXt chunks must be ASCII characters."));
                                       return FALSE;
                               }
                       }
                       num_keys++;
               }
       }

       if (num_keys > 0) {
               text_ptr = g_new0 (png_text, num_keys);
               for (i = 0; i < num_keys; i++) {
                       text_ptr[i].compression = PNG_TEXT_COMPRESSION_NONE;
                       text_ptr[i].key  = keys[i] + 6;
                       text_ptr[i].text = g_convert (values[i], -1, 
                                                     "ISO-8859-1", "UTF-8", 
                                                     NULL, &text_ptr[i].text_length, 
                                                     NULL);
                       if (!text_ptr[i].text) {
                               g_set_error (error,
                                            GDK_PIXBUF_ERROR,
                                            GDK_PIXBUF_ERROR_BAD_OPTION,
                                            _("Value for PNG tEXt chunk can not be converted to ISO-8859-1 encoding."));
                               num_keys = i;
                               for (i = 0; i < num_keys; i++)
                                       g_free (text_ptr[i].text);
                               g_free (text_ptr);
                               return FALSE;
                       }
               }
       }

       bpc = gdk_pixbuf_get_bits_per_sample (pixbuf);
       w = gdk_pixbuf_get_width (pixbuf);
       h = gdk_pixbuf_get_height (pixbuf);
       rowstride = gdk_pixbuf_get_rowstride (pixbuf);
       has_alpha = gdk_pixbuf_get_has_alpha (pixbuf);
       pixels = gdk_pixbuf_get_pixels (pixbuf);

       png_ptr = png_create_write_struct (PNG_LIBPNG_VER_STRING,
                                          error,
                                          png_simple_error_callback,
                                          png_simple_warning_callback);

       g_return_val_if_fail (png_ptr != NULL, FALSE);

       info_ptr = png_create_info_struct (png_ptr);
       if (info_ptr == NULL) {
	       success = FALSE;
	       goto cleanup;
       }
       if (setjmp (png_ptr->jmpbuf)) {
	       success = FALSE;
	       goto cleanup;
       }

       if (num_keys > 0) {
               png_set_text (png_ptr, info_ptr, text_ptr, num_keys);
       }

       png_init_io (png_ptr, f);

       if (has_alpha) {
               png_set_IHDR (png_ptr, info_ptr, w, h, bpc,
                             PNG_COLOR_TYPE_RGB_ALPHA, PNG_INTERLACE_NONE,
                             PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
       } else {
               png_set_IHDR (png_ptr, info_ptr, w, h, bpc,
                             PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
                             PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
       }
       sig_bit.red = bpc;
       sig_bit.green = bpc;
       sig_bit.blue = bpc;
       sig_bit.alpha = bpc;
       png_set_sBIT (png_ptr, info_ptr, &sig_bit);
       png_write_info (png_ptr, info_ptr);
       png_set_shift (png_ptr, &sig_bit);
       png_set_packing (png_ptr);

       ptr = pixels;
       for (y = 0; y < h; y++) {
               row_ptr = (png_bytep)ptr;
               png_write_rows (png_ptr, &row_ptr, 1);
               ptr += rowstride;
       }

       png_write_end (png_ptr, info_ptr);

cleanup:
       png_destroy_write_struct (&png_ptr, (png_infopp) NULL);

       if (num_keys > 0) {
               for (i = 0; i < num_keys; i++)
                       g_free (text_ptr[i].text);
               g_free (text_ptr);
       }

       return success;
}
