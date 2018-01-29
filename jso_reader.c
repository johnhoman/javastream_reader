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


static uint8_t type_code;
static uint8_t little_endian;
static uint32_t next_handle;

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
    next_handle = 0x7e0000;
    printf("next handle: %x\n", next_handle);

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

    printf("magic number = 0x%x\n", magic_number);
    printf("version = 0x%x\n", version);

    PyObject *handles = PyDict_New();
    /* global reference to handles */

    /* return parser */
    PyObject *data;// = PyList_New(0);
    // PyObject *dict;
    // assert(data != NULL);

    data = parse_stream(fd, handles);

    fclose(fd);
    return data;
}

static PyObject *
parse_stream(FILE *fd, PyObject *handles)
{
    /* * This function should return a python dictionary
     *
     * */

    printf("function(&_tc_collect);\n");

    size_t n_bytes;

    n_bytes = fread(&type_code, 1, 1, fd);
    printf("typecode = %x\n", type_code);
    assert(n_bytes == 1);

    PyObject *ob;
    switch(type_code){
        case TC_OBJECT: {
            
            ob = parse_tc_object(fd, handles);
            break;
        }
        case TC_ARRAY:
        case TC_CLASS: 
        case TC_ENUM: 
        case TC_CLASSDESC:
        case TC_PROXYCLASSDESC: 
        case TC_REFERENCE: 
        case TC_STRING:
        case TC_LONGSTRING: 
            fprintf(stderr, "Not Implemented\n");
            ob = Py_None;
    }

    return ob;

}

static PyObject *
parse_tc_string(FILE *fd, PyObject *handles)
{
    size_t n_bytes_read;
    uint64_t length;
    char string_term;

    assert(type_code == TC_STRING || type_code == TC_LONGSTRING);
    printf("typecode = %x\n", type_code);
    if (type_code == TC_STRING){
        uint16_t len;
        n_bytes_read = fread(&len, 1, 2, fd);
        if (little_endian)
            uint16_reverse_bytes(len);
        assert(n_bytes_read == 2);
        printf("len = %u\n", len);
        length = (uint64_t)len;
        printf("len = %llu\n", length);
    }
    else if (type_code == TC_LONGSTRING) {
        uint64_t len;
        n_bytes_read = fread(&len, 1, 8, fd);
        if (little_endian)
            uint64_reverse_bytes(len);
        assert(n_bytes_read == 8);
        length = (uint64_t)len; 
    }
    else {
        /* force exit - Nothing implemented for this
         * case yet, but length shouldn't be zero 
         * */
        length = 0;
        assert(length != 0);
    }


    printf("string len: %llu\n", length);
    /* L<string>3b */
    /* [<string or primitive typecode>3b */
    length = length - 2;

    /* strip L */
    char _typecode;

    n_bytes_read = fread(&_typecode, 1, 1, fd);
    assert(n_bytes_read == 1 && (_typecode == 'L' || _typecode == '['));

    char string[length + 1];
    string[length] = 0;

    /* string terminating character*/
    n_bytes_read = fread(&string, sizeof(char), length, fd);
    assert(n_bytes_read == length);

    n_bytes_read = fread(&string_term, sizeof(char), 1, fd);
    assert(n_bytes_read == 1 && string_term == 0x3b);

    /* create object and store reference to it. This will only work
     * for this case and probabily needs to be changed for other
     * {
     *     "type": TC_STRING | TC_LONGSTRING,
     *     "inst": PyObject *ob
     * }
     */
    
    int i, j, k;
    PyObject *item = PyDict_New();
    PyObject *ref_handle;
    PyObject *javastring;

    javastring = PyUnicode_FromString(string);
    printf("tc string: %s\n", string);
    printf("typecode = %x\n", type_code);
    /* create dict to hold the object reference */
    i = PyDict_SetItemString(item, "type", PyLong_FromLong(type_code));
    j = PyDict_SetItemString(item, "object", javastring);  

    /* set reference to string object */
    printf("ref handle: %x\n", next_handle);
    ref_handle = PyLong_FromLong(next_handle++);
    printf("ref handle: %x\n", next_handle);
    k = PyDict_SetItem(handles, ref_handle, item);
    assert(i == 0 && j == 0 && k == 0);

    Py_INCREF(javastring);
    return javastring;
}


static PyObject *
parse_tc_object(FILE *fd, PyObject *handles)
{
    /* * parse_tc_object should return a python dictionary
     * with the data in it.
     * */

    size_t n_bytes;
    PyObject *data;
    PyObject *ref;

    n_bytes = fread(&type_code, 1, 1, fd);
    assert(n_bytes == sizeof(type_code));

    ref = PyDict_New();

    switch(type_code) {
        case TC_CLASSDESC: {
            /* TODO: Add assertions and clean up */
            PyDict_SetItemString(ref, "type", PyLong_FromLong((long)type_code));
            data = parse_tc_classdesc(fd, handles);
            assert(PyDict_SetItemString(ref, "object", data) == 0);
            PyDict_SetItem(handles, PyLong_FromLong(next_handle++), ref);
            break;
        }
        case TC_PROXYCLASSDESC:
        case TC_REFERENCE: {
            data = Py_None;
        }
    }
    


    return data;
}

static PyObject *
parse_tc_classdesc(FILE *fd, PyObject *handles)
{
    /* Test Case 1
     * -----------
     * 00000000  ac ed 00 05 73 72 00 06  50 65 72 73 6f 6e 00 00  |....sr..Person..|
     * 00000010  00 00 00 96 b4 3f 02 00  04 49 00 03 61 67 65 49  |.....?...I..ageI|
     * 00000020  00 03 73 73 6e 4c 00 09  66 69 72 73 74 4e 61 6d  |..ssnL..firstNam|
     * 00000030  65 74 00 12 4c 6a 61 76  61 2f 6c 61 6e 67 2f 53  |et..Ljava/lang/S|
     * 00000040  74 72 69 6e 67 3b 4c 00  08 6c 61 73 74 4e 61 6d  |tring;L..lastNam|
     * 00000050  65 71 00 7e 00 01 78 70  00 00 00 1b 00 00 5b 88  |eq.~..xp......[.|
     * 00000060  74 00 04 4a 61 63 6b 74  00 05 48 6f 6d 61 6e     |t..Jackt..Homan|
     * 0000006f
     * */

    assert(type_code == TC_CLASSDESC);

    size_t n_bytes;
    uint16_t classname_len; /* first two bytes at the start of the stream */
    uint16_t n_fields; /* number of fields in the class */
    uint64_t suid; /* serial version uid */

    struct {
        uint8_t sc_write_method:1;
        uint8_t sc_serializable:1;
        uint8_t sc_externalizable:1;
        uint8_t sc_block_data:1;
        uint8_t sc_enum:4;
    } flags;
 
    /* get classname size */
    n_bytes = fread(&classname_len, 1, 2, fd);
    assert(n_bytes == 2);

    if (little_endian)
        uint16_reverse_bytes(classname_len);

    /* allocate extra byte for null terminate and set to 0 */
    char classname[classname_len + 1];
    classname[classname_len] = 0;

    /* read classname from stream {classname_len bytes}*/
    n_bytes = fread(classname, 1, classname_len, fd);
    printf("classname length: %x\n", classname_len);
    printf("classname = %s\n", classname);
    assert(n_bytes == classname_len);

    /* read serial version uid {8 bytes} */
    n_bytes = fread(&suid, 1, sizeof(uint64_t), fd);
    assert(n_bytes == sizeof(uint64_t));

    if (little_endian)
        uint64_reverse_bytes(suid);
    
    /* get flags from stream {1 byte} */
    n_bytes = fread(&flags, 1, sizeof(uint8_t), fd);
    assert(n_bytes == sizeof(uint8_t));

    /* get number of fields from stream {2 bytes} */
    n_bytes = fread(&n_fields, sizeof(n_fields), 1, fd);
    assert(n_bytes == sizeof(uint8_t));

    if (little_endian)
        uint16_reverse_bytes(n_fields);

    /* field descriptors */

    PyObject *fields;
    size_t i; /* loop through fields */

    /* for debugging - delete later */
    printf("\nclass: %s\n", classname);
    printf("number fields: %hu\n", n_fields);
    

    PyObject *class_desc;
    PyObject *class_name;

    class_desc = PyDict_New();
    assert(class_desc != NULL);

    class_name = PyUnicode_FromString(classname);
    assert(PyDict_SetItemString(class_desc, "classname", class_name) == 0);
    Py_DECREF(class_name);

    /* assign handle before getting field descriptors */
    assert(set_reference(class_desc, handles, (long)type_code) == 1);

    fields = PyList_New((Py_ssize_t)n_fields);
    PyObject *field_descriptor;
    for (i = 0; i < n_fields; i++) {
        printf("i = %zu\n", i);
        field_descriptor = get_field_descriptor(fd, handles);
        assert(field_descriptor != NULL);

        assert(PyList_SetItem(fields, i, field_descriptor) == 0);
    }

    assert(PyDict_SetItemString(class_desc, "fields", fields) == 0);
    Py_DECREF(fields); /* the class descriptor dict will own the array */
    Py_INCREF(class_desc);


    /* Placeholder for classannotation and super class desc */
    n_bytes = fread(&type_code, 1, 1, fd);
    assert(n_bytes == 1 && type_code == TC_ENDBLOCKDATA);

    /* place holder for super class descriptor */
    n_bytes = fread(&type_code, 1, 1, fd);
    assert(n_bytes == 1 && type_code == TC_NULL);

    return class_desc;
}

static PyObject *
get_field_descriptor(FILE *fd, PyObject *handles)
{   /* * get the field descriptor from the stream
     * If the field descriptor is an object type, 
     * 
     * */
    static struct {
        const char *key; 
        const char *name;
        const char *class;
    } keys;

    keys.key = "type";
    keys.name = "name";
    keys.class = "class";

    PyObject *dict;
    size_t n_bytes;
    uint8_t f_typecode;
    uint16_t name_len;

    /* read field typecode {1 byte} */
    n_bytes = fread(&f_typecode, sizeof(uint8_t), 1, fd);
    assert(n_bytes == 1);
    
    /* length of member variable name {2 bytes} */
    n_bytes = fread(&name_len, 1, 2, fd);
    assert(n_bytes == sizeof(uint16_t));


    if (little_endian)
        uint16_reverse_bytes(name_len);

    char name[name_len + 1];
    name[name_len] = 0;

    n_bytes = fread(name, 1, name_len, fd);
    assert(n_bytes == name_len);
    
    /* create dictionary to return information */
    dict = PyDict_New();
    assert(dict != NULL);

    assert(PyDict_SetItemString(dict, "type", PyLong_FromLong((long)f_typecode)) == 0);
    assert(PyDict_SetItemString(dict, "name", PyUnicode_FromString(name)) == 0);

    /*  */
    switch(f_typecode){
        case '[':
        case 'L': {

            n_bytes = fread(&type_code, 1, 1, fd);
            assert(n_bytes == 1);
            assert(type_code == TC_STRING || type_code == TC_LONGSTRING || type_code == TC_REFERENCE);

            /* return dict 
             * {
             *     type: typecode,
             *     name: fieldname,
             *     classname: string
             * }
             */
            PyObject *classname = NULL;

            if (type_code == TC_STRING || type_code == TC_LONGSTRING) {
            /* parse_tc_string returns PyUnicode_FromString */
            classname = parse_tc_string(fd, handles);
            assert(classname != NULL);

            } else if (type_code == TC_REFERENCE) {
                uint32_t handle;

                n_bytes = fread(&handle, 1, 4, fd);
                assert(n_bytes == sizeof(uint32_t));

                if (little_endian)
                    uint32_reverse_bytes(handle);
                printf("handle = %x\n", handle);

                
                PyObject *h = PyLong_FromLong((long)handle);
                int adf = PyDict_Contains(handles, h);
                printf("contains: %d\n", adf);

                PyObject *ref = PyDict_GetItem(handles, PyLong_FromLong(handle));
                assert(ref != NULL);

                PyObject *type = PyDict_GetItemString(ref, "type");
                assert(type != NULL);

                long _typecode = PyLong_AsLong(type);
                printf("typecode = %ld\n", _typecode);
                assert(_typecode == TC_STRING || _typecode == TC_LONGSTRING);

                classname = PyDict_GetItemString(ref, "object");
                assert(PyUnicode_Check(classname));

            }

            assert(classname != NULL);
            assert(PyDict_SetItemString(dict, keys.class, classname) == 0);
            break;

        }
        default: {


            /* return dict 
             * {
             *     type: typecode,
             *     name: fieldname,
             * }
             * 
             * for primitive field descriptors 
             * */

            break;

        }
    }

    return dict;
}

static PyObject *
get_value(FILE *fd, char tc_num)
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
    size_t byte_count; /* number of bytes to read from the stream */

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

            printf("value = %d\n", value);
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
        default:
            fprintf(stderr, "Not Implemented for typecode: %c. line(%d) "
                    "file(%s)", tc_num, __LINE__, __FILE__);
            exit(1);
    }
    return ob;
    
}

static PyObject *
get_values_from_class_descriptor(FILE *fd, PyObject *class_desc, char type)
{
    /** Get values from class descriptor will read the class descriptor 
     * 
     * arguments
     * ---------
     *     char type: type should be one of the stream type identifiers
     *                (i.e. TC_ARRAY, TC_CLASS, TC_OBJECT, ...)
     *     PyObject class_desc: class descriptor with items fields, classname
     *                          fields: PyList containing field descriptors
     *                          classname: PyUnicode object that holds the classname
     *     FILE fd: open file descriptor to be read from
     * 
     * */


    /* this should be called at the end of a block of data (TC_ENDBLOCKDATA)*/
    char *class;
    PyObject *fields; /* list of fields */
    PyObject *classname; /*  */
    PyObject *bytes_classname;
    
    /* the class descriptor should always  */
    assert(PyDict_Contains(class_desc, PyUnicode_FromString("fields"))); /* fields should always be in here */
    assert(PyDict_Contains(class_desc, PyUnicode_FromString("classname")));

    fields = PyDict_GetItemString(class_desc, "fields");
    classname = PyDict_GetItemString(class_desc, "classname");

    /* encode to bytes */
    bytes_classname = PyUnicode_AsUTF8String(classname);
    assert(bytes_classname != NULL);

    assert(PyBytes_Check(bytes_classname) == 1);
    class = PyBytes_AsString(bytes_classname); /* class is a null terminated string */

    /* check classname for [. if [ is the first character, then 
     * it is probably a primitive array with the classname in the form
     * [<primitive typecode> */

    if (strlen(class) == 2 && PyList_Size(fields) == 0 && type == TC_ARRAY) {
        /* this branch of logic assumes that tc will be equal to '[' */
        char tc;
        char prim_type;
  
        tc = class[0];
        assert(tc == '['); /* primitive array */

        prim_type = class[1];
        assert(strchr("BCDFIJSZ", prim_type) != NULL);
          
        return get_value(fd, prim_type);     

    }

    return Py_None;


}

static PyObject *
parse_tc_array(FILE *fd, PyObject *handles)
{
    /**prior to entering this function, a type_code of 
     * 0x75 must be pulled from the stream (TC_ARRAY)
     * if the class name from the class descriptor starts with 
     * [<primitive typecode> and there are no fields, 
     * */

    /** example
     *  -------
     *           stream_head        len   [  I  serial version
     *           -----------       -----  -- -- -----------------
     * 00000000  ac ed 00 05 75 72 00 02  5b 49 4d ba 60 26 76 ea  |....ur..[IM.`&v.|
     *            uid                 array length first value
     *           -----                ------------ -----------
     * 00000010  b2 a5 02 00 00 78 70 00  00 00 0a 00 00 00 00 00  |.....xp.........|
     * 00000020  00 00 01 00 00 00 02 00  00 00 03 00 00 00 04 00  |................|
     * 00000030  00 00 05 00 00 00 06 00  00 00 07 00 00 00 08 00  |................|
     * 00000040  00 00 09                                          |...|
     * 00000043
     * **/
    size_t i; /* loop counter */
    size_t n_bytes;
    uint32_t size;

    assert(type_code == TC_ARRAY);
    
    n_bytes = fread(&type_code, 1, 1, fd);
    assert(n_bytes == 1);
    
    /* get class descriptor from stream or from handles */
    PyObject *class_desc;

    switch(type_code){
        case TC_CLASSDESC: {
            class_desc = parse_tc_classdesc(fd, handles);
            break;
        }
        case TC_PROXYCLASSDESC:
        case TC_REFERENCE:
        case TC_NULL:
            return Py_None;
            
    }

    /* assign reference */
    PyObject *ref;

    ref = wrap_class_descriptor(class_desc, (char)TC_ARRAY);
    assert(ref != NULL);

    assert(set_reference(handles, ref, (uint8_t)TC_ARRAY) == 1);

    /* read value count from stream (int)<size> {4 bytes} */
    n_bytes = fread(&size, 1, sizeof(size), fd);

    assert(n_bytes == sizeof(size));

    if (little_endian)
        uint32_reverse_bytes(size);
    assert(size >= 0);
    printf("size of array: %x\n", size);
    printf("size of array: %d\n", size);


    /* primitive arrays have a class descriptor with zero fields
     * 
     */
    PyObject *ob;
    PyObject *values;

    values = PyList_New(size);
    for (i = 0; i < (size_t)size; i++) {
        ob = get_values_from_class_descriptor(fd, class_desc, (char)TC_ARRAY);
        assert(PyList_SetItem(values, (Py_ssize_t)i, ob) == 0);
        //Py_DECREF(ob);
    }

    return values;
}

static uint8_t 
set_reference(PyObject *ob, PyObject *handles, uint8_t type)
{
    /** set an item in the handles dict to be used later by 
     * the stream to reference
     * {
     *     "type": handle,
     *     "object": PyObject *ref
     * }
     * */
    PyObject *handle_item;
    PyObject *pyob__handle;
    long handle;
    int check;

    handle_item = PyDict_New();
    check = PyDict_SetItemString(handle_item, "type", PyLong_FromLong((long)type));
    assert(check == 0);

    check = PyDict_SetItemString(handle_item, "object", ob);
    assert(check == 0);
    
    /* handle value greater than 0x007e0000 */
    handle = next_handle++;
    assert(handle >= 0x7e0000);

    pyob__handle = PyLong_FromLong(handle);
    assert(pyob__handle != NULL);

    check = PyDict_SetItem(handles, pyob__handle, handle_item);
    assert(check == 0);
    
    return 1;

}

static PyObject *
wrap_class_descriptor(PyObject *cd, char type)
{   
    /**/

    PyObject *ref;
    PyObject *obj_type;

    ref = PyDict_New();


    obj_type = PyLong_FromLong((long)type);
    assert(obj_type != NULL);

    assert(PyDict_SetItemString(ref, "type", obj_type) == 0);
    assert(PyDict_SetItemString(ref, "object", cd) == 0); 

    return ref;
}


static PyObject *
test_parse_primitive_array(PyObject *self, PyObject *args)
{  
    int i = 0x0001;
    little_endian = *((char *)&i);
    next_handle = 0x7e0000;
    
   /*
    *
    **/
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

    PyObject *handles = PyDict_New();
    assert(handles != NULL);

    n_bytes = fread(&type_code, 1, 1, fd);
    assert(n_bytes == 1);

    return parse_tc_array(fd, handles);
     
}



static PyMethodDef ReaderMethods[] = {
    {"stream_read", java_stream_reader, METH_VARARGS, "read serialized java stream data"},
    {"_test_parse_primitive_array", test_parse_primitive_array, METH_VARARGS, "test case for primitive type integer array"},
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