/*
 * sllist.c
 *
 *  Created on: 26-Mar-2009
 *      Author: sjm
 */

// Singly-linked list utilities

#include <stdlib.h>
#include "sllist.h"

sllist *sllist_new(void *data) {
  sllist *newlist;

  newlist = calloc(1, sizeof(sllist));
  newlist->data = data;

  return newlist;
}

/**
 * Add a new item to the end of a list.
 */
sllist *sllist_append(sllist **list, void *data) {
  sllist *newlist, *last;

  // Make new list entry with our data
  newlist = sllist_new(data);

  // Do we have a list to append this to?
  if (*list == NULL) {
	// No list - make the new entry the start of the list
	*list = newlist;
  } else {
	// We have a list - add new entry at end
	last = sllist_last(*list);
	last->next = newlist;
  }

  return newlist;
}

/**
 * Find the last entry in a list
 */
sllist *sllist_last(sllist *list) {
  if (list == NULL) return NULL;

  while (list->next != NULL)
	list = list->next;

  return list;
}

/**
 * Find whichever entry in a list holds the given data
 */
sllist *sllist_find(sllist *list, void *data) {
  while (list != NULL) {
	if (list->data == data) return list;
	list = list->next;
  }
  return NULL;
}

/**
 * Find the entry in a list that comes before the given entry
 */
sllist *sllist_prev(sllist *list, sllist *entry) {
  while (list != NULL) {
	if (list->next == entry) return list;
	list = list->next;
  }
  return NULL;
}
/**
 * Insert a new entry at the given list entry.
 */
sllist *sllist_insert(sllist *entry, void *data) {
  sllist *newlist, *next;

  if (entry == NULL) return NULL;

  next = entry->next;

  newlist = sllist_new(data);
  entry->next = newlist;
  newlist->next = next;

  return newlist;
}

/**
 * If entry is in list, unlink it and free it.
 *
 * Return start of list, which will have changed if the
 * first entry was deleted.
 */
sllist *sllist_delete(sllist *list, sllist *entry) {
  if ((entry == NULL) || (list == NULL)) return list;

  while (list != NULL) {
	// Catch delete of first list entry
	if (list == entry)
	{
	  sllist *newlist;
	  newlist = list->next;
	  free(list);
	  return newlist;
	}

	// Look ahead
	if (list->next == entry) {
	  list->next = entry->next;
	  free(entry);
	  return list;
	}
	list = list->next;
  }

  return list;
}

/**
 * Search the list for a matching entry.
 *
 * matcher is a function which returns 0 for an sllist entry that matches
 */
sllist *sllist_match(sllist *list, sllist_matcher matcher, void *data){
  while (list != NULL) {
	if (matcher(list->data, data)==0) return list;
	list = list->next;
  }

  return NULL;
}
