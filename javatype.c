#include <stdlib.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "javatype.h"

Handles *
Handles_New(size_t size)
{
    Handles *handles;

    handles = (Handles *)malloc(sizeof(Handles));
    handles->stream = (StreamReference **)malloc(sizeof(void)*size);
    handles->size = 0;
    handles->reserved = size;
    handles->next_handle = 0x7e0000;

    return handles;
}

uint8_t
Handles_Destruct(Handles handles)
{
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

    StreamReference *ref;

    ref = (StreamReference *)malloc(sizeof(StreamReference));
    assert(ref != NULL);
    ref->ob = ob;
    ref->handle = handles->next_handle++;
    
    if (handles->size < handles->reserved){
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
    JavaType_Type *ob;

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
    return ob;
}