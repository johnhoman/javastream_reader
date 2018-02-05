#include "Python.h"

#define DEFAULT_REFERENCE_SIZE 300
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

#define isinstance(a, b) ((a)->jt_type == b)

typedef struct JavaType_Type JavaType_Type;
typedef struct StreamReference StreamReference;
typedef struct Handles Handles;


/* This may or may not work for a field descriptor. A field descriptor
 * I think contains no data, only fieldnames and types. This structure is 
 * only intended to be used as a means of holding references to handles
 * */



struct JavaType_Type {
    int jt_type;
    char prim_typecode;
    char obj_typecode;
    char *classname;
    char *fieldname;
    char *string;
    char **proxy_interface_names;
    uint8_t is_primitive:1;
    uint8_t is_array:1;
    uint8_t is_object:1;
    uint8_t unused:5;
    size_t array_size;
    size_t n_fields;
    size_t n_proxy_interface_names;
    size_t n_chars;
    uint64_t serial_version_uid;
    struct {
        uint8_t sc_write_method:1;
        uint8_t sc_serializable:1;
        uint8_t sc_externalizable:1;
        uint8_t sc_block_data:1;
        uint8_t sc_enum:4;
    } flags;
    JavaType_Type **fields;
    JavaType_Type *class_annotation;
    JavaType_Type *super;
    JavaType_Type *class_descriptor;
    PyObject *value;
    size_t ref_count;
};

struct StreamReference {
    uint32_t handle;
    JavaType_Type *ob;
};

struct Handles {
    size_t size;
    size_t index;
    size_t reserved;
    uint32_t next_handle;
    StreamReference **stream;
};

#define Type_Object 1
#define Type_Class 2
#define Type_Array 3
#define Type_ClassDescriptor 4
#define Type_ProxyClassDescriptor 5
#define Type_String 6 
#define Type_Enum 7
#define Type_Reference 8
#define Type_Exception 9

Handles *
Handles_New(size_t size);

uint8_t
Handles_Destruct(Handles *handles);

void 
Handles_Append(Handles *handles, JavaType_Type *ob);

JavaType_Type *
Handles_Find(Handles *handles, uint32_t handle);

JavaType_Type *
JavaType_New(char type);

void
JavaType_Destruct(JavaType_Type *type);

void
Handles_Print(Handles *handles);