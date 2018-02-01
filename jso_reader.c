#include "jso_reader.h"


/* *
 * class descriptor: {
 *     suid, 
 *     flags,
 *     class_name,
 *     [fields],
 *     
 * }
 * */


static uint8_t little_endian;

static PyObject *
java_stream_reader(PyObject *self, PyObject *args)
{
    /* *
     * assertions for unimplented platform differences
     * */

    assert(sizeof(float) == 4);
    assert(sizeof(double) == 8);
    assert(sizeof(uint32_t) == 4);
    assert(sizeof(uint16_t) == 2);
    assert(sizeof(long long) == 8);


    /* test endianness big_endian -> 0x0001, little_endian ->0x0100*/
    uint8_t i = 0x0001;
    little_endian = *((char *)&i);

    if (!little_endian){
        return PyErr_NewException("Not Implemented for big endian", NULL, NULL);
    }


    char *filename;

    if (PyArg_ParseTuple(args, "s", &filename)) {
        printf("filename = %s\n", filename);
    }

    FILE *fd = fopen(filename, "rb");

    assert(fd != NULL);

    uint16_t magic_number;
    uint16_t version;
    size_t n_bytes;

    /* validate magic number is correct */
    n_bytes = fread(&magic_number, 1, sizeof(uint16_t), fd);
    assert(n_bytes == sizeof(magic_number));
    assert(uint16_switch(magic_number) == 0xaced);

    /* validate version number is correct */
    n_bytes = fread(&version, 1, sizeof(uint16_t), fd);
    assert(n_bytes == sizeof(version));
    assert(uint16_switch(version) == 0x0005);

    Handles *handles = Handles_New(DEFAULT_REFERENCE_SIZE);
    /* global reference to handles */

    /* return parser */
    PyObject *data;// = PyList_New(0);
    // PyObject *dict;
    // assert(data != NULL);

    data = parse_stream(fd, handles, NULL);

    fclose(fd);
    return data;
}

static PyObject *
parse_stream(FILE *fd, Handles *handles, JavaType_Type *type)
{
    size_t n_bytes;
    char tc_typecode;

    n_bytes = fread(&tc_typecode, 1, 1, fd);
    assert(n_bytes == 1);

    printf("first typecode 0x%x\n", tc_typecode);

    switch(tc_typecode){
        case TC_ENUM:
            Py_INCREF(Py_None);
            return Py_None;
        case TC_ARRAY:
            return parse_tc_array(fd, handles);
        case TC_CLASS:
            Py_INCREF(Py_None);
            return Py_None;
        case TC_LONGSTRING:
            return parse_tc_longstring(fd, handles, NULL);
        case TC_STRING:
            return parse_tc_shortstring(fd, handles, NULL);
        case TC_OBJECT:
            return parse_tc_object(fd, handles);
        case TC_PROXYCLASSDESC:
            return Py_None; /* is this necessary if it doesn't hold any data? */
        case TC_CLASSDESC:
            return parse_tc_classdesc(fd, handles, type);
            Py_INCREF(Py_None);
            return Py_None;
        case TC_NULL:
            Py_INCREF(Py_None);
            return Py_None;
        case TC_REFERENCE: {
            uint32_t handle;
            n_bytes = fread(&handle, 1, 4, fd);
            assert(n_bytes == 4);

            if (little_endian){
                uint32_reverse_bytes(handle);
            }

            JavaType_Type *ob;
            ob = Handles_Find(handles, handle);
            assert(ob != NULL);

            switch(ob->jt_type){
                case TC_CLASSDESC:
                    return get_values_class_desc(fd, handles, ob);
                case TC_STRING:
                    return PyUnicode_FromString(ob->string);
                default:
                    fprintf(stderr, "Reference type not yet implemented.");
            }

        }

    }
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
get_value(FILE *fd, Handles *handles, char tc_num)
{
    /* * Use the field descriptor to get the value from
     * the stream.
     * 
     * arguments
     * ---------
     *     fd: file descriptor
     *     _tc: type code :: possible changing this to file descriptor
     *                    :: but I don't know what I'm going to return 
     *                    :: from the function
     * 
     * */
    PyObject *ob; /* return object */
    size_t n_bytes; /* number of bytes read from the stream */

    switch(tc_num){
        case 'B': {

            /* * Read 1 unsigned byte from the stream
             * 
             * */
            uint8_t value;
            n_bytes = fread(&value, 1, 1, fd);
            assert(n_bytes == sizeof(uint8_t));

            ob = PyBytes_FromStringAndSize((char *)&value, 1);
            break;

        }
        case 'C': {
            /*  
             * Read java char size {2 bytes} from the stream.
             * 
             * */  

            char value[3];
            value[2] = 0;
            n_bytes = fread(value + 1, 1, 1, fd);
            assert(n_bytes == 1);
            n_bytes = fread(value, 1, 1, fd);
            assert(n_bytes == 1);
            ob = PyUnicode_FromString(value);
            break;

        }
        case 'D': {
            /* read 8 bytes from the stream and convert them to
             * little endian if needed
             * */
            double value;
            n_bytes = fread(&value, 1, 8, fd);
            assert(n_bytes == 8);

            if (little_endian)  {

                double copy = value;
                char *ptr = (char *)&value;
                char *cpy = (char *)&copy;

                ptr[7] = cpy[0];
                ptr[6] = cpy[1];
                ptr[5] = cpy[2];
                ptr[4] = cpy[3];
                ptr[3] = cpy[4];
                ptr[2] = cpy[5];
                ptr[1] = cpy[6];
                ptr[0] = cpy[7];

            }

            ob = PyFloat_FromDouble(value);
            break;
        }
        case 'F': {

            float value;
            n_bytes = fread(&value, 1, 4, fd);
            assert(n_bytes == 4);

            if (little_endian){

                float copy = value;

                char *cpy = (char *)&copy;
                char *ptr = (char *)&value;
                /*  */

                ptr[0] = cpy[3];
                ptr[1] = cpy[2];
                ptr[2] = cpy[1];
                ptr[3] = cpy[0]; 

            }

            ob = PyFloat_FromDouble((double)value);

            break;

        }
        case 'I': {
            /* 4 byte integer */
            
            int32_t value;
            n_bytes = fread(&value, 1, 4, fd);
            assert(n_bytes == 4);

            if (little_endian)
                uint32_reverse_bytes(value);

            ob = PyLong_FromLong((long)value);

            break;
        }
        case 'J': {

            /* 8 byte long */
            
            int64_t value;
            n_bytes = fread(&value, 1, 8, fd);
            assert(n_bytes == 8);

            if (little_endian)
                uint64_reverse_bytes(value);

            ob = PyLong_FromLongLong((long long)value);
            break;

        }
        case 'S': {

            int16_t value;
            n_bytes = fread(&value, 1, 2, fd);
            assert(n_bytes == 2);

            if (little_endian)
                uint16_reverse_bytes(value);

            ob = PyLong_FromLong((long)value);

            break;
        }
        case 'Z': {

            uint8_t value;
            n_bytes = fread(&value, 1, 1, fd);
            assert(n_bytes == 1);
            assert(value == 1 || value == 0);

            ob = PyBool_FromLong((long)value);

            break;
        }
        case '[':
        case 'L':
            ob = parse_stream(fd, handles, NULL);
            break;
        default:
            fprintf(stderr, "Not Implemented for typecode: %c. line(%d) "
                    "file(%s)", tc_num, __LINE__, __FILE__);
            exit(1);
    }
    return ob;
}

static JavaType_Type *
get_field_descriptor(FILE *fd, Handles *handles)
{
    JavaType_Type *field;

    size_t n_bytes;
    char field_tc;
    uint16_t fieldname_len;
    char *fieldname;
    char *classname;

    n_bytes = fread(&field_tc, 1, 1, fd);
    assert(n_bytes == 1);
    
    field = (JavaType_Type *)malloc(sizeof(JavaType_Type));
    assert(field != NULL);

    n_bytes = fread(&fieldname_len, 1, 2, fd);
    assert(n_bytes == n_bytes);

    if (little_endian){
        uint16_reverse_bytes(fieldname_len);
    }

    fieldname = (char *)malloc(fieldname_len + 1);
    assert(fieldname != NULL);
    fieldname[fieldname_len] = 0;
    n_bytes = fread(fieldname, 1, fieldname_len, fd);
    assert(n_bytes == fieldname_len);

    field->fieldname = fieldname;

    switch(field_tc){
        case '[':
        case 'L': {
            char classname_tc;
            n_bytes = fread(&classname_tc, 1, 1, fd); 
            assert(n_bytes == 1);
            printf("typecode = 0x%x\n", classname_tc);
            assert(classname_tc == TC_STRING 
            || classname_tc == TC_LONGSTRING 
            || classname_tc == TC_REFERENCE);
            PyObject *str;
            classname = NULL;
            if (classname_tc == TC_STRING) {
                str = parse_tc_shortstring(fd, handles, classname);
                Py_DECREF(str);
            }
            else if (classname_tc == TC_LONGSTRING) {
                str = parse_tc_longstring(fd, handles, classname);
                Py_DECREF(str);

            } else if (classname_tc == TC_REFERENCE) {
                JavaType_Type *ref_string;
                uint32_t handle;

                n_bytes = fread(&handle, 1, 4, fd);
                assert(n_bytes == 4);

                if (little_endian) {
                    uint32_reverse_bytes(handle);
                }

                ref_string = Handles_Find(handles, handle);
                assert(ref_string != NULL);
                assert(ref_string->jt_type == TC_STRING || ref_string->jt_type == TC_LONGSTRING);
                
                classname = ref_string->string;
            }

            field->classname = classname;
            field->obj_typecode = field_tc;
            field->is_object = 1;
            field->jt_type = field_tc;
            break;

        }
        case 'B':
        case 'C':
        case 'D':
        case 'F':
        case 'I':
        case 'J':
        case 'S':
        case 'Z': {
            field->prim_typecode = field_tc;
            field->jt_type = field_tc;
            field->is_primitive = 1;
            break;
        }
        default:
            fprintf(stderr, "Error. Unknown field typecode\n");
    }

    return field;
}

static PyObject *
get_values_class_desc(FILE *fd, Handles *handles, JavaType_Type *class_desc)
{
    JavaType_Type *field;
    PyObject *data;
    PyObject *value;

    data = PyDict_New();
    size_t i;
    for (i = 0; i < class_desc->n_fields; i++) {
        field = class_desc->fields[i];
        value = get_value(fd, handles, field->jt_type);

        PyDict_SetItemString(data, field->fieldname, value);
        Py_DECREF(value); /* dictionary owns the value */

    } 
    return data;
}

static PyObject *
parse_tc_object(FILE *fd, Handles *handles)
{
    
    JavaType_Type *ob;
    JavaType_Type *class_desc;
    PyObject *data;
    char next_typecode;
    size_t n_bytes;

    next_typecode = fgetc(fd);
    ob = JavaType_New(TC_OBJECT);
    

    switch(next_typecode){
        case TC_REFERENCE:
        case TC_CLASSDESC: {
            PyObject *__py_ob;
            if (next_typecode == TC_CLASSDESC) {
                class_desc = JavaType_New(next_typecode);
                __py_ob = parse_tc_classdesc(fd, handles, class_desc);
                assert(__py_ob == Py_None);
                Py_DECREF(__py_ob);
            }
            else {
                uint32_t handle;
                n_bytes = fread(&handle, 1, 4, fd);
                assert(n_bytes == 4);

                if (little_endian) {
                    uint32_reverse_bytes(handle);
                }
                class_desc = Handles_Find(handles, handle);
                assert(class_desc != NULL);
                assert(class_desc->jt_type == TC_CLASSDESC);
            }
            ob->class_descriptor = class_desc; 
            Handles_Append(handles, ob);

            data = get_values_class_desc(fd, handles, ob->class_descriptor);
            break;
        }
        default:
            fprintf(stderr, "Typecode 0x%x not implemented\n", next_typecode);
            exit(1);
            Py_INCREF(Py_None);
            data = Py_None;
    }

    return data;

}

static PyObject *
parse_tc_string(FILE *fd, Handles *handles, char *dest, size_t length)
{
    JavaType_Type *str;
    size_t n_bytes;

    str = JavaType_New(TC_STRING);

    char *string = (char *)malloc(length + 1);
    string[length] = 0;
    
    str->string = string;
    str->n_chars = length;

    Handles_Append(handles, str);

    n_bytes = fread(string, sizeof(char), length, fd);
    assert(n_bytes == length);

    return PyUnicode_FromString(string);
}

static PyObject *
parse_tc_longstring(FILE *fd, Handles *handles, char *dest)
{
    uint64_t len;
    size_t n_bytes;

    n_bytes = fread(&len, 1, 8, fd);
    if (little_endian)
        uint64_reverse_bytes(len);
    assert(n_bytes == 8);


    return parse_tc_string(fd, handles, dest, (size_t)len);
}

static PyObject *
parse_tc_shortstring(FILE *fd, Handles *handles, char *dest)
{
    uint16_t len;
    size_t n_bytes;
 
    n_bytes = fread(&len, 1, 2, fd);
    if (little_endian){
        uint16_reverse_bytes(len);
    }
    assert(n_bytes == 2);

    return parse_tc_string(fd, handles, dest, (size_t)len);
}

static PyObject *
parse_tc_classdesc(FILE *fd, Handles *handles, JavaType_Type *type)
{
    size_t n_bytes;

    //JavaType_Type *handle_obj;
    uint16_t classname_len;
    char *classname;
    uint64_t suid;
    struct {
        uint8_t sc_write_method:1;
        uint8_t sc_serializable:1;
        uint8_t sc_externalizable:1;
        uint8_t sc_block_data:1;
        uint8_t sc_enum:4;
    } flags;
    uint16_t n_fields;
    JavaType_Type **fields;
    JavaType_Type *super;
    JavaType_Type *class_annotation;
    PyObject *__py_class_annotation;
    PyObject *__py_super;

    /* classname size */
    n_bytes = fread(&classname_len, 1, 2, fd);
    assert(n_bytes == 2);

    if (little_endian){
        uint16_reverse_bytes(classname_len);
    }

    /* classname */
    classname = (char *)malloc(classname_len + 1);
    assert(classname != NULL);
    classname[classname_len] = 0;
    n_bytes = fread(classname, 1, classname_len, fd);
    assert(n_bytes == classname_len);

    /* serial version uid */
    n_bytes = fread(&suid, 1, 8, fd);
    assert(n_bytes == 8);

    if (little_endian){
        uint16_reverse_bytes(suid);
    }
    
    type->classname = classname;
    type->serial_version_uid = suid;
    /*****NEW HANDLE NEEDS TO GO IN HERE BEFORE THE FIELDS*******/
    Handles_Append(handles, type);

    /* flags */
    n_bytes = fread(&flags, 1, 1, fd);
    assert(n_bytes == 1);

    /* number of fields */
    n_bytes = fread(&n_fields, 1, 2, fd);
    assert(n_bytes == 2);

    if (little_endian){
        uint16_reverse_bytes(n_fields);
    }
    fields = (JavaType_Type **)malloc(sizeof(void *)*n_fields);
    assert(fields != NULL);

    size_t i;
    for (i = 0; i < n_fields; i++){
        printf("field descriptor %zu\n", i);
        fields[i] = get_field_descriptor(fd, handles);
    }

    type->n_fields = n_fields;
    type->fields = fields;
    memcpy(&type->flags, &flags, 1);
    
    char c;
    printf("reached class descriptor end.\n");

    c = fgetc(fd);
    assert(c == ungetc(c, fd));
    class_annotation = JavaType_New(c);
    __py_class_annotation = parse_stream(fd, handles, class_annotation);
    if (__py_class_annotation != Py_None){

    }
    c = fgetc(fd);
    assert(c == ungetc(c, fd));
    super = JavaType_New(c);
    __py_super = parse_stream(fd, handles, super);
    if (__py_super != Py_None){

    }

    
 
    /* need to return values here */
    Py_INCREF(Py_None);
    return Py_None;

}

static PyObject *
parse_tc_array(FILE *fd, Handles *handles)
{
    /** Parse stream that starts with TC_ARRAY
     * 
     * arguments
     * ---------
     *     fd: memory buffer
     *     handles: references
     *     type: structure containing information about that ????
     *           ?????? WTF do I do here?????
     * */
    JavaType_Type *array;
    JavaType_Type *class_desc;
    PyObject *python_array;
    size_t n_bytes;
    uint32_t n_elements;
    char array_type;
    char next_type;
    char *classname;
    
    next_type = fgetc(fd);
    assert(next_type != EOF);

    array = JavaType_New(TC_ARRAY);
    if (next_type == TC_REFERENCE){
        uint32_t handle;
        size_t n_bytes;
        n_bytes = fread(&handle, 1, 4, fd);
        assert(n_bytes == 4);
        if (little_endian){
            uint32_reverse_bytes(handle);
        }
        class_desc = Handles_Find(handles, handle);
        assert(class_desc != NULL);
        printf("reference\n");
    }
    else if(next_type == TC_CLASSDESC){ /* next_type == TC_CLASSDESC|TC_PROXYCLASSDESC|TC_REFERENCE */
        class_desc = JavaType_New(TC_CLASSDESC);
        /* Arrays have no fields, so this should return no data */
        PyObject *data;
        data = parse_tc_classdesc(fd, handles, class_desc);
        assert(data == Py_None);
        Py_DECREF(data);
    }
    else {
        fprintf(stderr, "Not implemeneted for typecode 0x%x", next_type);
        exit(1);
    }
    array->class_descriptor = class_desc;
    Handles_Append(handles, array);

    classname = class_desc->classname;
    printf("classname = %s\n", classname);
    assert(classname[0] == '[');

    n_bytes = fread(&n_elements, 1, 4, fd);
    assert(n_bytes == 4);

    if (little_endian){
        uint32_reverse_bytes(n_elements);
    }
    
    /* needs to be decoupled incase a reference is used to get the data */
    PyObject *element;
    array_type = classname[1];
    switch(array_type){
        case 'L':
        case '[': {
            Py_ssize_t i;
            python_array = PyList_New(0);
            for (i = 0; i < (Py_ssize_t)n_elements; i++) {
                printf("loop iteration %zu\n", (size_t)i);
                element = parse_stream(fd, handles, NULL);
                if (element != Py_None){
                    PyList_Append(python_array, element);
                    Py_INCREF(element);
                }
            }
            break;
        }
        case 'B':
        case 'C':
        case 'D':
        case 'F':
        case 'I':
        case 'J':
        case 'S':
        case 'Z': {
            Py_ssize_t i;
            python_array = PyList_New(n_elements);
            for (i = 0; i < (Py_ssize_t)n_elements; i++){
                PyList_SetItem(python_array, i, get_value(fd, handles, array_type));
            }
            break;
        }
    }
    return python_array;
}



static PyObject *
__test_parse_primitive_array(PyObject *self, PyObject *args)
{  
    int i = 0x0001;
    little_endian = *((char *)&i);

    char *filename;
    uint32_t stream_head;
    size_t n_bytes;
    FILE *fd; 

    if (!PyArg_ParseTuple(args, "s", &filename)) {
        return Py_None;
    }

    fd = fopen(filename, "rb");
    assert(fd != NULL);

    n_bytes = fread(&stream_head, 1, 4, fd);
    assert(n_bytes == 4 && stream_head == uint32_switch(0xaced0005));

    Handles *handles = Handles_New(DEFAULT_REFERENCE_SIZE);
    assert(handles != NULL);

    return parse_stream(fd, handles, NULL);
     
}

static PyObject *
__test_parse_class_descriptor(PyObject *self, PyObject *args)
{
    int i = 0x0001;
    little_endian = *((char *)&i);
    
 
    char *filename;
    uint32_t stream_head;
    size_t n_bytes;
    FILE *fd; 

    if (!PyArg_ParseTuple(args, "s", &filename)) {
        return Py_None;
    }

    fd = fopen(filename, "rb");
    assert(fd != NULL);

    n_bytes = fread(&stream_head, 1, 4, fd);
    assert(n_bytes == 4 && stream_head == uint32_switch(0xaced0005));

    Handles *handles = Handles_New(DEFAULT_REFERENCE_SIZE);
    assert(handles != NULL);
 
    return parse_stream(fd, handles, NULL);

}


static PyMethodDef ReaderMethods[] = {
    {"stream_read", java_stream_reader, METH_VARARGS, "read serialized java stream data"},
    {"_test_parse_primitive_array", __test_parse_primitive_array, METH_VARARGS, "test case for primitive type integer array"},
    {"_test_parse_class_descriptor", __test_parse_class_descriptor, METH_VARARGS, "test case for class descriptor"},
 
    {NULL, NULL, 0, NULL}
};

static struct PyModuleDef jsoreadermodule = {
    PyModuleDef_HEAD_INIT, 
    "jso_reader",
    NULL,
    -1, 
    ReaderMethods

};

PyMODINIT_FUNC
PyInit_jso_reader(void)
{
    return PyModule_Create(&jsoreadermodule);
}