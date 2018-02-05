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
parse_tc_string(FILE *fd, Handles *handles, char *dest, size_t string_length);

static PyObject *
parse_tc_longstring(FILE *fd, Handles *handles, char *dest);

static PyObject *
parse_tc_shortstring(FILE *fd, Handles *handles, char *dest);

static void
parse_tc_classdesc(FILE *fd, Handles *handles, JavaType_Type *type);

static PyObject *
parse_tc_reference(FILE *fd, Handles *handles);

static PyObject *
parse_tc_array(FILE *fd, Handles *handles);

static PyObject *
get_value(FILE *fd, Handles *handles, char tc_num);

static JavaType_Type *
get_field_descriptor(FILE *fd, Handles *handles);

static PyObject *
get_values_class_desc(FILE *fd, Handles *handles, JavaType_Type *class_desc);

/* unit tests */

static PyObject *
__test_parse_primitive_array(PyObject *self, PyObject *args);

static PyObject *
__test_parse_class_descriptor(PyObject *self, PyObject *args);

 /* java.util collections */

 /* Java Containers implements their own write methods
 * that have block data written to the stream. This 
 * means that all of the block data has to be parsed per object.
 */

/* 1.  java.util.ArrayDeque 
 * 2.  java.util.ArrayList 
 * 3.  java.util.BitSet 
 *      - works as a normal class, but needs special condition to represent it as a bitset
 * 4.  java.util.Calendar - probably just ignore this one and get the block data
 * 5.  java.util.Collections {theres like a million write object methods}
 * 6.  java.util.Date 
 * 7.  java.util.EnumMap
 * 8.  java.util.HashMap
 * 9.  java.util.HashSet
 * 10. java.util.Hashtable
 * 11. java.util.IdentityHashMap
 * 12. java.util.LinkedList
 * 13. java.util.PriorityQueue
 * 13. <didn't finish list yet> stopped at linked list
 */

typedef struct LinkedListBlock Java_LinkedList;

struct 
ArrayDequeBlock {
    /* default write object */
    /* size of deque (int) */
    /* write each object */
};

struct 
ArrayListBlock {
    /* default write object */
    /* length of array (int) */
    /* write objects */
};

struct 
DateBlock {
    /* writeLong */  
};

struct 
EnumMapBlock {
    /* defaultWriteObject */
    /* write size (int) */
    /* writeObject(key), writeObject(value) for each k-v pair in EnumMap */
};

struct 
HashMapBlock {
    /* defaultWriteObject */
    /* writeInt buckets */
    /* writeInt size */
    /* writeObject(key) writeObject(value) for each k-v pair in Map */
};

struct 
HashSetBlock {
    /* defaultWriteObject */
    /* capacity (int) */
    /* load factor (float) */
    /* size (int) */
    /* write out all objects with writeObject */
};

struct 
HashTableBlock {
    /* defaultWriteObject */
    /* table length (int) */
    /* count (int) */
    /* writeObject(key) writeObject(value) */
};

struct 
IdentityHashMapBlock {
    /* defaultWriteObject */
    /* size (int) */
    /* writeObject(key: could be null?), writeObject(value) */
};

struct 
LinkedListBlock {
    /* defaultWriteObject */
    /* size (int) */
    /* writeObject for each object in list */
};

/* referenced functions */
static PyObject *
List_ReadObject(FILE *fd, Handles *handles, JavaType_Type *class_desc);

static PyObject *
parse_block_data(FILE *fd, Handles *handles, JavaType_Type *, PyObject *data);

static PyObject *
BitSet_ReadObject(FILE *fd, Handles *handles, PyObject *dict);

static PyObject *
Date_ReadObject(FILE *fd);

static PyObject *
EnumMap_ReadObject(FILE *fd);

static PyObject *
HashMap_ReadObject(FILE *fd, Handles *handles, JavaType_Type *class_desc);

static PyObject *
HashSet_ReadObject(FILE *fd, Handles *handles, JavaType_Type *class_des);

static PyObject *
HashTable_ReadObject(FILE *fd);

static PyObject *
IdentityHashMap_ReadObject(FILE *fd);

static PyObject *
PriorityQueue_ReadObject(FILE *fd, Handles *handles, JavaType_Type *class_desc);