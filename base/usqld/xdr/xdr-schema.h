
#ifndef XDR_SCHEMA_H
#define XDR_SCHEMA_H

/*
 * base type that all other types can be cast into, 
 * also used for basic types, including VAROPAQUE
 */
typedef struct  
{
  XDR_type type;
}XDR_schema;


typedef struct
{
  XDR_schema base;
  size_t num_elems; /*!=NULL*/
} XDR_opaque;

typedef struct 
{
  XDR_schema base;
  size_t num_elems;
  XDR_schema * elem_type;
} XDR_array;

typedef struct  
{
  XDR_schema base;
  size_t num_elems;
  XDR_schema ** elems;
}XDR_struct;

typedef struct {
  XDR_schema * t;
  unsigned int d;
}XDR_union_discrim;

typedef struct  
{
  XDR_schema base;
  size_t num_alternatives;
  XDR_union_discrim * elems;
}XDR_type_union;

#define XDR_SCHEMA_ARRAY(x) (assert(x->type==XDR_FIXEDARRAY||\
			     x->type==XDR_VARARRAY),\
		      (XDR_array*)x)

#define XDR_SCHEMA_STRUCT(x) (assert(x->type==XDR_STRUCT),\
		      (XDR_struct*)x)

#define XDR_SCHEMA_UNION(x) (assert(x->type==XDR_UNION),\
		      (XDR_type_union*)x)

#define XDR_SCHEMA_OPAQUE(x) (assert(x->type==XDR_FIXEDOPAQUE ||\
			      x->type==XDR_VARARRAY),\
		       (XDR_opaque*)x)

#define XDR_malloc(type) (type*) malloc(sizeof(type))
#define XDR_mallocn(type,n) (type*) malloc(sizeof(type) * n)
#define XDR_free(x) (assert(x!=NULL),free(x),x=NULL)

XDR_schema * XDR_schema_new_typedesc(XDR_type t);

XDR_schema * XDR_schema_new_opaque(size_t len);
XDR_schema * XDR_schema_new_array(XDR_schema *t,size_t len);
XDR_schema * XDR_schema_new_struct(size_t num_elems, XDR_schema **elems);
XDR_schema * XDR_schema_new_type_union(size_t num_alternatives,const XDR_union_discrim *elems);

#define XDR_schema_new_varopaque() XDR_schema_new_opaque(0)
#define XDR_schema_new_fixedopaque(n) XDR_schema_new_opaque(n)
#define XDR_schema_new_fixedarray(t,n) XDR_schema_new_array(t,n)
#define XDR_schema_new_vararray(t) XDR_schema_new_array(t,0)

#define XDR_schema_new_int() XDR_schema_new_typedesc(XDR_INT)
#define XDR_schema_new_uint() XDR_schema_new_typedesc(XDR_UINT)
#define XDR_schema_new_enum() XDR_schema_new_typedesc(XDR_ENUM)
#define XDR_schema_new_bool() XDR_schema_new_typedesc(XDR_BOOL)
#define XDR_schema_new_hyper() XDR_schema_new_typedesc(XDR_HYPER)
#define XDR_schema_new_uhyper() XDR_schema_new_typedesc(XDR_UHYPER)
#define XDR_schema_new_float() XDR_schema_new_typedesc(XDR_FLOAT)
#define XDR_schema_new_double() XDR_schema_new_typedesc(XDR_DOUBLE)
#define XDR_schema_new_longdouble() XDR_schema_new_typedesc(XDR_LONGDOUBLE)
#define XDR_schema_new_string() XDR_schema_new_typedesc(XDR_STRING)
#define XDR_schema_new_void() XDR_schema_new_typedesc(XDR_VOID)

void XDR_dump_schema(XDR_schema *s);

#endif
