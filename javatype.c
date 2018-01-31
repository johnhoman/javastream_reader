#include <stdlib.h>


Handles *
Handles_New(size_t size)
{
    Handles handles;

    handles = (Handles *)malloc(sizeof(Handles));
    handles->stream = (StreamReference **)malloc(sizeof(void)*size);
    handles->size = 0;
    handles->reserved = size;

    return handles;
}

uint8_t
Handles_Destruct(Handles handles)
{

}

void 
Handles_Append(Handles handles, JavaType_Type *ob)
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
        uint8_t result;

        new_size = sizeof(void *) * handles->reserved * 2;
        handles->stream = realloc(handles->stream, new_size);
        assert(handles->stream != NULL);
        handles->reserved = new_size;
        Handles_Append(handles, ob);
    }
}

JavaType_Type *
JavaType_New(int type)
{
    JavaType_Type *ob;

    ob = (JavaType_Type *)malloc(sizeof(JavaType_Type));
    assert(ob != NULL);

    ob->jt_type = type;

    return ob;
}