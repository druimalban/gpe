#ifndef XDR_H
#define XDR_H
#include <sys/types.h>
#define XDR_OK 0
#define XDR_PMALLOC 1
#define XDR_SCHEMA_VIOLATION 2
#define XDR_IO_ERROR 3
#define XDR_UNSUPPORTED 4
typedef enum {
  XDR_INT,
  XDR_UINT,
  XDR_ENUM,
  XDR_BOOL,
  XDR_HYPER,
  XDR_UHYPER,
  XDR_FLOAT,
  XDR_DOUBLE,
  XDR_LONGDOUBLE,
  XDR_STRING,
  XDR_FIXEDOPAQUE,
  XDR_VAROPAQUE,
  XDR_FIXEDARRAY,
  XDR_VARARRAY,
  XDR_STRUCT,
  XDR_UNION,
  XDR_VOID
}XDR_type;



typedef struct {
  XDR_type type;
}XDR_tree;

/*
 * rep for types of struct,array,union 
 * union is a 2-element array with the discriminant as in INT in [0] 
 * and the content as whatever in [1]
 */
typedef struct {
  XDR_type type;
  size_t nelems;
  XDR_tree ** subelems;
}XDR_tree_compound;


typedef union {
  int   boolVal;
  int32_t intVal;
  u_int32_t uintVal;
  int64_t hypVal;
  u_int64_t uhypVal;
  float    floatVal;
  double   doubleVal;
  long double longDoubleVal;
}XDR_tree_simple_val;
/*
 *rep for all other types.
 */
typedef struct {
  XDR_type type;
  XDR_tree_simple_val val;
}XDR_tree_simple;

typedef struct  {
  XDR_type type;
  size_t len; 
  unsigned char * data;
}XDR_tree_str;

typedef struct {
  XDR_type type;
  unsigned char * data;
}XDR_tree_string ;
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



#define mylloc(type) (type*) malloc(sizeof(type))
#define myllocn(type,n) (type*) malloc(sizeof(type) * n)

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



/*base constructors for tree elements*/
int  XDR_tree_new_compound(XDR_type type,
			   size_t nelems,
			   XDR_tree_compound ** out_t);
int  XDR_tree_new_str (XDR_type type,
		       size_t len,
		       XDR_tree_str** out_t);

void XDR_tree_new_simple(XDR_type type,XDR_tree_simple ** out_t);

/*
  API constructors for XDR tree elements
 */
XDR_tree*  XDR_tree_new_union (unsigned int disc,
			       XDR_tree * val);
XDR_tree* XDR_tree_new_mult (XDR_type type,
			     size_t num_elems,
			     XDR_tree ** elems);
#define XDR_tree_new_fixed_array(n,e) XDR_tree_new_mult(XDR_FIXEDARRAY,en,e)
#define XDR_tree_new_var_array(n,e) XDR_tree_new_mult(XDR_VARARRAY,n,e)
#define XDR_tree_new_struct(n,e) XDR_tree_new_mult(XDR_STRUCT,n,e)
XDR_tree * XDR_tree_new_opaque(XDR_type type,
			       int fixed_size,
			       size_t length,
			       unsigned char * data);
#define XDR_tree_new_fixed_opaque(l,d) \
  XDR_tree_new_opaque(XDR_FIXEDOPAQUE,1,l,d)
#define XDR_tree_new_var_opaque(l,d) \
  XDR_tree_new_opaque(XDR_VAROPAQUE,0,l,d)
XDR_tree * XDR_tree_new_string(char * data);
XDR_tree * XDR_tree_new_num(XDR_type t,u_int32_t val);
#define XDR_tree_new_int(v) XDR_tree_new_num(XDR_INT,(u_int32_t)v)
#define XDR_tree_new_uint(v) XDR_tree_new_num(XDR_UINT,(u_int32_t)v)
#define XDR_tree_new_bool(v) XDR_tree_new_num(XDR_BOOL,(u_int32_t)v)
#define XDR_tree_new_enum(v) XDR_tree_new_num(XDR_ENUM,(u_int32_t)v)

XDR_tree * XDR_tree_new_xhyper(XDR_type t,u_int64_t val);
#define  XDR_tree_new_uhyper(val) XDR_tree_new_xhyper(XDR_UHYPER, \
						      (u_int64_t)val)
#define  XDR_tree_new_hyper(val) XDR_tree_new_xhyper(XDR_HYPER,(u_int64_t)val)
XDR_tree * XDR_tree_new_float(float val);
XDR_tree * XDR_tree_new_double(double val);

/*accessors*/
#define XDR_t_get_union_disc(t)  ((XDR_tree_simple*)\
					 ((XDR_tree_compound*)\
					  t)->subelems[0])->val.uintVal
#define XDR_t_get_union_t(t)  ((XDR_tree_compound*)\
					  t)->subelems[1])

#define XDR_t_get_string(t)  ((XDR_tree_str *)t->data)
#define XDR_t_get_data(t)  ((XDR_tree_str *)t->data)
#define XDR_t_get_data_len(t)  ((XDR_tree_str *)t->len)

#define XDR_t_get_comp_elem(t,n) ((XDR_tree_compound*)t->subelems[n])

void XDR_tree_dump(XDR_tree*t);
void XDR_tree_free(XDR_tree * t);
void XDR_dump_schema(XDR_schema *s);
const char * XDR_find_type_name(XDR_type t);

int  XDR_deserialize_elem(XDR_schema *s,int fd, XDR_tree **out_t);
int  XDR_serialize_elem(XDR_schema *s, XDR_tree *t,int fd);

#endif 
