#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "dplist.h"

struct dplist_node {
    dplist_node_t *prev, *next;
    void *element;
};

struct dplist {
    dplist_node_t *head;
    void *(*element_copy)(void *src);
    void (*element_free)(void **element);
    int (*element_compare)(void *x, void *y);
};

dplist_t *dpl_create(
    void *(*element_copy)(void *),
    void (*element_free)(void **),
    int (*element_compare)(void *, void *)
) {
    dplist_t *list = malloc(sizeof(struct dplist));
    assert(list != NULL);
    list->head = NULL;
    list->element_copy = element_copy;
    list->element_free = element_free;
    list->element_compare = element_compare;
    return list;
}

void dpl_free(dplist_t **list, bool free_element) {
    if (list == NULL || *list == NULL) return;

    dplist_node_t *curr = (*list)->head;
    while (curr != NULL) {
        dplist_node_t *next_node = curr->next;
        if (free_element && (*list)->element_free != NULL) {
            (*list)->element_free(&(curr->element));
        }
        free(curr);
        curr = next_node;
    }
    free(*list);
    *list = NULL;
}

int dpl_size(dplist_t *list) {
    if (list == NULL) return -1;
    int count = 0;
    dplist_node_t *curr = list->head;
    while (curr != NULL) {
        count++;
        curr = curr->next;
    }
    return count;
}

dplist_t *dpl_insert_at_index(dplist_t *list, void *element, int index, bool insert_copy) {
    if (list == NULL) return NULL;

    dplist_node_t *new_node = malloc(sizeof(dplist_node_t));
    assert(new_node != NULL);

    if (insert_copy && list->element_copy) {
        new_node->element = list->element_copy(element);
    } else {
        new_node->element = element;
    }

    if (list->head == NULL) {
        new_node->prev = new_node->next = NULL;
        list->head = new_node;
    } else if (index <= 0) {
        new_node->prev = NULL;
        new_node->next = list->head;
        list->head->prev = new_node;
        list->head = new_node;
    } else {
        dplist_node_t *ref = dpl_get_reference_at_index(list, index);
        if (index >= dpl_size(list)) { // Insert after the last element
            new_node->next = NULL;
            new_node->prev = ref;
            ref->next = new_node;
        } else { // Insert before ref
            new_node->prev = ref->prev;
            new_node->next = ref;
            ref->prev->next = new_node;
            ref->prev = new_node;
        }
    }
    return list;
}

dplist_t *dpl_remove_at_index(dplist_t *list, int index, bool free_element) {
    if (list == NULL || list->head == NULL) return list;

    dplist_node_t *ref = dpl_get_reference_at_index(list, index);

    if (ref->prev == NULL) { // removing head
        list->head = ref->next;
    } else {
        ref->prev->next = ref->next;
    }

    if (ref->next != NULL) {
        ref->next->prev = ref->prev;
    }

    if (free_element && list->element_free) {
        list->element_free(&(ref->element));
    }
    free(ref);
    return list;
}

dplist_node_t *dpl_get_reference_at_index(dplist_t *list, int index) {
    if (list == NULL || list->head == NULL) return NULL;

    dplist_node_t *curr = list->head;
    if (index <= 0) return curr;

    int i = 0;
    while (curr->next != NULL && i < index) {
        curr = curr->next;
        i++;
    }
    return curr;
}

void *dpl_get_element_at_index(dplist_t *list, int index) {
    dplist_node_t *ref = dpl_get_reference_at_index(list, index);
    return (ref != NULL) ? ref->element : NULL;
}

int dpl_get_index_of_element(dplist_t *list, void *element) {
    if (list == NULL || list->head == NULL) return -1;

    dplist_node_t *curr = list->head;
    int index = 0;
    while (curr != NULL) {
        if (list->element_compare(curr->element, element) == 0) {
            return index;
        }
        curr = curr->next;
        index++;
    }
    return -1;
}

void *dpl_get_element_at_reference(dplist_t *list, dplist_node_t *reference) {
    if (list == NULL || reference == NULL || list->head == NULL) return NULL;

    // Verify reference exists in this list
    dplist_node_t *curr = list->head;
    while (curr != NULL) {
        if (curr == reference) return curr->element;
        curr = curr->next;
    }
    return NULL;
}
