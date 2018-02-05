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

/* static global - set where ever the reader entry point is */
static uint8_t little_endian;
static PyObject *collection_value;



static PyObject *
java_stream_reader(PyObject *self, PyObject *args)
{
    /* *Entry point for reading from a java serialized object stream
     * 
     * arguments
     * --------
     * */

    assert(sizeof(float) == 4);
    assert(sizeof(double) == 8);
    assert(sizeof(uint32_t) == 4);
    assert(sizeof(uint16_t) == 2);
    assert(sizeof(long long) == 8);


    /* test endianness big_endian -> 0x0001, little_endian ->0x0100*/
    uint8_t i = 0x0001;
    little_endian = *((char *)&i);


    collection_value = PyUnicode_FromString("value");


    char *filename;

    if (!PyArg_ParseTuple(args, "s", &filename)) {
        Py_INCREF(Py_None);
        return Py_None;
    }

    FILE *fd = fopen(filename, "rb");

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

    PyObject *data;

    data = parse_stream(fd, handles, NULL);
    
    fclose(fd);
    // Handles_Destruct(handles);
    handles = NULL;
    free(handles);
    return data;
}

static PyObject *
parse_stream(FILE *fd, Handles *handles, JavaType_Type *type)
{


    size_t n_bytes;
    char tc_typecode;

    n_bytes = fread(&tc_typecode, 1, 1, fd);
    assert(n_bytes == 1);
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

        parse_tc_classdesc(fd, handles, type); 
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
    size_t n_bytes;
    uint32_t handle;
    n_bytes = fread(&handle, 1, 4, fd);
    assert(n_bytes == 4);

    if (little_endian){
        uint32_reverse_bytes(handle);
    }

    assert(handle >= 0x7e0000 && handle < handles->next_handle);

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
        fprintf(stderr, "NOT TYPECODE IMPLEMENTATION: 0x%x, %d\n", obj->jt_type, __LINE__);
        exit(1); 
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

    uint64_t len;
    size_t n_bytes;

    n_bytes = fread(&len, 1, 8, fd);
    assert(n_bytes == 8);
    if (little_endian) {
        uint64_reverse_bytes(len);
    }

    return parse_tc_string(fd, handles, dest, (size_t)len);
}

static PyObject *
parse_tc_shortstring(FILE *fd, Handles *handles, char *dest)
{

    uint16_t len;
    size_t n_bytes;
 
    n_bytes = fread(&len, 1, 2, fd);
    assert(n_bytes == 2);
    if (little_endian){
        uint16_reverse_bytes(len);
    }

    return parse_tc_string(fd, handles, dest, (size_t)len);
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
    
    // if (tc_num == 'B'){
    //     /**java byte. 
    //      * 
    //      * type: 8-bit signed two's complement
    //      * range: -128 to 127
    //      * */
    //     int8_t value;
    //     n_bytes = fread(&value, 1, 1, fd);
    //     assert(-128 <= value && value <= 127 && n_bytes == 1);
    //     ob = PyLong_FromLong((long)value);
    // }
    // else if (tc_num == 'C'){
    //     /** Java char. 
    //      * 
    //      * char: The char data type is a single 16-bit 
    //      *       Unicode character. It has a minimum value 
    //      *       of '\u0000' (or 0) and a maximum value 
    //      *       of '\uffff' (or 65,535 inclusive).
    //      */
    //     uint32_t value;
    //     n_bytes = fread(&value, 1, 2, fd);
    //     if (little_endian) {
    //         uint16_reverse_bytes(n_bytes);
    //     }
    //     assert(0 <= value && value <= 65353 && n_bytes == 2);
    //     ob = PyUnicode_FromStringAndSize((char *)&value, 2);
    // }


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


            uint32_t value;
            n_bytes = fread(&value, 1, 2, fd);
            if (little_endian) {
                uint16_reverse_bytes(n_bytes);
            }
            assert(0 <= value && value <= 65353 && n_bytes == 2);
            ob = PyUnicode_FromStringAndSize((char *)&value, 2);
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
    uint16_t fieldname_len;
    char *fieldname;
    char *classname;

    n_bytes = fread(&field_tc, 1, 1, fd);
    assert(n_bytes == 1 && strchr("BCDFIJSZL[", field_tc) != NULL);
    
    field = JavaType_New(0);
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

            } else if (classname_tc == TC_REFERENCE) {
                JavaType_Type *ref_string = NULL;
                uint32_t handle;

                n_bytes = fread(&handle, 1, 4, fd);
                assert(n_bytes == 4);

                if (little_endian) {
                    uint32_reverse_bytes(handle);
                }
                assert(0x7e0000 <= handle && handle < handles->next_handle);

                ref_string = Handles_Find(handles, handle);
                assert(ref_string != NULL);
                assert(ref_string->jt_type == TC_STRING || ref_string->jt_type == TC_LONGSTRING);
                printf("classname from ref: %s\n", ref_string->string);
                classname = ref_string->string;

            }
            printf("classname: %s\n", field->classname);
            field->classname = classname;
            printf("classname: %s\n\n", field->classname);
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
    char sc_externalizable = class_desc->flags.sc_externalizable;
    char sc_block_data = class_desc->flags.sc_block_data;
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
    size_t n_bytes;

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
    JavaType_Type **fields = NULL;

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
    printf("classname: %s\n", type->classname);
    type->classname = classname;
    type->string = classname;

    printf("classname: %s\n\n", type->classname); 
    printf("string: %s\n\n", type->string); 
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
        fields[i] = get_field_descriptor(fd, handles);

    }

    type->n_fields = n_fields;
    type->fields = fields;
    memcpy(&type->flags, &flags, 1);
    
    char c;

    /* PLACEHOLDER FOR CLASS ANNOTATIONS */
    assert(fgetc(fd) == TC_ENDBLOCKDATA);


    c = fgetc(fd);

    if (c == TC_REFERENCE) {

        uint32_t handle;
        n_bytes = fread(&handle, 1, 4, fd);
        assert(n_bytes == 4);
        if (little_endian) {
            uint32_reverse_bytes(handle);
        }
        JavaType_Type *ref;

        ref = Handles_Find(handles, handle);
        assert(ref != NULL);
        type->super = ref;
        ref->ref_count++;
    }
    else if (c == TC_PROXYCLASSDESC) {
        printf("TC_PROXYCLASSDESC not implemented\n");
        type->super = NULL;
    }
    else if (c == TC_CLASSDESC) {
        type->super = JavaType_New(c);
        parse_tc_classdesc(fd, handles, type->super);
    }
    else {
        type->super = NULL;
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
                element = parse_stream(fd, handles, NULL);
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

    size_t n_bytes;
    uint32_t size;
    unsigned char first_byte;
    PyObject *list;
    PyObject *element;

    assert(class_desc->flags.sc_write_method == 1);

    n_bytes = fread(&first_byte, 1, 1, fd);
    assert(n_bytes == 1);

    assert(first_byte == 4);

    /* LinkedList writeObject writes a java int to the stream for the size of the list */
    n_bytes = fread(&size, 1, 4, fd);
    assert(n_bytes == 4);

    if (little_endian){
        uint32_reverse_bytes(size);
    }

    list = PyList_New(size);
    assert(list != NULL);

    size_t i;
    for (i = 0; i < size; i++){
        element = parse_stream(fd, handles, NULL);
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

    size_t n_bytes;
    uint32_t buckets;
    uint32_t size;
    unsigned char first_byte;
    PyObject *dict;
    PyObject *item;
    PyObject *key;

    assert(strncmp(class_desc->classname, "java.util.HashMap", 17) == 0);

    n_bytes = fread(&first_byte, 1, 1, fd);
    assert(n_bytes == 1);
    assert(first_byte == 8);

    n_bytes = fread(&buckets, 1, sizeof(buckets), fd);
    assert(n_bytes == sizeof(buckets));

    n_bytes = fread(&size, 1, sizeof(size), fd);
    assert(n_bytes == sizeof(size));

    if (little_endian){
        uint32_reverse_bytes(buckets);
        uint32_reverse_bytes(size);
    }

    assert(size < buckets);

    dict = PyDict_New();
    size_t i;
    for (i = 0; i < size; i++){

        key = parse_stream(fd, handles, NULL);

        item = parse_stream(fd, handles, NULL);

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

    size_t n_bytes;
    uint8_t first_byte;
    uint32_t capacity;
    uint32_t size;
    float load_factor;
    PyObject *list;
    PyObject *element;

    n_bytes = fread(&first_byte, 1, 1, fd);
    assert(n_bytes == 1);
    assert(first_byte == 12); /* sizeof(int) + sizeof(float) + sizeof(int) */

    n_bytes = fread(&capacity, 1, 4, fd);
    assert(n_bytes == 4);

    n_bytes = fread(&load_factor, 1, 4, fd);
    assert(n_bytes == 4);

    n_bytes = fread(&size, 1, 4, fd);
    assert(n_bytes == 4);

    if (little_endian) {
        uint32_reverse_bytes(capacity);
        uint32_reverse_bytes(size);
        float copy = load_factor;
        char *ptr = (char *)&load_factor;
        char *cpy = (char *)&copy;
        ptr[3] = cpy[0];
        ptr[2] = cpy[1];
        ptr[1] = cpy[2];
        ptr[0] = cpy[3];
    }

    list = PyList_New(size);

    size_t i;
    for (i = 0; i < size; i++) {
        element = parse_stream(fd, handles, NULL);
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

    size_t n_bytes;
    uint32_t size;
    unsigned char first_byte;
    PyObject *list;
    PyObject *element;

    assert(class_desc->flags.sc_write_method == 1);

    n_bytes = fread(&first_byte, 1, 1, fd);
    assert(n_bytes == 1);
    assert(first_byte == 4);

    /* LinkedList writeObject writes a java int to the stream for the size of the list */
    n_bytes = fread(&size, 1, 4, fd);
    assert(n_bytes == 4);

    if (little_endian){
        uint32_reverse_bytes(size);
    }

    list = PyList_New(size - 1);
    assert(list != NULL);

    size_t i;
    for (i = 0; i < size - 1; i++){
        element = parse_stream(fd, handles, NULL);
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