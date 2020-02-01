/* omem.h */

/* Oww memory allocator */

#ifndef OMEM_H

#define OMEM_H 1

typedef struct {
  void *mem ;
  int lump_size ;
  int alloced ;
} omem ;

/* Create a new memory allocation block, and allocate memory */

omem *
omem_new(int lump_size, int initial_size) ;

/* Ensure we have at least the specified memory allocation */
int
omem_ensure(omem *mem, int min) ;

/* Free the memory allocated for a memory block */
void
omem_free(omem *mem) ;

/* Free up a memory block completely */
void
omem_wipe(omem *mem) ;

#endif
