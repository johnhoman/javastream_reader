#include <stdlib.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "javatype.h"

typedef void * object;


Handles *
Handles_New(size_t size)
{
    Handles *handles;

    handles = (Handles *)malloc(sizeof(Handles));
    handles->stream = (StreamReference **)malloc(sizeof(void *)*size);
    handles->size = 0;
    handles->reserved = size;
    handles->next_handle = 0x7e0000;

    return handles;
}

uint8_t
Handles_Destruct(Handles *handles)
{

    size_t i;
    
    for (i = 0; i < handles->size; i++){
        JavaType_Destruct(handles->stream[i]->ob);
        handles->stream[i]->ob = NULL;
    }
    for (i = 0; i < handles->size; i++){
        free(handles->stream[i]->ob);
        handles->stream[i]->handle = 0;
        handles->stream[i] = NULL;
        free(handles->stream[i]);
    }
    return 1;
}

void 
Handles_Append(Handles *handles, JavaType_Type *ob)
{
    /* * Appends a stream object to the list of references
     * for later use. Will reallocate memory if the size 
     * attemps to grow greater than the reserved allocation
     * on the heap. 
     * */
    
    if (handles->size < handles->reserved){
        StreamReference *ref;

        ref = (StreamReference *)malloc(sizeof(StreamReference));
        assert(ref != NULL);
        ref->ob = ob;
        ref->handle = handles->next_handle++;
        handles->stream[handles->size++] = ref;

    }
    else {
        size_t new_size;

        new_size = sizeof(void *) * handles->reserved * 2;
        handles->stream = realloc(handles->stream, new_size);
        assert(handles->stream != NULL);
        handles->reserved = new_size;
        Handles_Append(handles, ob);
    }
}

JavaType_Type *
Handles_Find(Handles *handles, uint32_t handle)
{
    size_t i;
    for (i = 0; i < handles->size; i++){
        if (handles->stream[i]->handle == handle){

            return handles->stream[i]->ob;
        }
    }
    return NULL;
}

JavaType_Type *
JavaType_New(char type)
{
    JavaType_Type *ob = NULL;

    ob = (JavaType_Type *)malloc(sizeof(JavaType_Type));
    assert(ob != NULL);
    ob->jt_type = type;
    ob->prim_typecode = 0;
    ob->obj_typecode = 0;
    ob->classname = NULL;
    ob->fieldname = NULL;
    ob->string = NULL;
    ob->proxy_interface_names = NULL;
    ob->is_primitive = 0;
    ob->is_array = 0;
    ob->is_object = 0;
    ob->unused = 0;
    ob->array_size = 0;
    ob->n_fields = 0;
    ob->n_proxy_interface_names = 0;
    ob->n_chars = 0;
    ob->serial_version_uid = 0;
    ob->flags.sc_write_method = 0;
    ob->flags.sc_serializable = 0;
    ob->flags.sc_externalizable = 0;
    ob->flags.sc_block_data = 0;
    ob->flags.sc_enum = 0;
    ob->fields = NULL;
    ob->class_annotation = NULL;
    ob->super = NULL;
    ob->class_descriptor = NULL;
    ob->value = NULL;
    ob->ref_count = 1;
    return ob;
}

void
JavaType_Destruct(JavaType_Type *type)
{
    if (type == NULL) {
        return;
    }

    size_t i;

    /* clear all pointers first */

    free(type->classname);
    type->classname = NULL;

    free(type->fieldname);
    type->fieldname = NULL;

    free(type->string);
    type->string = NULL;


    for (i = 0; i < type->n_proxy_interface_names; i++){

        free(type->proxy_interface_names[i]);
        type->proxy_interface_names[i] = NULL;
    }
    free(type->proxy_interface_names);
    type->proxy_interface_names = NULL;


    for (i = 0; i < type->n_fields; i++){

        JavaType_Destruct(type->fields[i]);
        type->fields[i] = NULL;
    }
    free(type->fields);
    type->fields = NULL;

    JavaType_Destruct(type->class_annotation);
    type->class_annotation = NULL;


    JavaType_Destruct(type->super);
    type->super = NULL;

    JavaType_Destruct(type->class_descriptor);
    type->class_descriptor = NULL;


    Py_XDECREF(type->value);

    type->jt_type = 0;
    type->prim_typecode = 0;
    type->obj_typecode = 0;
    type->is_primitive = 0;
    type->is_array = 0;
    type->is_object = 0;
    type->unused = 0;
    type->array_size = 0;
    type->n_fields = 0;
    type->n_proxy_interface_names = 0;
    type->n_chars = 0;
    type->serial_version_uid = 0;
    type->flags.sc_write_method = 0;
    type->flags.sc_serializable = 0;
    type->flags.sc_externalizable = 0;
    type->flags.sc_block_data = 0;
    type->flags.sc_enum = 0;
}

void 
Handles_Print(Handles *handles){
    
    size_t i;
    for (i = 0; i < handles->size; i++) {
        printf("0x%x: 0x%x = ", handles->stream[i]->ob->jt_type, handles->stream[i]->handle);
        
        switch(handles->stream[i]->ob->jt_type){
            case TC_OBJECT:
            case TC_CLASS:
            case TC_ENUM:
            case TC_ARRAY:
                printf("%s\n", handles->stream[i]->ob->class_descriptor->classname);
                break;
            case TC_CLASSDESC:
                printf("%s\n", handles->stream[i]->ob->classname);
                break;
            case TC_STRING:
            case TC_LONGSTRING:
                printf("%s\n", handles->stream[i]->ob->string);
                break;
            default:
                printf("???\n");

        }

    }


}