#include <Python.h>

static PyMethodDef UsqldMethods[] = 
{
   

     {
	"connect",  spam_system, METH_VARARGS,
	       "Connect to a database server."
     }
   ,
     {
	NULL, NULL, 0, NULL
     }
           /* Sentinel */
}
;


void
initusqld(void)
{
   PyObject *m,*d;
   m = Py_InitModule("usqld",UsqldMethods);
   d = PyModule_GetDict(m);
   
}

static PyObject * 
usqld_connect(PyObject * self,PyObject * args)
{
   
}
