#include "mbpixbuf.h"

/* libmb
 * Copyright (C) 2002 Matthew Allum
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */


/* 

   TODO:
   o better composite - check for alpha
   o SHM ext
   o speedups where possible - do last, inline. gcc flags
   
NOTES:

_NET_WM_ICON CARDINAL[][2+n]/32

This is an array of possible icons for the client. This specification
does not stipulate what size these icons should be, but individual
desktop environments or toolkits may do so. The Window Manager MAY
scale any of these icons to an appropriate size.

This is an array of 32bit packed CARDINAL ARGB with high byte being A,
low byte being B. The first two cardinals are width, height. Data is
in rows, left to right and top to bottom.

--

Static stripped down mbpixbuf in matchbox ?

--

 */

static unsigned char* 
_load_png_file( const char *file, 
		int *width, int *height, int *has_alpha );
static int
_paletteAlloc(MBPixbuf *pb);


#ifdef USE_PNG

static unsigned char* 
_load_png_file( const char *file, 
	       int *width, int *height, int *has_alpha ) {
  FILE *fd;
  unsigned char *data;
  unsigned char header[8];
  int  bit_depth, color_type;

  png_uint_32  png_width, png_height, i, rowbytes;
  png_structp png_ptr;
  png_infop info_ptr;
  png_bytep *row_pointers;

  if ((fd = fopen( file, "rb" )) == NULL) return NULL;

  fread( header, 1, 8, fd );
  if ( ! png_check_sig( header, 8 ) ) 
    {
      fclose(fd);
      return NULL;
    }

  png_ptr = png_create_read_struct( PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if ( ! png_ptr ) {
    fclose(fd);
    return NULL;
  }

  info_ptr = png_create_info_struct(png_ptr);
  if ( ! info_ptr ) {
    png_destroy_read_struct( &png_ptr, (png_infopp)NULL, (png_infopp)NULL);
    fclose(fd);
    return NULL;
  }

  if ( setjmp( png_ptr->jmpbuf ) ) {
    png_destroy_read_struct( &png_ptr, &info_ptr, NULL);
    fclose(fd);
    return NULL;
  }

  png_init_io( png_ptr, fd );
  png_set_sig_bytes( png_ptr, 8);
  png_read_info( png_ptr, info_ptr);
  png_get_IHDR( png_ptr, info_ptr, &png_width, &png_height, &bit_depth, 
		&color_type, NULL, NULL, NULL);
  *width = (int) png_width;
  *height = (int) png_height;

  if (( color_type == PNG_COLOR_TYPE_PALETTE )||
      ( png_get_valid( png_ptr, info_ptr, PNG_INFO_tRNS )))
    png_set_expand(png_ptr);

  if (( color_type == PNG_COLOR_TYPE_GRAY )||
      ( color_type == PNG_COLOR_TYPE_GRAY_ALPHA ))
    png_set_gray_to_rgb(png_ptr);
 
  if ( info_ptr->color_type == PNG_COLOR_TYPE_RGB_ALPHA 
       || info_ptr->color_type == PNG_COLOR_TYPE_GRAY_ALPHA
       )
    *has_alpha = 1;
  else
    *has_alpha = 0;

  /* 8 bits */
  if ( bit_depth == 16 )
    png_set_strip_16(png_ptr);

  if (bit_depth < 8)
    png_set_packing(png_ptr);


  png_set_filler(png_ptr, 0xff, PNG_FILLER_AFTER);


  png_read_update_info( png_ptr, info_ptr);

  /* allocate space for data and row pointers */
  rowbytes = png_get_rowbytes( png_ptr, info_ptr);
  data = (unsigned char *) malloc( rowbytes*(*height) );
  row_pointers = (png_bytep *) malloc( (*height)*sizeof(png_bytep));

  if (( data == NULL )||( row_pointers == NULL )) {
    png_destroy_read_struct( &png_ptr, &info_ptr, NULL);
    free(data);
    free(row_pointers);
    return NULL;
  }


  for ( i = 0;  i < *height; i++ )
    row_pointers[i] = data + i*rowbytes;


  png_read_image( png_ptr, row_pointers );
  png_read_end( png_ptr, NULL);


  free(row_pointers);
  png_destroy_read_struct( &png_ptr, &info_ptr, NULL);
  fclose(fd);
  /* return an array of RGB(A) values */
  return data;
}

#endif

static unsigned char* 
_load_xpm_file( MBPixbuf *pb, const char *filename, int *w, int *h, int *has_alpha)
{ 				/* This hell is adapted from imlib ;-) */
  FILE *file;
  
  struct _cmap
  {
    unsigned char       str[6];
    unsigned char       transp;
    short                 r, g, b;
  } *cmap = NULL;
  
  unsigned char *data = NULL, *ptr = NULL, *end = NULL;

  int pc, c = ' ', i = 0, j = 0, k = 0, ncolors = 0, cpp = 0, 
    comment = 0, transp = 0, quote = 0, context = 0,  len, done = 0;

  char *line, s[256], tok[128], col[256];

  XColor xcol;
  int lsz = 256;
  
  short lookup[128 - 32][128 - 32];
  
  if (!filename) return NULL;
  
  if ((file = fopen( filename, "rb" )) == NULL) return NULL;
  
  line = malloc(lsz);
  
  while (!done)
    {
      pc = c;
      c = fgetc(file);
      if (c == EOF)
	break;
      if (!quote)
	{
	  if ((pc == '/') && (c == '*'))
	    comment = 1;
	  else if ((pc == '*') && (c == '/') && (comment))
	    comment = 0;
	}
      if (!comment)
	{
	  if ((!quote) && (c == '"'))
	    {
	      quote = 1;
	      i = 0;
	    }
	  else if ((quote) && (c == '"'))
	    {
	      line[i] = 0;
	      quote = 0;
	      if (context == 0)
		{
		  /* Header */
		  sscanf(line, "%i %i %i %i", w, h, &ncolors, &cpp);
                  if (ncolors > 32766 || cpp > 5 || *w > 32767 || *h > 32767)
		    {
		      fprintf(stderr, "xpm file invalid");
		      free(line);
		      fclose(file);
		      return NULL;
		    }

		  cmap = malloc(sizeof(struct _cmap) * ncolors);

		  if (!cmap) 		      
		    {
		      free(line);
		      fclose(file);
		      return NULL;
		    }

		  data = malloc(*w ** h * 4);
		  if (!data)
		    {
		      free(cmap);
		      free(line);
		      fclose(file);
		      return NULL;
		    }

		  ptr = data;
		  end = ptr + (*w ** h * 4);
		  j = 0;
		  context++;
		}
	      else if (context == 1)
		{
		  /* Color Table */
		  if (j < ncolors)
		    {
		      int                 slen;
		      int                 hascolor, iscolor;

		      iscolor = 0;
		      hascolor = 0;
		      tok[0] = 0;
		      col[0] = 0;
		      s[0] = 0;
		      len = strlen(line);
		      strncpy(cmap[j].str, line, cpp);
		      cmap[j].str[cpp] = 0;
		      cmap[j].r = -1;
		      cmap[j].transp = 0;
		      for (k = cpp; k < len; k++)
			{
			  if (line[k] != ' ')
			    {
			      s[0] = 0;
			      sscanf(&line[k], "%256s", s);
			      slen = strlen(s);
			      k += slen;
			      if (!strcmp(s, "c"))
				iscolor = 1;
			      if ((!strcmp(s, "m")) || (!strcmp(s, "s")) ||
				  (!strcmp(s, "g4")) || (!strcmp(s, "g")) ||
				  (!strcmp(s, "c")) || (k >= len))
				{
				  if (k >= len)
				    {
				      if (col[0])
					strcat(col, " ");
                                      if (strlen(col) + strlen(s) < sizeof(col))
					strcat(col, s);
				    }
				  if (col[0])
				    {
				      if (!strcasecmp(col, "none"))
					{
					  transp = 1;
					  cmap[j].transp = 1;
					}
				      else
					{
					  if ((((cmap[j].r < 0) ||
						(!strcmp(tok, "c"))) &&
					       (!hascolor)))
					    {
					      XParseColor(pb->dpy,
							  DefaultColormap(pb->dpy, 
									  pb->scr),

							  col, &xcol);
					      cmap[j].r = xcol.red >> 8;
					      cmap[j].g = xcol.green >> 8;
					      cmap[j].b = xcol.blue >> 8;
					      if ((cmap[j].r == 255) &&
						  (cmap[j].g == 0) &&
						  (cmap[j].b == 255))
						cmap[j].r = 254;
					      if (iscolor)
						hascolor = 1;
					    }
					}
				    }
				  strcpy(tok, s);
				  col[0] = 0;
				}
			      else
				{
				  if (col[0])
				    strcat(col, " ");
				  strcat(col, s);
				}
			    }
			}
		    }
		  j++;
		  if (j >= ncolors)
		    {
		      if (cpp == 1)
			for (i = 0; i < ncolors; i++)
			  lookup[(int)cmap[i].str[0] - 32][0] = i;
		      if (cpp == 2)
			for (i = 0; i < ncolors; i++)
			  lookup[(int)cmap[i].str[0] - 32][(int)cmap[i].str[1] - 32] = i;
		      context++;
		    }
		}
	      else
		{
		  /* Image Data */
		  i = 0;
		  if (cpp == 1)
		    {
		      for (i = 0; 
			   ((i < 65536) && (ptr < end) && (line[i])); 
			   i++)
			{
			  col[0] = line[i];
			  if (transp && 
			      cmap[lookup[(int)col[0] - 32][0]].transp)
			    {
			      *ptr++ = 0; *ptr++ = 0; *ptr++ = 0; *ptr++ = 0;
			    }
			  else 
			    {
			      int idx = lookup[(int)col[0] - 32][0];
			      *ptr++ = (unsigned char)cmap[idx].r;
			      *ptr++ = (unsigned char)cmap[idx].g;
			      *ptr++ = (unsigned char)cmap[idx].b;
			      *ptr++ = 255;
			    }
			}
		    }
		  else
		    {
		      for (i = 0; 
			   ((i < 65536) && (ptr < end) && (line[i])); 
			   i++)
			{
			  for (j = 0; j < cpp; j++, i++)
			    {
			      col[j] = line[i];
			    }
			  col[j] = 0;
			  i--;
			  for (j = 0; j < ncolors; j++)
			    {
			      if (!strcmp(col, cmap[j].str))
				{
				  if (transp && cmap[j].transp)
				    {
				      *ptr++ = 0;
				      *ptr++ = 0;
				      *ptr++ = 0;
				      *ptr++ = 0;
				    }
				  else
				    {
				      *ptr++ = (unsigned char)cmap[j].r;
				      *ptr++ = (unsigned char)cmap[j].g;
				      *ptr++ = (unsigned char)cmap[j].b;
				      *ptr++ = 255;
				    }
				  j = ncolors;
				}
			    }
			}
		    }
		}
	    }
	}

      /* Scan in line from XPM file */
      if ((!comment) && (quote) && (c != '"'))
	{
	  if (c < 32)
	    c = 32;
	  else if (c > 127)
	    c = 127;
	  line[i++] = c;
	}
      if (i >= lsz)
	{
	  lsz += 256;
	  line = realloc(line, lsz);
	  if(line == NULL)
	    {
	      free(cmap);
	      fclose(file);
	      return NULL;
	    }
	}

      if ((ptr) && ((ptr - data) >= *w ** h * 4))
	done = 1;
    }

  if (transp)
    *has_alpha = 1;
  else
    *has_alpha = 0;

  free(cmap);
  free(line);
  fclose(file);
  return data;
}

static int
_paletteAlloc(MBPixbuf *pb)
{
  XColor              xcl;
  int                 colnum, i, j;
  unsigned long       used[256];
  int                 num_used, is_used;

  int num_of_cols = 1 << pb->depth;
  int colors_per_channel = num_of_cols / 3;

  if (pb->palette) free(pb->palette);

  pb->palette = malloc(sizeof(MBPixbufColor) * num_of_cols);

  num_used = 0;
  colnum = 0;

  switch(pb->vis->class)
    {
    case PseudoColor:
    case StaticColor:
      /*
      for (r = 0, i = 0; r < colors_per_channel; r++)
        for (g = 0; g < colors_per_channel; g++)
          for (b = 0; b < colors_per_channel; b++, i++) 
	    {      
	      xcl.red   = (r * 0xffff) / (colors_per_channel - 1);
	      xcl.green = (g * 0xffff) / (colors_per_channel - 1);
	      xcl.blue  = (b * 0xffff) / (colors_per_channel - 1);
	      xcl.flags = DoRed | DoGreen | DoBlue;

      */

      for (i = 0; i < num_of_cols; i++)
	{			/* RRRGGGBB - TODO check for 4 bit col */
	  int ii = (i * 256)/num_of_cols;
	  xcl.red = (unsigned short)( ( ii & 0xe0 ) << 8 );
	  xcl.green = (unsigned short)(( ii & 0x1c ) << 11 );
	  xcl.blue = (unsigned short)( ( ii & 0x03 ) << 14 );
	  xcl.flags = DoRed | DoGreen | DoBlue;
	  
	  if (!XAllocColor(pb->dpy, pb->root_cmap, &xcl))
	    {
	      //printf("alloc color failed\n");
	    }
	  is_used = 0;
	  for (j = 0; j < num_used; j++)
	    {
	      if (xcl.pixel == used[j])
		{
		  is_used = 1;
		  j = num_used;
		}
	    }
	  if (!is_used)
	    {
	      pb->palette[colnum].r = xcl.red >> 8;
	      pb->palette[colnum].g = xcl.green >> 8;
	      pb->palette[colnum].b = xcl.blue >> 8;
	      pb->palette[colnum].pixel = xcl.pixel;
	      used[num_used++] = xcl.pixel;
	      colnum++;
	    }
	  else
	    xcl.pixel = 0;
	}
      break;
    case GrayScale:
    case StaticGray:
      for(i = 0; i < num_of_cols; i++)
	{
	  xcl.red   = (i * 0xffff) / (colors_per_channel - 1);
	  xcl.green = (i * 0xffff) / (colors_per_channel - 1);
	  xcl.blue  = (i * 0xffff) / (colors_per_channel - 1);
	  xcl.flags = DoRed | DoGreen | DoBlue;
	  
	  if (!XAllocColor(pb->dpy, pb->root_cmap, &xcl))
	    {
	      printf("alloc color failed\n");
	    }
	  is_used = 0;
	  for (j = 0; j < num_used; j++)
	    {
	      if (xcl.pixel == used[j])
		{
		  is_used = 1;
		  j = num_used;
		}
	    }
	  if (!is_used)
	    {
	      pb->palette[colnum].r = xcl.red >> 8;
	      pb->palette[colnum].g = xcl.green >> 8;
	      pb->palette[colnum].b = xcl.blue >> 8;
	      pb->palette[colnum].pixel = xcl.pixel;
	      used[num_used++] = xcl.pixel;
	      colnum++;
	    }
	  else
	    xcl.pixel = 0;
	}
    }
  return colnum;
}

unsigned long
mb_pixbuf_get_pixel(MBPixbuf *pb, int *r, int *g, int *b)
{
  int                 i;
  int                 dif;
  int                 dr, dg, db;
  unsigned long       col;
  int                 mindif = 0x7fffffff;

  col = 0;

  if (pb->depth > 8)
    {
      dr = *r;
      dg = *g;
      db = *b;
      switch (pb->depth)
	{
	case 15:
	  *r = dr - (dr & 0xf8);
	  *g = dg - (dg & 0xf8);
	  *b = db - (db & 0xf8);
	  return ((dr & 0xf8) << 7) | ((dg & 0xf8) << 2) | ((db & 0xf8) >> 3);
	  break;
	case 16:
	  *r = dr - (dr & 0xf8);
	  *g = dg - (dg & 0xfc);
	  *b = db - (db & 0xf8);
	  return ((dr & 0xf8) << 8) | ((dg & 0xfc) << 3) | ((db & 0xf8) >> 3);
	  break;
	case 24:
	case 32:
	  *r = 0;
	  *g = 0;
	  *b = 0;
	  switch (pb->byte_order)
	    {
	    case BYTE_ORD_24_RGB:
	      return ((dr & 0xff) << 16) | ((dg & 0xff) << 8) | (db & 0xff);
	      break;
	    case BYTE_ORD_24_RBG:
	      return ((dr & 0xff) << 16) | ((db & 0xff) << 8) | (dg & 0xff);
	      break;
	    case BYTE_ORD_24_BRG:
	      return ((db & 0xff) << 16) | ((dr & 0xff) << 8) | (dg & 0xff);
	      break;
	    case BYTE_ORD_24_BGR:
	      return ((db & 0xff) << 16) | ((dg & 0xff) << 8) | (dr & 0xff);
	      break;
	    case BYTE_ORD_24_GRB:
	      return ((dg & 0xff) << 16) | ((dr & 0xff) << 8) | (db & 0xff);
	      break;
	    case BYTE_ORD_24_GBR:
	      return ((dg & 0xff) << 16) | ((db & 0xff) << 8) | (dr & 0xff);
	      break;
	    default:
	      return 0;
	      break;
	    }
	  break;
	default:
	  return 0;
	  break;
	}
      return 0;
    }

  switch(pb->vis->class)
    {
    case PseudoColor:
    case StaticColor:
      for (i = 0; i < pb->num_of_cols; i++)
	{
	  dr = *r - pb->palette[i].r;
	  if (dr < 0)
	    dr = -dr;
	  dg = *g - pb->palette[i].g;
	  if (dg < 0)
	    dg = -dg;
	  db = *b - pb->palette[i].b;
	  if (db < 0)
	    db = -db;
	  dif = dr + dg + db;
	  if (dif < mindif)
	    {
	      mindif = dif;
	      col = i;
	    }
	}
      break;
    case GrayScale:
    case StaticGray:
      return (((*r * 77) + (*g * 151) + (*b * 28)) >> (16 - pb->depth));
#if 0
      if ((r+g+b) == 765 ) return 0; 	/* HACK */
      return (1 << pb->depth) - ((( ((*r * 54) + (*g * 183) + (*b * 19)) / 256) * 0xffff )/ (1 << pb->depth)); /* TODO should be oxffffffff ?? */
#endif      
    }

  *r -= pb->palette[col].r;
  *g -= pb->palette[col].g;
  *b -= pb->palette[col].b;
  col = pb->palette[col].pixel;
  return col;
}

MBPixbuf *
mb_pixbuf_new(Display *dpy, int scr)
{
  XGCValues gcv;
  unsigned long rmsk, gmsk, bmsk;
  MBPixbuf *pb = malloc(sizeof(MBPixbuf));  

  pb->dpy = dpy;
  pb->scr = scr;

  pb->depth = DefaultDepth(dpy, scr);
  pb->vis   = DefaultVisual(dpy, scr); 
  pb->root  = RootWindow(dpy, scr);

  pb->palette = NULL;

  rmsk = pb->vis->red_mask;
  gmsk = pb->vis->green_mask;
  bmsk = pb->vis->blue_mask;

  if ((rmsk > gmsk) && (gmsk > bmsk))
    pb->byte_order = BYTE_ORD_24_RGB;
  else if ((rmsk > bmsk) && (bmsk > gmsk))
    pb->byte_order = BYTE_ORD_24_RBG;
  else if ((bmsk > rmsk) && (rmsk > gmsk))
    pb->byte_order = BYTE_ORD_24_BRG;
  else if ((bmsk > gmsk) && (gmsk > rmsk))
    pb->byte_order = BYTE_ORD_24_BGR;
  else if ((gmsk > rmsk) && (rmsk > bmsk))
    pb->byte_order = BYTE_ORD_24_GRB;
  else if ((gmsk > bmsk) && (bmsk > rmsk))
    pb->byte_order = BYTE_ORD_24_GBR;
  else
    pb->byte_order = 0;

  if ((pb->depth <= 8))
    {
      XWindowAttributes   xwa;
      if (XGetWindowAttributes(dpy, pb->root, &xwa))
	{
	  if (xwa.colormap)
	    pb->root_cmap = xwa.colormap;
	  else
	    pb->root_cmap = 0;
	}
      else
	pb->root_cmap = 0;
      pb->num_of_cols = _paletteAlloc(pb);
    }

  /* TODO: No exposes ? */
  gcv.foreground = BlackPixel(dpy, scr);
  gcv.background = WhitePixel(dpy, scr);

  pb->gc = XCreateGC( dpy, pb->root, GCForeground | GCBackground, &gcv);

  return pb;
}

MBPixbufImage *
mb_pixbuf_img_new(MBPixbuf *pb, int w, int h)
{
  MBPixbufImage *img;

  img = malloc(sizeof(MBPixbufImage));
  img->width = w;
  img->height = h;

  img->rgba = malloc(sizeof(unsigned char)*((w*h*4)+1));
  memset(img->rgba, 0, sizeof(unsigned char)*((w*h*4)+1));

  img->ximg = NULL;

  img->_safe = False;

  return img;
}


MBPixbufImage *
mb_pixbuf_img_new_from_drawable(MBPixbuf *pb, Drawable drw, Drawable msk,
				int sx, int sy, int sw, int sh)
{
  int i,x,y,br,bg,bb,mg,mb,mr,lr,lg,lb;
  unsigned long xpixel;
  unsigned char *p;
  MBPixbufImage *img;

  XImage *ximg, *xmskimg = NULL;
  int num_of_cols = 1 << pb->depth;

  Window chld;
  unsigned int rx, ry, rw, rh, rb, rdepth;

  /* XXX should probably tray an X error here. */
  XGetGeometry(pb->dpy, (Window)drw, &chld, &rx, &rx,
	       (unsigned int *)&rw, (unsigned int *)&rh,
	       (unsigned int *)&rb, (unsigned int *)&rdepth);

  if ( (sx + sw) > rw || (sy + sh) > rh || rdepth != pb->depth)
    {
      /* area wanted is bigger than pixmap */
      printf("get geometry gave +%i+%i %ix%i depth: %i\n", 
	     rx, ry, rw, rh, rdepth );
      return NULL;
    }

  ximg = XGetImage(pb->dpy, drw, sx, sy, sw, sh, -1, ZPixmap);

  if (msk != None)
    xmskimg = XGetImage(pb->dpy, msk, sx, sy, sw, sh, -1, ZPixmap);

  if (ximg == NULL) return NULL;

  img = mb_pixbuf_img_new(pb, sw, sh);

  p = img->rgba;

  if (pb->depth > 8)
    {
      switch (pb->depth) {
      case 15:
        br = 7;
        bg = 2;
        bb = 3;
        mr = mg = mb = 0xf8;
	lr = lg = lb = 0;
        break;
      case 16:
	br = 8;
	bg = 3;
	lb = 3;
	bb = lr = lg = 0;
        mr = mb = 0xf8;
        mg = 0xfc;
        break;
      case 24:
      case 32:
        br = 16;
        bg = 8;
        bb = 0;
	lr = lg = lb = 0;
        mr = mg = mb = 0xff;
        break;
      default:
        return NULL;
      }

      for (y = 0; y < sh; y++)
	for (x = 0; x < sw; x++)
	  {
	    xpixel = XGetPixel(ximg, x, y);
	    *p++ = (((xpixel >> br) << lr) & mr);      /* r */
	    *p++ = (((xpixel >> bg) << lg) & mg);      /* g */
	    *p++ = (((xpixel >> bb) << lb) & mb);      /* b */
	    if (xmskimg && XGetPixel(xmskimg, x, y))
	      {
		*p++ = 255;
	      }
	    else *p++ = 0;	/* 0  */
	  }
    }
  else
    {
      XColor cols[256];
      MBPixbufColor mbcols[256];

      for (i = 0; i < num_of_cols; i++) {
	cols[i].pixel = i;
	cols[i].flags = DoRed | DoGreen | DoBlue;
      }
      XQueryColors(pb->dpy, pb->root_cmap, cols, num_of_cols);
      for (i = 0; i < num_of_cols; i++) {
	mbcols[i].r = cols[i].red >> 8;
	mbcols[i].g = cols[i].green >> 8;
	mbcols[i].b = cols[i].blue >> 8;
	mbcols[i].pixel = cols[i].pixel;
      }
      for (x = 0; x < sw; x++)
	for (y = 0; y < sh; y++)
	  {
	    xpixel = XGetPixel(ximg, x, y);
	    *p++ = mbcols[xpixel & 0xff].r;
	    *p++ = mbcols[xpixel & 0xff].g;
	    *p++ = mbcols[xpixel & 0xff].b;
	    if (xmskimg && XGetPixel(xmskimg, x, y))
	      {
		*p++ = 255;
	      }
	    else *p++ = 0;		          
	  }
    }


  return img;
}

MBPixbufImage *
mb_pixbuf_img_clone(MBPixbuf *pb, MBPixbufImage *img)
{
  MBPixbufImage *img_new = mb_pixbuf_img_new(pb, img->width, img->height);
  memcpy(img_new->rgba, img->rgba, 
	 sizeof(unsigned char)*((img->width*img->height*4)+1));
  return img_new;
}

void
mb_pixbuf_img_free(MBPixbuf *pb, MBPixbufImage *img)
{
  if (img->rgba) free(img->rgba);
  /*  if (img->ximg) XDestroyImage (img->ximg); */
  free(img);
}

MBPixbufImage *
mb_pixbuf_img_new_from_file(MBPixbuf *pb, const char *filename)
{
  MBPixbufImage *img;

  img = malloc(sizeof(MBPixbufImage));

#ifdef USE_PNG
  if (!strcasecmp(&filename[strlen(filename)-4], ".png"))
    img->rgba = _load_png_file( filename, &img->width, 
				&img->height, &img->has_alpha ); 
  else 
#endif
if (!strcasecmp(&filename[strlen(filename)-4], ".xpm"))
    img->rgba = _load_xpm_file( pb, filename, &img->width, 
				&img->height, &img->has_alpha ); 
  else img->rgba = NULL;

  if (img->rgba == NULL)
    {
      /* Load failed */
      free(img);
      return NULL;
    }

  img->ximg = NULL;


  img->_safe = False;

  return img;

}

void
mb_pixbuf_img_fill(MBPixbuf *pb, MBPixbufImage *img,
		   int r, int g, int b, int a)
{
  unsigned char *p = img->rgba;
  int x,y;

  for(y=0; y<img->height; y++)
    for(x=0; x<img->width; x++)
	{
	  *p++ = r;
	  *p++ = g;
	  *p++ = b;
	  *p++ = a;
	}
}

void
mb_pixbuf_img_composite(MBPixbuf *pb, MBPixbufImage *dest,
			MBPixbufImage *src, int dx, int dy)
{
  /* XXX depreictaed, should really now use copy_composite */
  int x, y, r, g, b, a;
  unsigned char *sp, *dp;
  

  sp = src->rgba;
  dp = dest->rgba;

  dp += ((dest->width*4)*dy) + (dx*4);

  for(y=0; y<src->height; y++)
    {
      for(x=0; x<src->width; x++)
	{
	  r = *sp++;
	  g = *sp++;
	  b = *sp++;
	  a = *sp++;

	  alpha_composite(*dp, r, a, *dp);
	  dp++;
	  alpha_composite(*dp, g, a, *dp);
	  dp++;
	  alpha_composite(*dp, b, a, *dp);
	  dp++;
	  dp++;
	}
      dp += (dest->width-src->width)*4;
    }
}

void
mb_pixbuf_img_copy_composite(MBPixbuf *pb, MBPixbufImage *dest,
			     MBPixbufImage *src, int sx, int sy, 
			     int sw, int sh, int dx, int dy)
{
  int x, y, r, g, b, a;
  unsigned char *sp, *dp;
  
  sp = src->rgba;
  dp = dest->rgba;

  dp += ((dest->width*4)*dy) + (dx*4);
  sp += ((src->width*4)*sy)  + (sx*4);

  for(y=0; y<sh; y++)
    {
      for(x=0; x < sw; x++)
	{
	  r = *sp++;
	  g = *sp++;
	  b = *sp++;
	  a = *sp++;

	  alpha_composite(*dp, r, a, *dp);
	  dp++;
	  alpha_composite(*dp, g, a, *dp);
	  dp++;
	  alpha_composite(*dp, b, a, *dp);
	  dp++;
	  dp++;
	}
      dp += (dest->width-sw)*4;
      sp += (src->width-sw)*4;
    }
}


void
mb_pixbuf_img_copy(MBPixbuf *pb, MBPixbufImage *dest,
		   MBPixbufImage *src, int sx, int sy, int sw, int sh,
		   int dx, int dy)
{
  int x, y;
  unsigned char *sp, *dp;
  
  sp = src->rgba;
  dp = dest->rgba;

  dp += ((dest->width*4)*dy) + (dx*4);
  sp += ((src->width*4)*sy)  + (sx*4);

  for(y=0; y<sh; y++)
    {
      for(x=0; x < sw; x++)
	{
	  *dp++ = *sp++;
	  *dp++ = *sp++;
	  *dp++ = *sp++;
	  *dp++ = *sp++;
	}
      dp += (dest->width-sw)*4;
      sp += (src->width-sw)*4;
    }
}

MBPixbufImage *
mb_pixbuf_img_scale_down(MBPixbuf *pb, MBPixbufImage *img, 
			 int new_width, int new_height)
{
  MBPixbufImage *img_scaled;
  unsigned char *dest, *src, *srcy;
  int *xsample, *ysample;
  int bytes_per_line, i, x, y, r, g, b, a, nb_samples, xrange, yrange, rx, ry;

  if ( new_width > img->width || new_height > img->height) 
    return NULL;

 img_scaled = mb_pixbuf_img_new(pb, new_width, new_height);

  xsample = malloc( (new_width+1) * sizeof(int));
  ysample = malloc( (new_height+1) * sizeof(int));
  bytes_per_line = (img->width << 2);

  for ( i = 0; i <= new_width; i++ )
    xsample[i] = i*img->width/new_width;
  for ( i = 0; i <= new_height; i++ )
    ysample[i] = i*img->height/new_height * img->width;

  dest = img_scaled->rgba;

  /* scan output image */
  for ( y = 0; y < new_height; y++ ) {
    yrange = ( ysample[y+1] - ysample[y] )/img->width;
    for ( x = 0; x < new_width; x++) {
      xrange = xsample[x+1] - xsample[x];
      srcy = img->rgba + (( ysample[y] + xsample[x] ) << 2 );

      /* average R,G,B,A values on sub-rectangle of source image */
      nb_samples = xrange * yrange;
      if ( nb_samples > 1 ) {
	r = 0;
	g = 0;
	b = 0;
	a = 0;
	for ( ry = 0; ry < yrange; ry++ ) {
	  src = srcy;
	  for ( rx = 0; rx < xrange; rx++ ) {
	    /* average R,G,B,A values */
	    r += *src++;
	    g += *src++;
	    b += *src++;
	    a += *src++;
	  }
	  srcy += bytes_per_line;
	}
	*dest++ = r/nb_samples;
	*dest++ = g/nb_samples;
	*dest++ = b/nb_samples;
	*dest++ = a/nb_samples; 
      }
      else {
	*((int *) dest) = *((int *) srcy);
	dest += 4;
      }
    }
  }
  /* cleanup */
  free( xsample );
  free( ysample );

  return img_scaled;
}

MBPixbufImage *
mb_pixbuf_img_scale_up(MBPixbuf *pb, MBPixbufImage *img, 
		       int new_width, int new_height)
{
  MBPixbufImage *img_scaled;
  unsigned char *dest, *src;
  int x, y, xx, yy, bytes_per_line;

  if ( new_width < img->width || new_height < img->height) 
    return NULL;

 img_scaled = mb_pixbuf_img_new(pb, new_width, new_height);

  bytes_per_line = (img->width << 2);

  dest = img_scaled->rgba;
  
  for (y = 0; y < new_height; y++)
    {
      yy = (y * img->height) / new_height;
      for (x = 0; x < new_width; x++)
      {
	 xx = (x * img->width) / new_width;
	 src = img->rgba + ((yy * bytes_per_line)) + (xx << 2);
	 *dest++ = *src++;
	 *dest++ = *src++;
	 *dest++ = *src++;
	 *dest++ = *src++;
      }
   }

  return img_scaled;
}

MBPixbufImage *
mb_pixbuf_img_scale(MBPixbuf *pb, MBPixbufImage *img, 
		    int new_width, int new_height)
{
  if (new_width >= img->width && new_height >= img->height)
    return mb_pixbuf_img_scale_up(pb, img, new_width, new_height);

  if (new_width <= img->width && new_height <= img->height)
    return mb_pixbuf_img_scale_down(pb, img, new_width, new_height);

  /* TODO: all scale functions should check for a dimention change
           being zero and act accordingly - ie faster. 
  */
  if (new_width >= img->width && new_height <= img->height)
    {
      MBPixbufImage *tmp=NULL, *tmp2 = NULL;
      tmp = mb_pixbuf_img_scale_up(pb, img, new_width, img->height);
      
      tmp2 = mb_pixbuf_img_scale_down(pb, tmp, new_width, new_height);
      mb_pixbuf_img_free(pb, tmp);
      return tmp2;
    }

  if (new_width <= img->width && new_height >= img->height)
    {
      MBPixbufImage *tmp, *tmp2;
      tmp = mb_pixbuf_img_scale_down(pb, img, new_width, img->height);
      
      tmp2 = mb_pixbuf_img_scale_up(pb, tmp, new_width, new_height);
      mb_pixbuf_img_free(pb, tmp);
      return tmp2;
    }

  return NULL;
}

void
mb_pixbuf_img_render_to_drawable(MBPixbuf    *pb,
				 MBPixbufImage *img,
				 Drawable     drw,
				 int drw_x,
				 int drw_y)
{
      int bitmap_pad;
      unsigned char *p;
      unsigned long pixel;
      int x,y;
      int r, g, b;

      bitmap_pad = ( pb->depth > 16 )? 32 : (( pb->depth > 8 )? 16 : 8 );

      img->ximg = XCreateImage( pb->dpy, pb->vis, pb->depth, 
				ZPixmap, 0, 0,
				img->width, img->height, bitmap_pad, 0);

      img->ximg->data = malloc( img->ximg->bytes_per_line*img->height );

      p = img->rgba;

      for(y=0; y<img->height; y++)
	{
	  for(x=0; x<img->width; x++)
	    {
	      r = ( *p++ );
	      g = ( *p++ );
	      b = ( *p++ );
	      p++;  /* Alpha */

	      pixel = mb_pixbuf_get_pixel(pb, &r, &g, &b);
	      XPutPixel(img->ximg, x, y, pixel);
	    }
	}
  
      XPutImage( pb->dpy, drw, pb->gc, img->ximg, 0, 0, 
		 drw_x, drw_y, img->width, img->height);

      XDestroyImage (img->ximg);
      img->ximg = NULL;		/* Safety On */
}

