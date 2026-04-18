#include "vector.h"
#include <stdint.h>
#include <string.h>

Vector* vector_create(const size_t element_size, const size_t initial_capacity) {
    Vector* tmp = NULL;

    tmp = (Vector*)malloc(sizeof(Vector));
    if (tmp == NULL) { return NULL; }

    tmp->arr = (char*)malloc(element_size * initial_capacity);
    if (tmp->arr == NULL) { return NULL; }

    tmp->capacity = initial_capacity;
    tmp->elem_size = element_size;
    tmp->size = 0;

    return tmp;
}

void vector_free(Vector* vector) {
    free(vector->arr);
    free(vector);
    vector = NULL;
}

size_t vector_size(const Vector* vector) {
    return vector->size;
}

size_t vector_capacity(const Vector* vector) {
    return vector->capacity;
}

void* vector_arr(const Vector* vector) {
    if (vector == NULL) { return NULL; }
    if (vector->arr == NULL) { return NULL; }
    return vector->arr;
}

int vector_push_back(Vector* vector, const void* value) {
    if (vector == NULL || value == NULL) { return VECTOR_INVALID_PTR; }
    size_t size = vector->size;
    size_t capacity = vector->capacity;
    size_t elem_size = vector->elem_size;

    if (size == capacity) {
        if (capacity > SIZE_MAX / 2) { return VECTOR_SIZE_T_OVERFLOW; }
        if (elem_size * capacity > SIZE_MAX / 2) { return VECTOR_SIZE_T_OVERFLOW; }
        char* tmp = (char*)realloc(vector->arr, elem_size * capacity * 2);
        if (tmp == NULL) { return VECTOR_ERR_ALLOC; }
        capacity *= 2;
        vector->arr = tmp;
    }

    memcpy(&(vector->arr[size * elem_size]), value, elem_size);
    size += 1;

    vector->size = size;
    vector->capacity = capacity;

    return VECTOR_SUCCESS;
}

int vector_pop_back(Vector* vector, void* out_val) {
    if (vector == NULL) { return VECTOR_INVALID_PTR; }
    if (vector->size < 1) { return VECTOR_ERR_OUT_OF_RANGE; }

    if (out_val == NULL) { 
        vector->size--;
        return VECTOR_SUCCESS;
    }

    memcpy(out_val, &(vector->arr[(vector->size - 1) * vector->elem_size]),
            vector->elem_size);
    vector->size--;
    return VECTOR_SUCCESS;
}

int vector_insert(Vector* vector, const void* value, size_t index) {
    if (vector == NULL || value == NULL) { return VECTOR_INVALID_PTR; }
    size_t size = vector->size;
    size_t capacity = vector->capacity;
    size_t elem_size = vector->elem_size;

    if (index > size) { return VECTOR_ERR_OUT_OF_RANGE; }

    if (size == capacity) {
        if (capacity > SIZE_MAX / 2) { return VECTOR_SIZE_T_OVERFLOW; }
        if (elem_size * capacity > SIZE_MAX / 2) { return VECTOR_SIZE_T_OVERFLOW; }
        char* tmp = (char*)realloc(vector->arr, elem_size * capacity * 2);
        if (tmp == NULL) { return VECTOR_ERR_ALLOC; }
        capacity *= 2;
        vector->arr = tmp;
    }

    size_t elem_index = (index) * elem_size;
    size_t shift_size = (size - index) * elem_size;
    if (index < size) {
        memmove(&(vector->arr[elem_index + elem_size]), &(vector->arr[elem_index]),
                shift_size);
    }

    memcpy(&(vector->arr[index * elem_size]), value, elem_size);
    size += 1;

    vector->size = size;
    vector->capacity = capacity;

    return VECTOR_SUCCESS;
}

int vector_remove(Vector* vector, const size_t index) {
    if (vector == NULL) { return VECTOR_INVALID_PTR; }
    size_t size = vector->size;
    size_t elem_size = vector->elem_size;

    if (index >= size) { return VECTOR_ERR_OUT_OF_RANGE; }
    if (vector->size < 1) { return VECTOR_ERR_OUT_OF_RANGE; }
    
    size_t elem_index = (index) * elem_size;
    size_t shift_size = (size - index - 1) * elem_size;
    memmove(&(vector->arr[elem_index]), &(vector->arr[elem_index + elem_size]),
            shift_size);
    size--;
    vector->size = size;

    return VECTOR_SUCCESS;
}

char* vector_get(const Vector* vector, const size_t index) {
    if (vector == NULL) { return NULL; }
    if (vector->arr == NULL) { return NULL; }
    if (index >= vector->size) { return NULL; }
    if (vector->size < 1) { return NULL; }

    return &(vector->arr[index * vector->elem_size]);
}

int vector_set(Vector* vector, const void* value, const size_t index) {
    if (vector == NULL || value == NULL) { return VECTOR_INVALID_PTR; }
    size_t size = vector->size;
    size_t elem_size = vector->elem_size;

    if (index >= size) { return VECTOR_ERR_OUT_OF_RANGE; }

    memcpy(&(vector->arr[index * elem_size]), value, elem_size);

    return VECTOR_SUCCESS;
}

void vector_clear(Vector* vector) {
    vector->size = 0;
}

int vector_reserve(Vector* vector, const size_t req_cap) {
    if (vector == NULL) { return VECTOR_INVALID_PTR; }
    size_t size = vector->size;
    size_t capacity = vector->capacity;
    size_t elem_size = vector->elem_size;

    if (req_cap <= size || req_cap == capacity) { return VECTOR_REQ_CAP_INVALID; }

    if (elem_size * req_cap > SIZE_MAX) { return VECTOR_SIZE_T_OVERFLOW; }
    char* tmp = (char*)realloc(vector->arr, elem_size * req_cap);
    if (tmp == NULL) { return VECTOR_ERR_ALLOC; }

    vector->capacity = req_cap;
    vector->arr = tmp;

    return VECTOR_SUCCESS;
}

int vector_shrink(Vector* vector) {
    if (vector == NULL) { return VECTOR_INVALID_PTR; }
    size_t size = vector->size;
    size_t elem_size = vector->elem_size;

    char* tmp = (char*)realloc(vector->arr, elem_size * size);
    if (tmp == NULL) { return VECTOR_ERR_ALLOC; }

    vector->capacity = size;
    vector->arr = tmp;

    return VECTOR_SUCCESS;
}

int vector_push_front(Vector* vector, const void* value) {
   return vector_insert(vector, value, 0);
}

int vector_pop_front(Vector* vector, void* out_val) {
    if (vector == NULL) { return VECTOR_INVALID_PTR; }
    if (vector->size < 1) { return VECTOR_ERR_OUT_OF_RANGE; }
    size_t size = vector->size;
    size_t elem_size = vector->elem_size;

    if (out_val != NULL) {
        memcpy(out_val, vector->arr, elem_size);
    }

    memmove(&(vector->arr[0]), &(vector->arr[elem_size]), (size - 1) * elem_size);
    size--;
    vector->size = size;

    return VECTOR_SUCCESS;
}

Vector* vector_copy(const Vector* vector) {
    Vector* output = vector_create(vector->elem_size, vector->capacity);
    memcpy(output->arr, vector->arr, vector->size * vector->elem_size);
    return output;
}

int vector_swap(Vector* vector, const size_t index1, const size_t index2) {
    if (vector == NULL) { return VECTOR_INVALID_PTR; }
    if (index1 == index2) { return VECTOR_SWAP_PARAM_INVALID; }
    size_t elem_size = vector->elem_size;

    char* tmp = NULL;
    tmp = (char*)malloc(sizeof(char) * elem_size);
    if (tmp == NULL) { return VECTOR_ERR_ALLOC; }

    memcpy(tmp, &(vector->arr[index1 * elem_size]), elem_size);
    memcpy(&(vector->arr[index1 * elem_size]), &(vector->arr[index2 * elem_size]),
            elem_size);
    memcpy(&(vector->arr[index2 * elem_size]), tmp, elem_size);
    free(tmp);
    return VECTOR_SUCCESS;
}

size_t vector_find(const Vector* vector, const void* value,
                    int (*cmp)(const void*, const void*)) {
    if (vector == NULL || value == NULL) { return VECTOR_NOT_FOUND_POS; }
    if (vector->size < 1) { return VECTOR_ERR_OUT_OF_RANGE; }

    for (size_t i = 0; i < vector->size; i++) {
        void* tmp = vector_get(vector, i);
        if ((*cmp)(tmp, value) == 0) {
            return i;
        }
    }
    return VECTOR_NOT_FOUND_POS;
}

int ch_vcmp(const void* a, const void* value) {
    char* first = (char*)a;
    char* char_val = (char*)value;
    if (*first == *char_val) {
        return 0;
    }
    return 1;
}

char* vector_get_string(const Vector* vector, size_t index) {
    return *(char**)vector_get(vector, index);
}
