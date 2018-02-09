/** Java Stream Reader. This module was written with the intention of 
 * reading a java byte stream of objects that implement the 
 * java.io.serializable interface. Classes that override the write object method
 * need to have special cases written so that the object can be read 
 * back in from the stream. This is the case with several non Abstract
 * implementations of the java.util.Collections<E> interface. 
 * 
 * Classes with 'writeObject(ob)' support
 * ---------------------------------
 *     java.util.ArrayDeque * not implemeneted yet *
 *     java.util.ArrayList
 *     java.util.BitSet * works fine down the regular path, but the data comes out weird *
 *     java.util.Calendar * not implemented yet *
 *     java.util.Collections * not implemented yet *
 *     java.util.Date * not implemented yet *
 *     java.util.EnumMap * not implemented yet *
 *     java.util.HashMap * not implemented yet *
 *     java.util.HashSet *not implemented yet *
 *     java.util.Hashtable * not implemented yet *
 *     java.util.IdentityHashMap * not implemented yet *
 *     java.util.LinkedList
 *     java.util.PriorityQueue
 * 
 * */


#include "jso_reader.h"
#include <time.h>

/* static global - set where ever the reader entry point is */
static uint8_t little_endian;
static PyObject *collection_value;

 

static PyObject *
java_stream_reader(PyObject *self, PyObject *args)
{
    /* these assertions can be removed once I know more about
     * handling different system architecture */
    assert(sizeof(float) == 4);
    assert(sizeof(double) == 8);
    assert(sizeof(uint32_t) == 4);
    assert(sizeof(uint16_t) == 2);
    assert(sizeof(long long) == 8);


    /* test endianness big_endian -> 0x0001, little_endian ->0x0100*/
    uint8_t i = 0x0001;
    little_endian = *((char *)&i);

    /* get rid of this. I don't like this at all */
    collection_value = PyUnicode_FromString("value");

    char *filename;

    if (!PyArg_ParseTuple(args, "s", &filename)) {
        Py_INCREF(Py_None);
        return Py_None;
    }

    FILE *fd = fopen(filename, "rb");

    uint16_t magic_number = get_unsigned_short(fd);
    uint16_t version = get_unsigned_short(fd);

    /* validate magic number and version are correct  */
    assert(magic_number == 0xaced);
    assert(version == 0x0005);

    Handles *handles = Handles_New(DEFAULT_REFERENCE_SIZE);

    PyObject *data;

    data = parse_stream(fd, handles);
    
    fclose(fd);
    // Handles_Destruct(handles);
    handles = NULL;
    free(handles);
    return data;
}

static PyObject *
read_stream(const char *stream, size_t buffer_length)
{
    FILE *fd;
    PyObject *data;
    Handles *handles;
    uint16_t magic_number;
    uint16_t version;

    fd = fmemopen((void *)stream, buffer_length, "r");
    /* validate stream header */
    magic_number = get_unsigned_short(fd);
    version = get_unsigned_short(fd);
    if (magic_number != 0xaced || version != 0x0005) {
        fprintf(stderr, "Invalid stream header for java object serialization"
        " stream protocol.\n\t First 8 bytes must read 0xaced0005.\n\t"
        "First 8 bytes instead read 0x%x%x", magic_number, version);
        exit(EXIT_FAILURE);
    }

    handles = Handles_New(100); /* change to estimate based on 'buffer_length' */
    data = parse_stream(fd, handles);

    return data;

}

static PyObject *
parse_stream(FILE *fd, Handles *handles)
{
    char tc_typecode;

    tc_typecode = get_and_validate_stream_typecode(fd);
    PyObject *ob = Py_None;

    if (tc_typecode == TC_ARRAY) {
        ob = parse_tc_array(fd, handles);
    }
    else if (tc_typecode == TC_LONGSTRING){
        /* the NULL field is so a char buffer can be passed in 
        and the original string can be returned along with the 
        PyUnicode object */
        ob = parse_tc_longstring(fd, handles, NULL);
    }
    else if (tc_typecode == TC_STRING) {
        /* the NULL field is so a char buffer can be passed in 
        and the original string can be returned along with the 
        PyUnicode object */
        ob = parse_tc_shortstring(fd, handles, NULL);
    }
    else if (tc_typecode == TC_OBJECT) {
        ob = parse_tc_object(fd, handles);
    }
    else if (tc_typecode == TC_CLASSDESC) {
        /* TODO: i hate the way this is written */
        JavaType_Type *type = JavaType_New(TC_CLASSDESC);
        parse_tc_classdesc(fd, handles, type);
        ob = get_values_class_desc(fd, handles, type); 
    }
    else if (tc_typecode == TC_NULL) {
        /* TODO: fix this shit. This should return null instead of Py_None and 
                 be validated by the calling function */
        ob = Py_None;
        Py_INCREF(ob);
    }
    else if (tc_typecode == TC_REFERENCE) {
        ob = parse_tc_reference(fd, handles);
    }
    else {
        fprintf(stderr, "NOT TYPECODE IMPLEMENTATION: 0x%x, %d\n", tc_typecode, __LINE__);
        exit(1);
    }

    return ob;
}

static PyObject *
parse_tc_reference(FILE *fd, Handles *handles)
{
    /* * parse_tc_refernce parses a stream following a typecode of 0x77.
     * the first position in the stream should be the java int (4 bytes) 
     * that represents the reference in the stream. 
     * 
     * returns
     * -------
     *     PyObject *ob: pyobject value that the stream is referencing. 
     * */
    uint32_t handle;

    handle = get_handle(fd);

    JavaType_Type *obj = NULL;
    PyObject *ob;
    obj = Handles_Find(handles, handle);
    assert(obj != NULL);
    
    if (obj->jt_type == TC_CLASSDESC) {
        assert(obj->classname != NULL);
        ob = get_values_class_desc(fd, handles, obj);
    }
    else if (obj->jt_type == TC_STRING) {
        assert(obj->string != NULL);
        assert(obj->n_chars != 0);
        ob = PyUnicode_FromString(obj->string);
    }
    else if (obj->jt_type == TC_OBJECT) {
        assert(obj->value != NULL);
        assert(obj->class_descriptor != NULL);
        ob = obj->value;
    }
    else {
        fprintf(stderr, "NO TYPECODE IMPLEMENTATION: 0x%x, %d\n", obj->jt_type, __LINE__);
        exit(EXIT_FAILURE); 
    }

    return ob;
}

static PyObject *
parse_tc_string(FILE *fd, Handles *handles, char *dest, size_t length)
{

    JavaType_Type *str = NULL;
    size_t n_bytes;

    str = JavaType_New(TC_STRING);
    assert(str != NULL);

    char *string = (char *)malloc(length + 1);
    assert(string != NULL);
    string[length] = 0;
    
    str->string = string;
    str->n_chars = length;

    Handles_Append(handles, str);

    n_bytes = fread(string, 1, length, fd);
    assert(n_bytes == length);

    dest = string;

    return PyUnicode_FromString(string);
}

static PyObject *
parse_tc_longstring(FILE *fd, Handles *handles, char *dest)
{
    /* the casting at the botton to size_t won't work I 
     * think unless the size of a long on the machine is
     * 8 bytes, which doesn't seem to be guarenteed.
     */
    uint64_t len;

    len = get_unsigned_long_long(fd);
    return parse_tc_string(fd, handles, dest, (size_t)len);
}

static PyObject *
parse_tc_shortstring(FILE *fd, Handles *handles, char *dest)
{
    /* read 2 bytes from the stream followed by a string
     * and a python unicode object.
     * */
    return parse_tc_string(fd, handles, dest, get_size(fd));
}

static PyObject *
get_value(FILE *fd, Handles *handles, char tc_num)
{
    PyObject *ob; /* return object */
    size_t n_bytes; /* number of bytes read from the stream */

    if (tc_num == 'B'){
        uint8_t value;
        n_bytes = fread(&value, 1, 1, fd);
        assert(n_bytes == sizeof(uint8_t));

        ob = PyBytes_FromStringAndSize((char *)&value, 1);
    }
    else if (tc_num == 'C'){
        uint32_t value;
        n_bytes = fread(&value, 1, 2, fd);
        if (little_endian) {
            uint16_reverse_bytes(n_bytes);
        }
        assert(0 <= value && value <= 65353 && n_bytes == 2);
        ob = PyUnicode_FromStringAndSize((char *)&value, 2);
    }
    else if (tc_num == 'D'){
        ob = get_double_value(fd);
    }
    else if (tc_num == 'F'){
        ob = get_float_value(fd);
    }
    else if (tc_num == 'I'){
        ob = get_signed_integer_value(fd);
    }
    else if (tc_num == 'J'){
        ob = get_signed_long_value(fd);
    }
    else if (tc_num == 'S'){
        ob = get_signed_short_value(fd);
    }
    else if (tc_num == 'Z'){
        uint8_t value;
        n_bytes = fread(&value, 1, 1, fd);
        assert(n_bytes == 1);
        assert(value == 1 || value == 0);

        ob = PyBool_FromLong((long)value);
    }
    else if (tc_num == '[' || tc_num == 'L'){
        ob = parse_stream(fd, handles);
    }
    else {
        fprintf(stderr, "Not Implemented for typecode: 0x%x. line(%d) "
                    "file(%s)\n", tc_num, __LINE__, __FILE__);
        exit(1); 
    }

    return ob;
}

static JavaType_Type *
get_field_descriptor(FILE *fd, Handles *handles)
{
    JavaType_Type *field = NULL;

    size_t n_bytes;
    char field_tc;
    char *fieldname;
    char *classname;

    field_tc = get_and_validate_field_typecode(fd);
    
    field = JavaType_New(0);
    assert(field != NULL);

    fieldname = get_size_and_string(fd);

    field->fieldname = fieldname;

    switch(field_tc){
        case '[':
        case 'L': {
            char classname_tc;
            n_bytes = fread(&classname_tc, 1, 1, fd); 
            assert(n_bytes == 1);

            assert(classname_tc == TC_STRING 
            || classname_tc == TC_LONGSTRING 
            || classname_tc == TC_REFERENCE);

            PyObject *str;
            classname = NULL;
            if (classname_tc == TC_STRING) {
                str = parse_tc_shortstring(fd, handles, classname);
                Py_DECREF(str); /* I don't need these values */
            }
            else if (classname_tc == TC_LONGSTRING) {
                str = parse_tc_longstring(fd, handles, classname);
                Py_DECREF(str); /* I don't need these values */
            } 
            else if (classname_tc == TC_REFERENCE) {
                JavaType_Type *ref_string;
                uint32_t handle;

                handle = get_handle(fd);

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
            fprintf(stderr, "Error. Unknown field typecode 0x%x\n", field_tc);
            exit(1);
    }

    return field;
}

static PyObject *
get_values_class_desc(FILE *fd, Handles *handles, JavaType_Type *class_desc)
{


    char sc_write_method = class_desc->flags.sc_write_method;
    char sc_serializable = class_desc->flags.sc_serializable;
    // char sc_externalizable = class_desc->flags.sc_externalizable;
    // char sc_block_data = class_desc->flags.sc_block_data;
    char c;
    PyObject *data, *value;
    JavaType_Type *field = NULL;
    
    data = PyDict_New();

    if (sc_serializable) {
        if (class_desc->n_fields == 1 
            && !strncmp(class_desc->fields[0]->fieldname, "value", 5)
            && (!strncmp(class_desc->classname, "java.lang.Boolean", 17)
                || !strncmp(class_desc->classname, "java.lang.Byte", 14)
                || !strncmp(class_desc->classname, "java.lang.Character", 19)
                || !strncmp(class_desc->classname, "java.lang.Float", 15)
                || !strncmp(class_desc->classname, "java.lang.Integer", 17)
                || !strncmp(class_desc->classname, "java.lang.Long", 14)
                || !strncmp(class_desc->classname, "java.lang.Short", 15)
                || !strncmp(class_desc->classname, "java.lang.Double", 16)
            )){
            Py_DECREF(data); /* clear data */
             /* ref count should only be 1 from get_value */
            assert(strchr("BCDFIJSZL[", class_desc->fields[0]->jt_type) != NULL);
            return get_value(fd, handles, class_desc->fields[0]->jt_type);
        }
        else {
            size_t i;

            for (i = 0; i < class_desc->n_fields; i++) {
                field = class_desc->fields[i];

                assert(strchr("BCDFIJSZL[", field->jt_type) != NULL); 
                value = get_value(fd, handles, field->jt_type);


                PyDict_SetItemString(data, field->fieldname, value);

                Py_DECREF(value); /* dictionary owns the value */

            }
        }

        if (sc_write_method) {
            /* java.util.BitSet isn't tagged with the 0x77 Block data tag */

            if (!strncmp(class_desc->classname, "java.util.BitSet", 16)){
                PyObject *bit_set;

                Py_INCREF(data);
                bit_set = BitSet_ReadObject(fd, handles, data);
                Py_DECREF(data);

                assert(fgetc(fd) == TC_ENDBLOCKDATA); 
                if (bit_set != NULL) {
                    Py_DECREF(data); /* clear data */
                    return bit_set;
                } else {
                    fprintf(stderr, "something got fucked up with bitset restructure.\n");
                    exit(1);
                }
            }
            c = fgetc(fd);
            if (c == TC_ENDBLOCKDATA){

                /* java.util.BitSet will go down this path */
                assert(c == ungetc(c, fd));
            }
            else { /* block data from classes that override writeObject() */

                assert(c == TC_BLOCKDATA);
                data = parse_block_data(fd, handles, class_desc, data);


                c = fgetc(fd);


                ungetc(c, fd);
            }
            assert(fgetc(fd) == TC_ENDBLOCKDATA);
            return data;
        }

    }
    return data; 

}

static PyObject *
parse_tc_object(FILE *fd, Handles *handles)
{

    JavaType_Type *ob = NULL;
    JavaType_Type *class_desc = NULL;
    PyObject *data;
    char next_typecode;

    next_typecode = fgetc(fd);
    assert(next_typecode == TC_REFERENCE 
          || next_typecode == TC_PROXYCLASSDESC
          || next_typecode == TC_CLASSDESC 
          || next_typecode == TC_NULL);

    ob = JavaType_New(TC_OBJECT);

    switch(next_typecode){
        case TC_REFERENCE:
        case TC_CLASSDESC: {
            if (next_typecode == TC_CLASSDESC) {
                class_desc = JavaType_New(next_typecode);
                parse_tc_classdesc(fd, handles, class_desc);
            }
            else {
                uint32_t handle = get_handle(fd);
                class_desc = Handles_Find(handles, handle);
                assert(class_desc != NULL);
                assert(class_desc->jt_type == TC_CLASSDESC);
            }
            assert(class_desc != NULL);


            ob->class_descriptor = class_desc;
            class_desc->ref_count++;
            Handles_Append(handles, ob);

            
            data = get_values_class_desc(fd, handles, class_desc);
            Py_INCREF(data);
            ob->value = data;

            // if (ob->class_descriptor->super != NULL && PyDict_Check(data)){
            //     PyObject *super;
            //     super = get_values_class_desc(fd, handles, class_desc->super);
            //     PyDict_SetItemString(data, class_desc->classname, super);
            // }
            break;
        }
        default:
            fprintf(stderr, "Typecode 0x%x not implemented\n", next_typecode); exit(1);
    }

    return data;

}

static void
parse_tc_classdesc(FILE *fd, Handles *handles, JavaType_Type *type)
{
    assert(type != NULL);
    size_t n_bytes;

    //JavaType_Type *handle_obj;
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
    JavaType_Type **fields = NULL;

    classname = get_size_and_string(fd);
    suid = get_unsigned_long_long(fd);

    type->classname = classname;
    type->string = classname;
    type->serial_version_uid = suid;

    /*****NEW HANDLE NEEDS TO GO IN HERE BEFORE THE FIELDS*******/
    Handles_Append(handles, type);

    /* flags */
    n_bytes = fread(&flags, 1, 1, fd);
    assert(n_bytes == 1);

    /* number of fields */
    n_fields = get_size(fd);

    fields = (JavaType_Type **)malloc(sizeof(void *)*n_fields);
    assert(fields != NULL);

    size_t i;
    for (i = 0; i < n_fields; i++){
        fields[i] = get_field_descriptor(fd, handles);
    }

    type->n_fields = n_fields;
    type->fields = fields;
    memcpy(&type->flags, &flags, 1);
    
 
    /* PLACEHOLDER FOR CLASS ANNOTATIONS */
    assert(fgetc(fd) == TC_ENDBLOCKDATA);

    char c;
    c = get_and_validate_stream_typecode(fd);
    if (c == TC_REFERENCE) {

        uint32_t handle;
        
        handle = get_handle(fd);

        JavaType_Type *ref;

        ref = Handles_Find(handles, handle);
        assert(ref != NULL);
        type->super = ref;
        ref->ref_count++;
    }
    else if (c == TC_PROXYCLASSDESC) {
        printf("TC_PROXYCLASSDESC not implemented\n");
        exit(EXIT_FAILURE);
        type->super = NULL;
    }
    else if (c == TC_CLASSDESC) {
        type->super = JavaType_New(c);
        parse_tc_classdesc(fd, handles, type->super);
    }
    else if (c == TC_NULL) {
        type->super = NULL;
    }
    else {
        printf("unknown stream code 0x%x\n", c);
    }

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
    JavaType_Type *array = NULL;
    JavaType_Type *class_desc = NULL;
    PyObject *python_array;

    uint32_t n_elements;
    char array_type;
    char next_type;
    char *classname;
    
    next_type = get_and_validate_stream_typecode(fd);

    array = JavaType_New(TC_ARRAY);
    if (next_type == TC_REFERENCE){
        uint32_t handle;
        
        handle = get_handle(fd);
        class_desc = Handles_Find(handles, handle);
        assert(class_desc != NULL);
    }
    else if(next_type == TC_CLASSDESC){ /* next_type == TC_CLASSDESC|TC_PROXYCLASSDESC|TC_REFERENCE */
        class_desc = JavaType_New(TC_CLASSDESC);
        /* Arrays have no fields, so this should return no data */
        parse_tc_classdesc(fd, handles, class_desc);
    }
    else {
        fprintf(stderr, "Not implemeneted for typecode 0x%x", next_type);
        exit(1);
    }

    assert(class_desc != NULL);
    array->class_descriptor = class_desc;
    class_desc->ref_count++;
    Handles_Append(handles, array);

    classname = class_desc->classname;
    assert(classname[0] == '[');

    n_elements = get_unsigned_long(fd);

    
    /* needs to be decoupled incase a reference is used to get the data */
    PyObject *element;
    array_type = classname[1];
    switch(array_type){
        case 'L':
        case '[': {
            Py_ssize_t i;
            python_array = PyList_New(0);
            for (i = 0; i < (Py_ssize_t)n_elements; i++) {
                element = parse_stream(fd, handles);
                if (element != Py_None){
                    assert(PyList_Append(python_array, element) == 0);
                    Py_DECREF(element);

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
                PyObject *ob = get_value(fd, handles, array_type);
                assert(PyList_SetItem(python_array, i, ob) == 0);
            }
            break;
        }
    }
    return python_array;
}

static PyObject *
parse_block_data(FILE *fd, Handles *handles, JavaType_Type *class_desc, PyObject *data)
{

    /* follows the class descriptor in the values part of the 
     * stream. 
     * Object Annotation
     * -----------------
     *     [endBlockData (TC_ENDBLOCKDATA)]
     *     [contents (parse_stream)] [TC_<ANY> endBlockData]
     */
    


    assert(class_desc != NULL);
    assert(class_desc->flags.sc_write_method);

    PyObject *ob;

    ob = Py_None; /* temporary */
    if (!strncmp(class_desc->classname, "java.util.ArrayDeque", 20)
         || !strncmp(class_desc->classname, "java.util.ArrayList", 19)
         || !strncmp(class_desc->classname, "java.util.LinkedList", 19) 
    ) {
        ob = List_ReadObject(fd, handles, class_desc);
    }
    else if (!strncmp(class_desc->classname, "java.util.BitSet", 16)) {}
    else if (!strncmp(class_desc->classname, "java.util.Calendar", 18)) {}
    else if (!strncmp(class_desc->classname, "java.util.Collections", 21)) {}
    else if (!strncmp(class_desc->classname, "java.util.Date", 14)) {}
    else if (!strncmp(class_desc->classname, "java.util.EnumMap", 17)) {}
    else if (!strncmp(class_desc->classname, "java.util.HashMap", 17)) {


        ob = HashMap_ReadObject(fd, handles, class_desc);
    }
    else if (!strncmp(class_desc->classname, "java.util.HashSet", 17)) {


        ob = HashSet_ReadObject(fd, handles, class_desc);
    }
    else if (!strncmp(class_desc->classname, "java.util.Hashtable", 19)) {}
    else if (!strncmp(class_desc->classname, "java.util.IdentityHashMap", 25)) {}
    else if (!strncmp(class_desc->classname, "java.util.PriorityQueue", 23)) {
        ob = PriorityQueue_ReadObject(fd, handles, class_desc);
    }
    else {
       Py_INCREF(Py_None);
       fprintf(stderr, "classname not found %s\n", class_desc->classname); 
       exit(EXIT_FAILURE); 
    }

    return ob;
}

/* referenced functions */
static PyObject *
List_ReadObject(FILE *fd, Handles *handles, JavaType_Type *class_desc)
{

    /* assume that the default write object operation has happened and
     * the last byte read from the stream was a 0x77 (TC_BLOCKDATA)
     * and the classname is {java.util.LinkedList}
     * 
     * note
     * ----
     *     this function returns a list that needs to be propagated to
     *     the interpreter because the default return type is of type dict.
     */

    uint32_t size;
    unsigned char first_byte;
    PyObject *list;
    PyObject *element;

    assert(class_desc->flags.sc_write_method == 1);

    first_byte = get_byte(fd);
    assert(first_byte == 4);

    /* LinkedList writeObject writes a java int to the stream for the size of the list */
    size = get_unsigned_long(fd);

    list = PyList_New(size);
    assert(list != NULL);

    size_t i;
    for (i = 0; i < size; i++){
        element = parse_stream(fd, handles);
        PyList_SetItem(list, (Py_ssize_t)i, element); /* PyList 'steals' */
    }  

    return list;
}


static PyObject *
BitSet_ReadObject(FILE *fd, Handles *handles, PyObject *data) {

    /* change data to set */
    PyObject *bits, *list, *element, *bit_set, *bit;
    unsigned long l_bits;


    bits = PyUnicode_FromString("bits");
    assert(PyDict_Contains(data, bits));

    list = PyDict_GetItemString(data, "bits");
    assert(PyList_Check(list));

    /* should be a list of longs */

    bit_set = PyList_New(0);
    Py_ssize_t i;
    Py_ssize_t j;
    for (i = 0; i < PyList_Size(list); i++){
        element = PyList_GetItem(list, i);
        assert(PyLong_Check(element));
        
        l_bits = (unsigned long)PyLong_AsLong(element);

        for (j = 63; j >= 0; j--){

            if ((l_bits & 1) == 1) {

                bit = PyLong_FromLong(63 - j + i*64); 
                PyList_Append(bit_set, bit);
                Py_DECREF(bit); /* append increments the ref count, 
                but the list should own the reference */
            }
            l_bits = l_bits >> 1;
        }    
    }

    Py_DECREF(data);
    return PySet_New(bit_set);

}

static PyObject *
HashMap_ReadObject(FILE *fd, Handles *handles, JavaType_Type *class_desc)
{

    uint32_t buckets;
    uint32_t size;
    unsigned char first_byte;
    PyObject *dict;
    PyObject *item;
    PyObject *key;

    assert(strncmp(class_desc->classname, "java.util.HashMap", 17) == 0);

    first_byte = get_byte(fd);
    assert(first_byte == 8);

    buckets = get_unsigned_long(fd);
    size = get_unsigned_long(fd);

    assert(size < buckets);

    dict = PyDict_New();
    size_t i;
    for (i = 0; i < size; i++){
        key = parse_stream(fd, handles);
        item = parse_stream(fd, handles);
        PyDict_SetItem(dict, key, item);
        Py_DECREF(key);
        Py_DECREF(item);
    }

    return dict;
}

static PyObject *
HashSet_ReadObject(FILE *fd, Handles *handles, JavaType_Type *class_desc)
{

    assert(!strncmp(class_desc->classname, "java.util.HashSet", 17));
    assert(class_desc->flags.sc_write_method);

    uint8_t first_byte;
    uint32_t capacity;
    uint32_t size;
    float load_factor;
    PyObject *list;
    PyObject *element;

    first_byte = get_byte(fd);
    assert(first_byte == 12);

    capacity = get_unsigned_long(fd);
    load_factor = get_signed_float(fd);
    size = get_unsigned_long(fd);

    list = PyList_New(size);

    size_t i;
    for (i = 0; i < size; i++) {
        element = parse_stream(fd, handles);
        assert(element != NULL);
        PyList_SetItem(list, (Py_ssize_t)i, element);
    }
    
    PyObject *set;
    set = PySet_New(list);
    assert(set != NULL);

    return set;
}


static PyObject *
PriorityQueue_ReadObject(FILE *fd, Handles *handles, JavaType_Type *class_desc)
{

    /** priority queue saves max(2, size + 1) for the size when writing it to the stream, 
     * so when reading the object back in, 1 has to be subtracted from the number 
     * read to get the actual number
     * */

    assert(!strncmp(class_desc->classname, "java.util.PriorityQueue", 23));
    assert(class_desc->flags.sc_write_method == 1);

    uint32_t size;
    unsigned char first_byte;
    PyObject *list;
    PyObject *element;

    first_byte = get_byte(fd);
    assert(first_byte == 4);

    /* LinkedList writeObject writes a java int to the stream for the size of the list */
    size = get_unsigned_long(fd) - 1;

    list = PyList_New(size);
    assert(list != NULL);

    size_t i;
    for (i = 0; i < size; i++){
        element = parse_stream(fd, handles);
        PyList_SetItem(list, (Py_ssize_t)i, element);
    }  

    return list;
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

    return parse_stream(fd, handles);
     
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
 
    return parse_stream(fd, handles);

}

static char *
get_string(FILE *fd, size_t len)
{   
    /* * get_string(fd, len) reads a string of
     * bytes of length len and allocate those bytes
     * to the heap. The caller of this function will
     * be responsible for freeing the memory
     * 
     * arguments
     * ---------
     *     fd: file descriptor for the file stream
     *     len: number of bytes to read from the stream
     * 
     * returns
     * -------
     *     str: null terminated string allocated on the heap
     * */
    size_t n_bytes; 
    char *str;
    
    str = (char *)malloc(len + 1);
    assert(str != NULL);
    str[len] = 0;

    n_bytes = fread(str, sizeof(char), len, fd);

    return str;
}

static uint32_t
get_unsigned_long(FILE *fd)
{
    /* * Read a 4-byte unsigned integer from the stream.
     * 
     * */

    uint32_t value;
    size_t n_bytes;
    
    n_bytes = fread(&value, sizeof(char), sizeof(uint32_t), fd);
    assert(n_bytes == sizeof(uint32_t));

    if (little_endian){
        uint32_reverse_bytes(value);
    }
    
    return value;
}

static PyObject *
get_signed_integer_value(FILE *fd)
{
    PyObject *ob;
    long value;

    value = (long)(int32_t)get_unsigned_long(fd);
    ob = PyLong_FromLong(value);

    return ob;

}

static uint16_t
get_unsigned_short(FILE *fd)
{
    uint16_t value;
    size_t n_bytes;

    n_bytes = fread(&value, 1, sizeof(uint16_t), fd);
    assert(n_bytes == sizeof(uint16_t));
    
    if (little_endian){
        uint16_reverse_bytes(value);
    }
    return value;
}

static int16_t
get_signed_short(FILE *fd)
{
    return (int16_t)get_unsigned_short(fd);
}

static PyObject *
get_signed_short_value(FILE *fd)
{
    int16_t value;

    value = get_signed_short(fd);
    /* possible unsafe cast */
    return PyLong_FromLong((long)value);
}


static size_t 
get_size(FILE *fd)
{ 
    /* *return 2 byte unsigned int from the file stream.
     * Several objects from the stream require the size of an 
     * item prior to reading.
     * 
     * use for
     * -------
     *     classname length
     *     number of fields
     * */

    return (size_t)get_unsigned_short(fd);
}

static char *
get_size_and_string(FILE *fd)
{
    /** Reads the size of a string from the byte stream
     * and then the string itself. 
     * 
     * arguments
     * ---------
     *     fd: file descriptor/byte stream
     * 
     * returns
     * -------
     *     str: a null terminated string allocated to the heap
     * */

    size_t num_chars;
    char *str;

    num_chars = get_size(fd); 
    str = get_string(fd, num_chars);

    return str;
}

static double
get_signed_double(FILE *fd)
{
    /* * Read a signed double from the file stream. 
     * Big to Little endian conversion has to be done
     * as shown below because bitwise shifting isn't 
     * allowed for floating point values.
     * 
     * */
    double value;
    size_t n_bytes;

    n_bytes = fread(&value, 1, sizeof(double), fd);
    assert(n_bytes == sizeof(double));

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

    return value;
}

static PyObject *
get_double_value(FILE *fd)
{
    double value;
    PyObject *ob;

    value = get_signed_double(fd);
    ob = PyFloat_FromDouble(value);

    return ob;
}

static float
get_signed_float(FILE *fd)
{
    size_t n_bytes;
    float value;

    n_bytes = fread(&value, 1, sizeof(float), fd);
    assert(n_bytes == 4);

    if (little_endian){

        float copy = value;

        char *cpy = (char *)&copy;
        char *ptr = (char *)&value;

        ptr[0] = cpy[3];
        ptr[1] = cpy[2];
        ptr[2] = cpy[1];
        ptr[3] = cpy[0]; 

    }

    return value; 
}

static PyObject *
get_float_value(FILE *fd)
{
    double value;
    PyObject *ob;

    value = (double)get_signed_float(fd);
    ob = PyFloat_FromDouble(value);

    return ob;
}

static int64_t 
get_signed_long_long(FILE *fd)
{
    int64_t value;
    size_t n_bytes;

    n_bytes = fread(&value, 1, sizeof(int64_t), fd);
    assert(n_bytes == sizeof(int64_t));

    if (little_endian) {
        uint64_reverse_bytes(value);
    }

    return value;
}

static uint64_t 
get_unsigned_long_long(FILE *fd)
{

    return (uint64_t)get_signed_long_long(fd);
}

static PyObject *
get_signed_long_value(FILE *fd)
{
    int64_t value;
    PyObject *ob;

    value = get_signed_long_long(fd);
    ob = PyLong_FromLongLong(value);

    return ob;
}

static unsigned char
get_byte(FILE *fd)
{
    size_t n_bytes;
    char bbyte;

    n_bytes = fread(&bbyte, 1, 1, fd);
    assert(n_bytes == 1);

    return bbyte;
}

static unsigned char 
get_and_validate_field_typecode(FILE *fd)
{
    unsigned char typecode = get_byte(fd);
    assert(strchr("BCDFIJSZL[", typecode) != NULL);

    return typecode;
}

static unsigned char 
get_and_validate_stream_typecode(FILE *fd)
{
    unsigned char typecode = get_byte(fd);
    assert(0x70 <= typecode && typecode <= 0x7E);

    return typecode;
}

static uint32_t
get_handle(FILE *fd)
{
    uint32_t value;

    value = get_unsigned_long(fd);
    assert(0x7e0000 <= value);

    return value;
}

static uint64_t
get_serial_version_uid(FILE *fd)
{
    return 0;
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