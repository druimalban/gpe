#ifndef XDR_TREE_H
#define XDR_TREE_H

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


#define u_int32_t unsigned int 
#define u_int64_t unsigned long long

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

/*
  tree casting operators, 
  these will assert if the type of the tree given is incorrect
*/
#define XDR_TREE_SIMPLE(x)(assert(XDR_tree_is_simple(x)),\
			   (XDR_tree_simple*)x)

#define XDR_TREE_COMPOUND(x)(assert(XDR_tree_is_compound(x)),\
			   (XDR_tree_compound*)x)

#define XDR_TREE_STR(x)(assert(XDR_tree_is_str(x)),\
			   (XDR_tree_str*)x)



int XDR_tree_is_simple(XDR_tree * t);
int XDR_tree_is_compound(XDR_tree * t);
int XDR_tree_is_str(XDR_tree * t);

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
			       size_t length,
			       unsigned char * data);
#define XDR_tree_new_fixed_opaque(l,d) \
  XDR_tree_new_opaque(XDR_FIXEDOPAQUE,l,d)
#define XDR_tree_new_var_opaque(l,d) \
  XDR_tree_new_opaque(XDR_VAROPAQUE,l,d)

XDR_tree * XDR_tree_new_string(const char * data);
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
XDR_tree * XDR_tree_new_void();
/*accessors*/


unsigned int XDR_t_get_union_disc(XDR_tree_compound * t);
XDR_tree * XDR_t_get_union_t(XDR_tree_compound * t);

char *  XDR_t_get_string(XDR_tree_str * t);
unsigned char *  XDR_t_get_data(XDR_tree_str * t);
size_t XDR_t_get_data_len(XDR_tree_str * t);
unsigned int XDR_t_get_uint(XDR_tree_simple * t);


unsigned int XDR_t_get_comp_len(XDR_tree_compound * t);
XDR_tree *  XDR_t_get_comp_elem(XDR_tree_compound * t,unsigned int  n);
void  XDR_t_set_comp_elem(XDR_tree_compound * t, unsigned int n, XDR_tree * val);

void XDR_tree_dump(XDR_tree*t);
void XDR_tree_free(XDR_tree * t);


#endif
