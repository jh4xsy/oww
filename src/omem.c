/* omem.h */

/* Oww memory allocator */

#include <stdlib.h>
//#include <assert.h>

#include "omem.h"

/* Create a new memory allocation block, and allocate memory */

omem *
omem_new(int lump_size, int initial_size)
{
  omem *new ;

  new = (omem *) malloc(sizeof(omem)) ;

  if (!new) return NULL ;

  new->lump_size = lump_size ;

  if (initial_size > 0)
  {
    new->mem = calloc(initial_size, 1) ;
    new->alloced = (new->mem) ? initial_size : 0 ;
  }
  else
  {
    new->mem = NULL ;
    new->alloced = 0 ;
  }

  return new ;
}

/* Ensure we have at least the specified memory allocation */
int
omem_ensure(omem *mem, int min)
{
  int lumps ;
  size_t size ;

//  assert((mem != NULL)) ;

  if (mem->alloced >= min) return 1 ; /* Ok - already alloced */

  /* Need more memory */
  lumps = min / mem->lump_size ;
  size = lumps * mem->lump_size ;
  if (size < min) size += mem->lump_size ;

  mem->mem = realloc(mem->mem, size) ;
  mem->alloced = (mem->mem != NULL) ? size : 0 ;

  return (mem->mem != NULL) ;
}

/* Free the memory allocated for a memory block */
void
omem_free(omem *mem)
{
//  assert((mem != NULL)) ;

  if (mem->mem) free(mem->mem) ;
  mem->alloced = 0 ;
}

/* Free up a memory block completely */
void
omem_wipe(omem *mem)
{
//  assert((mem != NULL)) ;

  free(mem->mem) ;
  free(mem) ;
}
