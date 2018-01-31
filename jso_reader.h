#include <Python.h>
#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <wchar.h>
#include "javatype.h"

#define TC_NULL 0x70
#define TC_REFERENCE 0x71
#define TC_CLASSDESC 0x72
#define TC_OBJECT 0x73
#define TC_STRING 0x74
#define TC_ARRAY 0x75
#define TC_CLASS 0x76
#define TC_BLOCKDATA 0x77
#define TC_ENDBLOCKDATA 0x78
#define TC_RESET 0x79
#define TC_EXCEPTION 0x7B
#define TC_LONGSTRING 0x7C
#define TC_PROXYCLASSDESC 0x7D
#define TC_ENUM 0x7E 

/* const dict keys */

#define uint16_switch(a) (((a) & 0xFF00) >> 8 | ((a) & 0x00FF) << 8)
#define uint32_switch(a) \
   (((a) & 0xFF000000) >> 24 \
  | ((a) & 0x00FF0000) >>  8 \
  | ((a) & 0x0000FF00) <<  8 \
  | ((a) & 0x000000FF) << 24)
#define uint64_switch(a) \
	  (((a) & 0xFF00000000000000) >> 56 \
   | ((a) & 0x00FF000000000000) >> 40 \
   | ((a) & 0x0000FF0000000000) >> 24 \
   | ((a) & 0x000000FF00000000) >>  8 \
   | ((a) & 0x00000000FF000000) <<  8 \
   | ((a) & 0x0000000000FF0000) << 24 \
   | ((a) & 0x000000000000FF00) << 40 \
   | ((a) & 0x00000000000000FF) << 56)

#define uint64_reverse_bytes(a) a = uint64_switch(a)
#define uint32_reverse_bytes(a) a = uint32_switch(a)
#define uint16_reverse_bytes(a) a = uint16_switch(a)


/* function declarations */

static PyObject *
java_stream_reader(PyObject *self, PyObject *args);

static PyObject *
parse_stream(FILE *fd, Handles *handles, JavaType_Type *);

static PyObject *
parse_tc_object(FILE *fd, Handles *handles);

static PyObject *
parse_tc_string(FILE *fd, Handles *handles, char *dest);

static PyObject *
parse_tc_classdesc(FILE *fd, Handles *handles, JavaType_Type *type);

static PyObject *
parse_tc_array(FILE *fd, Handles *handles);

static PyObject *
get_values_from_java_type(FILE *fd, PyObject *class_desc);

static PyObject *
get_value(FILE *fd, char tc_num);

static JavaType_Type *
get_field_descriptor(FILE *fd, Handles *handles);

static PyObject *
get_array_values(FILE *fd, JavaType_Type *ob);

/* unit tests */

static PyObject *
__test_parse_primitive_array(PyObject *self, PyObject *args);

