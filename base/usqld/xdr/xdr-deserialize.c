#include <sys/types.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <netinet/in.h>
#include <string.h>
#include "xdr.h"

int
XDR_deserialize_elem (XDR_schema * s, int fd, XDR_tree ** out_t)
{
  unsigned char buf[4];
  int rv = XDR_OK;

#ifdef VERBOSE_DEBUG
  fprintf (stderr, "deserialize:about to try and read a %s\n",
	   XDR_find_type_name (s->type));
#endif

  switch (s->type)
    {
    case XDR_INT:
    case XDR_BOOL:
    case XDR_UINT:
    case XDR_FLOAT:
      {
	XDR_tree_simple *t;
	if (-1 == read (fd, buf, 4))
	  return XDR_IO_ERROR;
	XDR_tree_new_simple (s->type, &t);
	t->val.uintVal = ntohl (*((u_int32_t *) buf));
	*out_t = (XDR_tree *) t;
      };
      break;
    case XDR_HYPER:
    case XDR_UHYPER:
    case XDR_DOUBLE:
    case XDR_LONGDOUBLE:
      return XDR_UNSUPPORTED;
    case XDR_FIXEDOPAQUE:
    case XDR_VAROPAQUE:
    case XDR_STRING:
      {
	XDR_tree_str *t = NULL;
	size_t slen = 0;
	unsigned char *data = NULL;

	if (-1 == read (fd, buf, 4))
	  return XDR_IO_ERROR;

	slen = ntohl (*((u_int32_t *) buf));

	if (XDR_OK != (rv = XDR_tree_new_str (s->type, slen, &t)))
	  {
	    return rv;
	  }

	data = t->data;

	while (slen)
	  {
	    if (-1 == read (fd, buf, 4))
	      return XDR_IO_ERROR;
	    if (slen < 4)
	      {
		memcpy (data, buf, slen);
		slen -= slen;
	      }
	    else
	      {
		memcpy (data, buf, 4);
		slen -= 4;
		data += 4;
	      }
	  }
	if (s->type == XDR_STRING)	//just in case 
	  t->data[t->len] = '\0';
	*out_t = (XDR_tree *) t;
      }
      break;
    case XDR_FIXEDARRAY:
    case XDR_VARARRAY:
      {
	size_t len;
	int i;
	XDR_tree_compound *t = NULL;

	if (s->type == XDR_VARARRAY)
	  {
	    if (-1 == read (fd, buf, 4))
	      return XDR_IO_ERROR;

	    len = ntohl (*((int *) buf));
	  }
	else
	  {
	    len = ((XDR_array *) s)->num_elems;
	  }

	if (XDR_OK != (rv = XDR_tree_new_compound (s->type, len, &t)))
	  return rv;

	for (i = 0; i < len; i++)
	  {
	    if (XDR_OK !=
		(rv =
		 XDR_deserialize_elem (((XDR_array *) s)->elem_type, fd,
				       ((XDR_tree_compound *) t)->subelems +
				       i)))
	      {
		XDR_tree_free ((XDR_tree *) t);
		return rv;
	      }
	  }
	*out_t = (XDR_tree *) t;
      }
      break;
    case XDR_STRUCT:
      {
	size_t len;
	int i;
	XDR_tree_compound *t;

	len = ((XDR_struct *) s)->num_elems;

	if (XDR_OK != (rv = (XDR_tree_new_compound (s->type, len, &t))))
	  {
	    return rv;
	  }

	for (i = 0; i < len; i++)
	  {
	    if (XDR_OK !=
		(rv =
		 XDR_deserialize_elem (((XDR_struct *) s)->elems[i], fd,
				       ((XDR_tree_compound *) t)->subelems +
				       i)))
	      return rv;
	  }

	*out_t = (XDR_tree *) t;
      }
      break;
    case XDR_UNION:
      {
	int i;
	unsigned int d_val = 0;
	XDR_schema *d_t = NULL;
	XDR_tree_compound *t = NULL;
	XDR_tree_simple *disc = NULL;
	if (-1 == read (fd, buf, 4))
	  return XDR_IO_ERROR;

	d_val = ntohl (*((int *) buf));

	for (i = 0; i < ((XDR_type_union *) s)->num_alternatives; i++)
	  {
	    if (((XDR_type_union *) s)->elems[i].d == d_val)
	      {
		d_t = ((XDR_type_union *) s)->elems[i].t;
		break;
	      }
	  }

	if (NULL == d_t)
	  {
	    return XDR_SCHEMA_VIOLATION;
	  }
	if (XDR_OK != (rv = XDR_tree_new_compound (XDR_UNION, 2, &t)))
	  {
	    return rv;
	  }
	XDR_tree_new_simple (XDR_UINT, &disc);
	disc->val.uintVal = d_val;
	t->subelems[0] = (XDR_tree *) disc;
	if (XDR_OK !=
	    (rv = (XDR_deserialize_elem (d_t, fd, &(t->subelems[1])))))
	  {
	    XDR_tree_free ((XDR_tree *) t);
	    return rv;
	  }
	*out_t = (XDR_tree *) t;
      }
      break;
    case XDR_VOID:
      *out_t = NULL;
      break;
    default:
      return XDR_SCHEMA_VIOLATION;
    }
#ifdef VERBOSE_DEBUG
  fprintf (stderr, "succesfully read a %s\n", XDR_find_type_name (s->type));
#endif
  return XDR_OK;
}
