/*
 * ol-irc - A small irc client using GTK+
 *
 * Copyright (C) 1998, 1999 Yann Grossel [Olrick]
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
 *
 */

#ifdef USE_PYTHON

#include "olirc.h"
#include "python-olirc.h"

#include "windows.h"

//	Py_Finalize();

/* Functions of the 'olirc' module */

static PyObject *Olirc_Py_VersionString(PyObject *self, PyObject *args)
{
	if (!PyArg_ParseTuple(args, "")) return NULL;
	return Py_BuildValue("s", VER_STRING);
}

static PyObject *Olirc_Py_ReleaseDate(PyObject *self, PyObject *args)
{
	if (!PyArg_ParseTuple(args, "")) return NULL;
	return Py_BuildValue("s", RELEASE_DATE);
}

static PyObject *Olirc_Py_Stdout(PyObject *self, PyObject *args)
{
	char *string;
	if (!PyArg_ParseTuple(args, "s", &string)) return NULL;
	VW_output(((GUI_Window *) GW_List->data)->vw_active, T_WARNING, "t", string);
	return Py_BuildValue("");
}

static PyObject *Olirc_Py_Stderr(PyObject *self, PyObject *args)
{
	char *string;
	if (!PyArg_ParseTuple(args, "s", &string)) return NULL;
	VW_output(((GUI_Window *) GW_List->data)->vw_active, T_ERROR, "t", string);
	return Py_BuildValue("");
}

static PyObject *Olirc_Py_Bind(PyObject *self, PyObject *args)
{
	if (!PyArg_ParseTuple(args, "")) return NULL;
	return Py_BuildValue("");
}

static PyMethodDef Olirc_Py_Methods[] = {
	{"VersionString", Olirc_Py_VersionString, METH_VARARGS},
	{"ReleaseDate", Olirc_Py_ReleaseDate, METH_VARARGS},
	{"Stdout", Olirc_Py_Stdout, METH_VARARGS},
	{"Stderr", Olirc_Py_Stderr, METH_VARARGS},
	{"Bind", Olirc_Py_Bind, METH_VARARGS},
//	{"Server_Connect", Olirc_Py_Server_Quit, METH_VARARGS},
//	{"Server_Disconnect", Olirc_Py_Server_Disconnect, METH_VARARGS},
//	{"Server_Quit", Olirc_Py_Server_Quit, METH_VARARGS},
//	{"Server_QuitDialog", Olirc_Py_Server_QuitDialog, METH_VARARGS},
//	{"Window_Close", Olirc_Py_Window_Close, METH_VARARGS},
//	{"Window_Clear", Olirc_Py_Window_Clear, METH_VARARGS},
//	{"Window_Output", Olirc_Py_Window_Output, METH_VARARGS},
//	{"Stderr", Olirc_Py_Stderr, METH_VARARGS},
//	{"Stderr", Olirc_Py_Stderr, METH_VARARGS},
//	{"Stderr", Olirc_Py_Stderr, METH_VARARGS},
//	{"Stderr", Olirc_Py_Stderr, METH_VARARGS},
//	{"Stderr", Olirc_Py_Stderr, METH_VARARGS},
	{NULL, NULL}
};

int Init_Olirc_Py(int argc, char **argv)
{
	Py_SetProgramName(argv[0]);
	Py_Initialize();
	PyImport_AddModule("olirc");
	Py_InitModule("olirc", Olirc_Py_Methods);
	return Py_IsInitialized();
}

#endif	/* USE_PYTHON */

/* vi: set ts=3: */

