/*
 * Copyright (C) 2001, 2002 Petter Knudsen (PaxAnima) <paxanima@handhelds.org>
 * Part of GPE Gallery by Damien Tanner <dctanner@magenet.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <stdlib.h>
#include <math.h>
#include <gtk/gtk.h>
#include <assert.h>
#include <string.h>

static void image_convolve( GdkPixbuf* pixbuf, int* mask, int mask_size, int mask_divisor ) {

    int x, y, k, l, b, rowstride, width, height, channels, padding, new_value;
    int* temp_pixel;
    guchar *temp_image, *image;

    rowstride = gdk_pixbuf_get_rowstride( GDK_PIXBUF( pixbuf ) );
    channels = gdk_pixbuf_get_n_channels( GDK_PIXBUF( pixbuf ) );
    width = gdk_pixbuf_get_width( GDK_PIXBUF( pixbuf ) );
    height = gdk_pixbuf_get_height( GDK_PIXBUF( pixbuf ) );

    padding = ( mask_size - 1 ) / 2; 

    image = gdk_pixbuf_get_pixels( GDK_PIXBUF( pixbuf ) );
    temp_image = (guchar*) malloc( width * height * channels * sizeof( guchar ) );
    memcpy( temp_image, image, width * height * channels * sizeof( guchar ) ); 
    temp_pixel =(int*)  malloc( channels * sizeof( int ) );
    for( y = padding; y < height - padding; y++ ) {

        for( x = padding; x < width - padding; x++ ) {
        
            for( b = 0; b < channels; b++ ) 
                temp_pixel[b] = 0;

            for( l = 0; l < mask_size; l++ ) {

                for( k = 0; k < mask_size; k++ ) {

                    for( b = 0; b < channels; b++ ) 
                        temp_pixel[b] += temp_image[ ( y + l - padding ) * rowstride
                        + ( x + k - padding ) * channels + b ] * mask[ k + l *
                        mask_size ];

                }

            }

            for( b = 0; b < channels; b++ ) {

                new_value = temp_pixel[b] / mask_divisor; 
                image[ y * rowstride + x * channels +  b ] = ( new_value > 255 ? 255
                : new_value < 0 ? 0 : new_value ); 

            }

        }

    }


    free( temp_image );
    free( temp_pixel );

}

void image_tools_blur( GdkPixbuf* pixbuf ) {

    int mask[] = { 1, 1, 1,
                   1, 1, 1, 
                   1, 1, 1 };

    image_convolve( pixbuf, mask, 3, 9 );

}

void image_tools_sharpen( GdkPixbuf* pixbuf ) {

    int mask[] = {-1, -1, -1,
                  -1, 9, -1, 
                  -1, -1, -1 };

    image_convolve( pixbuf, mask, 3, 1 );

}

GdkPixbuf* image_tools_rotate( GdkPixbuf* pixbuf, gint rotation)
{
  int x, y, b, width, height, new_width, new_height, rowstride, new_rowstride, channels;
  GdkPixbuf* return_pixbuf;
  guchar *image, *return_image;
	
  rowstride = gdk_pixbuf_get_rowstride (GDK_PIXBUF (pixbuf));
  channels = gdk_pixbuf_get_n_channels (GDK_PIXBUF (pixbuf));

  width = gdk_pixbuf_get_width (GDK_PIXBUF (pixbuf));
  height = gdk_pixbuf_get_height (GDK_PIXBUF (pixbuf));
  if (rotation % 2)
    {
      new_width = gdk_pixbuf_get_height (GDK_PIXBUF (pixbuf));
      new_height = gdk_pixbuf_get_width (GDK_PIXBUF (pixbuf));
    }
  else
    {
      new_height = height;
      new_width = width;
    }
  

  return_pixbuf = gdk_pixbuf_new (gdk_pixbuf_get_colorspace (GDK_PIXBUF (pixbuf)),
				  gdk_pixbuf_get_has_alpha (GDK_PIXBUF (pixbuf)),
				  gdk_pixbuf_get_bits_per_sample (GDK_PIXBUF (pixbuf)),
				  new_width,
				  new_height);

  new_rowstride = gdk_pixbuf_get_rowstride (GDK_PIXBUF (return_pixbuf));

  gdk_pixbuf_fill (GDK_PIXBUF (return_pixbuf), 0);
   
  image = gdk_pixbuf_get_pixels (GDK_PIXBUF (pixbuf));
  return_image = gdk_pixbuf_get_pixels (GDK_PIXBUF (return_pixbuf));

  for (y = 0; y < height; y++)
  {
    for(x = 0; x < width; x++)
    {
      for( b = 0; b < channels; b++)
      {
         switch (rotation)
           {
             case 0:
               return_image[y * new_rowstride + x * channels + b] = 
		         image[x * channels + y * rowstride + b];
             break;
             case 3:             
               return_image[x * new_rowstride + y * channels + b] = 
		         image[y * rowstride + (width - x - 1) * channels + b];
             break;
             case 2:
                return_image[y * new_rowstride + x * channels + b] = 
		         image[(width - x - 1) * channels + (height - y - 1) * rowstride + b];
            break;
             case 1:
               return_image[x * new_rowstride + y * channels + b] = 
		         image[(height - y - 1) * rowstride + x * channels + b];
             break;
           }
      }
    }
  }

  return return_pixbuf;
}
