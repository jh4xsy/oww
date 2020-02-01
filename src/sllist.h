/*
 * sllist.h
 *
 *  Created on: 26-Mar-2009
 *      Author: sjm
 */

#ifndef SLLIST_H_
#define SLLIST_H_

typedef struct _sllist {
  void *data;
  struct _sllist *next;
} sllist ;

/**
 * Function type that compares list entry data with some other data.
 *
 * A zero return value indicates a match
 */
typedef int (*sllist_matcher)(void *, void *);

// Function prototypes
sllist *sllist_last(sllist *list);
sllist *sllist_append(sllist **list, void *data);
sllist *sllist_insert(sllist *entry, void *data);
sllist *sllist_find(sllist *list, void *data);
sllist *sllist_insert(sllist *entry, void *data);
sllist *sllist_match(sllist *list, sllist_matcher matcher, void *data);
sllist *sllist_delete(sllist *list, sllist *entry);
#endif /* SLLIST_H_ */
