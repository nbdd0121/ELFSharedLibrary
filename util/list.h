#ifndef NORLIT_CONTAINER_LIST_H
#define NORLIT_CONTAINER_LIST_H

#include <stdbool.h>
#include <stddef.h>

#ifdef _MSC_VER
#define inline __inline
#endif

typedef struct str_list_t {
    struct str_list_t *prev;
    struct str_list_t *next;
} list_t;

static inline void list_empty(list_t *list) {
    list->prev = list->next = list;
}

#define container_of(list, str, name) ((str*)((size_t)list - offsetof(str, name)))

#define list_add list_addLast

static inline bool list_isEmpty(list_t *list) {
    return list->prev == list;
}

static inline void list_addFirst(list_t *list, list_t *inserted) {
    inserted->next = list->next;
    inserted->prev = list;
    list->next->prev = inserted;
    list->next = inserted;
}

static inline void list_addLast(list_t *list, list_t *inserted) {
    inserted->prev = list->prev;
    inserted->next = list;
    list->prev->next = inserted;
    list->prev = inserted;
}

static inline void list_remove(list_t *list) {
    list->prev->next = list->next;
    list->next->prev = list->prev;
}

#define list_forEach(list, var, str, name)\
    for (\
            list_t *__listHead = (list), *__listVar = __listHead->next, *__listNext = __listVar->next;\
            __listVar != __listHead && (var = container_of(__listVar, str, name));\
            __listVar = __listNext, __listNext = __listVar->next\
        )\

#define list_forEachInclusive(list, var, str, name)\
    for (\
            list_t *__listHead = (list), *__listVar = __listHead, *__listNext = __listVar->next;\
            __listNext && (var = container_of(__listVar, str, name));\
            __listVar = __listNext, __listNext = __listVar == __listHead ? NULL: __listVar->next\
        )\

#endif
