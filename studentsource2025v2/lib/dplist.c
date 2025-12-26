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
    dplist_t *list = (dplist_t *)malloc(sizeof(struct dplist));
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
    dplist_node_t *next;

    while (curr != NULL) {
        next = curr->next;
        if (free_element == true && (*list)->element_free != NULL) {
            (*list)->element_free(&(curr->element));
        }
        free(curr);
        curr = next;
    }
    
    free(*list);
    *list = NULL; 
}

int dpl_size(dplist_t *list) {
    if (list == NULL) return -1;
    
    int count = 0;
    dplist_node_t *temp = list->head;
    while (temp != NULL) {
        count++;
        temp = temp->next;
    }
    return count;
}

dplist_t *dpl_insert_at_index(dplist_t *list, void *element, int index, bool insert_copy) {
    if (list == NULL) return NULL;

    dplist_node_t *new_node = malloc(sizeof(struct dplist_node));
    assert(new_node != NULL); 

    if (insert_copy == true && list->element_copy != NULL) {
        new_node->element = list->element_copy(element);
    } else {
        new_node->element = element;
    }

    if (list->head == NULL) {
        new_node->prev = NULL;
        new_node->next = NULL;
        list->head = new_node;
    } else if (index <= 0) {
        new_node->next = list->head;
        new_node->prev = NULL;
        list->head->prev = new_node;
        list->head = new_node;
    } else {
        dplist_node_t *ref = dpl_get_reference_at_index(list, index);
        if (index >= dpl_size(list)) {
            ref->next = new_node;
            new_node->prev = ref;
            new_node->next = NULL;
        } else {
            new_node->prev = ref->prev;
            new_node->next = ref;
            if (ref->prev != NULL) {
                ref->prev->next = new_node;
            }
            ref->prev = new_node;
        }
    }
    return list;
}

dplist_t *dpl_remove_at_index(dplist_t *list, int index, bool free_element) {
    if (list == NULL) return NULL;
    if (list->head == NULL) return list;

    dplist_node_t *to_remove = dpl_get_reference_at_index(list, index);

    if (to_remove->prev == NULL) {
        list->head = to_remove->next;
        if (list->head != NULL) {
            list->head->prev = NULL;
        }
    } else {
        to_remove->prev->next = to_remove->next;
        if (to_remove->next != NULL) {
            to_remove->next->prev = to_remove->prev;
        }
    }

    if (free_element == true && list->element_free != NULL) {
        list->element_free(&(to_remove->element));
    }
    
    free(to_remove);
    return list;
}

dplist_node_t *dpl_get_reference_at_index(dplist_t *list, int index) {
    if (list == NULL || list->head == NULL) return NULL;

    dplist_node_t *dummy = list->head;
    if (index <= 0) return dummy;

    for (int i = 0; i < index; i++) {
        if (dummy->next == NULL) return dummy; 
        dummy = dummy->next;
    }
    return dummy;
}

void *dpl_get_element_at_index(dplist_t *list, int index) {
    if (list == NULL) return NULL;
    dplist_node_t *res = dpl_get_reference_at_index(list, index);
    if (res == NULL) return NULL;
    return res->element;
}

int dpl_get_index_of_element(dplist_t *list, void *element) {
    if (list == NULL) return -1;
    if (list->head == NULL) return -1;

    dplist_node_t *current = list->head;
    int pos = 0;
    while (current != NULL) {
        if (list->element_compare(current->element, element) == 0) {
            return pos;
        }
        current = current->next;
        pos++;
    }
    return -1;
}

void *dpl_get_element_at_reference(dplist_t *list, dplist_node_t *reference) {
    if (list == NULL || reference == NULL || list->head == NULL) return NULL;

    dplist_node_t *check = list->head;
    while (check != NULL) {
        if (check == reference) return reference->element;
        check = check->next;
    }
    
    return NULL;
}
