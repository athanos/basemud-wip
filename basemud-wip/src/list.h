/* file: list.h
 *
 * Headerfile for a basic double-linked list
 */

#ifndef _LIST_HEADER
#define _LIST_HEADER

#include <stdbool.h>


//List Macros
#define ITERATE_LIST( list, var_type, var, content ) \
{\
        ITERATOR Iter;\
        AttachIterator( &Iter, list ); \
        while ( ( var = ( var_type * ) NextInList( &Iter ) ) ) \
        { \
                content \
        } \
        DetachIterator( &Iter ); \
}
#define DESTROY_LIST( list, struct_var, delete_func ) \
{ \
        struct_var *var; \
        ITERATOR Iter; \
\
        AttachIterator( &Iter, list ); \
\
        while ( ( var = ( struct_var * ) NextInList( &Iter ) ) ) \
        { \
                delete_func( var ); \
        } \
\
        DetachIterator( &Iter ); \
\
        DeleteList( list ); \
}

#define CLEAR_LIST( list, var, type, func ) \
{ \
AttachIterator( &Iter, list ); \
while ( ( var = type NextInList( &Iter ) ) ) \
func( var ); \
DetachIterator( &Iter ); \
DeleteList( list ); \
}


typedef struct Cell
{
  struct Cell  *_pNextCell;
  struct Cell  *_pPrevCell;
  void         *_pContent;
  int           _valid;
} CELL;

typedef struct List
{
  CELL  *_pFirstCell;
  CELL  *_pLastCell;
  int    _iterators;
  int    _size;
  int    _valid;
} LIST;

typedef struct Iterator
{
  LIST  *_pList;
  CELL  *_pCell;
} ITERATOR;

LIST *AllocList          ( void );
void  AttachIterator     ( ITERATOR *pIter, LIST *pList);
void *NextInList         ( ITERATOR *pIter );
void  AttachToList       ( void *pContent, LIST *pList );
void  DetachFromList     ( void *pContent, LIST *pList );
void  DetachIterator     ( ITERATOR *pIter );
void  FreeList           ( LIST *pList );
int   SizeOfList         ( LIST *pList );
void  DeleteList         ( LIST *list);
void  DeleteCell         ( CELL *cell, LIST *list );

#endif
