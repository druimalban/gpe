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

void image_convolve( GdkPixbuf* pixbuf, int* mask, int mask_size, int mask_divisor ) {

    int x, y, k, l, b, rowstride, width, height, channels, padding, new_value;
    int* temp_pixel;
    guchar *temp_image, *image;

    rowstride = gdk_pixbuf_get_rowstride( GDK_PIXBUF( pixbuf ) );
    channels = gdk_pixbuf_get_n_channels( GDK_PIXBUF( pixbuf ) );
    width = gdk_pixbuf_get_width( GDK_PIXBUF( pixbuf ) );
    height = gdk_pixbuf_get_height( GDK_PIXBUF( pixbuf ) );

//    fprintf( stderr, "Rowstride: %d, width: %d, height: %d, channels: %d\n", rowstride, width, height, channels );

    
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

void blur( GdkPixbuf* pixbuf ) {

    int mask[] = { 1, 1, 1,
                   1, 1, 1, 
                   1, 1, 1 };

    image_convolve( pixbuf, mask, 3, 9 );

}

void sharpen( GdkPixbuf* pixbuf ) {

    int mask[] = {-1, -1, -1,
                  -1, 9, -1, 
                  -1, -1, -1 };

    image_convolve( pixbuf, mask, 3, 1 );

}

GdkPixbuf* image_rotate( GdkPixbuf* pixbuf, int degrees ) {

    int x, y, new_x, new_y, width, height, new_width, new_height, center_x,
    center_y, new_center_x, new_center_y, rowstride, new_rowstride, channels;
    int x0, y0, x1, y1, x2, y2, x3, y3, new_x0, new_y0, new_x1, new_y1, new_x2,
    new_y2, new_x3, new_y3, b;
    float cos_value, sin_value;
    int temp_x, temp_y;
    GdkPixbuf* return_pixbuf;
    guchar *image, *return_image;

    int debug;


    // make sure the degrees are normalized
    degrees = degrees % 360;
    
    cos_value = cos( degrees * M_PI / 180.0 );
    sin_value = sin( degrees * M_PI / 180.0 );

    rowstride = gdk_pixbuf_get_rowstride( GDK_PIXBUF( pixbuf ) );
    channels = gdk_pixbuf_get_n_channels( GDK_PIXBUF( pixbuf ) );
    width = gdk_pixbuf_get_width( GDK_PIXBUF( pixbuf ) );
    height = gdk_pixbuf_get_height( GDK_PIXBUF( pixbuf ) );

    // calculate the 4 corners of the image
    center_x = width / 2;
    center_y = height / 2;
    x0 = -center_x;
    y0 = - center_y;
    x1 = width - center_x;
    y1 = - center_y;
    x2 = width - center_x;
    y2 = height - center_y;
    x3 = -center_x;
    y3 = height - center_y;

    // calculated the rotated corners of the image
    new_x0 = x0 * cos_value - y0 * sin_value;  
    new_y0 = x0 * sin_value + y0 * cos_value;  
    new_x1 = x1 * cos_value - y1 * sin_value;  
    new_y1 = x1 * sin_value + y1 * cos_value;  
    new_x2 = x2 * cos_value - y2 * sin_value;  
    new_y2 = x2 * sin_value + y2 * cos_value;  
    new_x3 = x3 * cos_value - y3 * sin_value;  
    new_y3 = x3 * sin_value + y3 * cos_value;  

    // create the new image
    if( degrees > 270 ) { 

        new_width = new_x2 - new_x0;
        new_height = new_y3 - new_y1;

    }
    else if( degrees == 270 ) {

        new_width = height;
        new_height = width;

    }
    else if( degrees > 180 ) {

        new_width = new_x3 - new_x1;
        new_height = new_y0 - new_y2;

    }
    else if( degrees == 180 ) {

        new_width = width;
        new_height = height;

    }
    else if( degrees > 90 ) {

        new_width = new_x0 - new_x2;
        new_height = new_y1 - new_y3;

    }
    else if( degrees == 90 ) {

        new_width = height;
        new_height = width;

    }
    else if( degrees > 0 ) {

        new_width = new_x1 - new_x3;
        new_height = new_y2 - new_y0;

    }
    else {

        new_width = width;
        new_height = height;

    }
   
    if( degrees % 90 ) {

        new_width += 2;
        new_height += 2;

    }
    else 
        new_width++;
    return_pixbuf = gdk_pixbuf_new( gdk_pixbuf_get_colorspace( GDK_PIXBUF( pixbuf ) ),
                                    gdk_pixbuf_get_has_alpha( GDK_PIXBUF( pixbuf ) ),
                                    gdk_pixbuf_get_bits_per_sample( GDK_PIXBUF(
                                    pixbuf ) ),
                                    new_width,
                                    new_height );

    new_rowstride = gdk_pixbuf_get_rowstride( GDK_PIXBUF( return_pixbuf ) );

    gdk_pixbuf_fill( GDK_PIXBUF( return_pixbuf ), 0 );

    new_center_x = new_width / 2;
    new_center_y = new_height / 2;
   
    image = gdk_pixbuf_get_pixels( GDK_PIXBUF( pixbuf ) );
    return_image = gdk_pixbuf_get_pixels( GDK_PIXBUF( return_pixbuf ) );
    for( y = 0; y < height; y++ ) {

        for( x = 0; x < width; x++ ) {

            temp_x = x - center_x;
            temp_y = y - center_y;

            new_x = temp_x * cos_value - temp_y * sin_value + new_center_x;  
            new_y = temp_x * sin_value + temp_y * cos_value + new_center_y;  

	    assert (new_x < new_width);
	    assert (new_y < new_height);

            for( b = 0; b < channels; b++ ) {

                return_image[ new_y * new_rowstride + new_x * channels +  b ] =
                image[ y * rowstride + x * channels +  b ]; 
                return_image[ new_y * new_rowstride + ( new_x + 1 ) * channels +  b
                ] = image[ y * rowstride + x * channels +  b ]; 

            }

        }

    } 

    return return_pixbuf; 

}
