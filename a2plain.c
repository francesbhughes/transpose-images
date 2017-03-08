/* Names: Frances Hughes
 * Created: 1/21/2017
 *
 * a2plain.c 
 */

#include <stdlib.h>
#include <a2plain.h>
#include "uarray2.h"

typedef A2Methods_UArray2 A2;

/*
 * Creates a distinct 2D array of memory cells, each of the given ’size’
 * each cell is uninitialized
 */
static A2Methods_UArray2 new(int width, int height, int size)
{
        return UArray2_new(width,height,size);
}

/*
 * Creates a distinct 2D array of memory cells, each of the given ’size’
 * each cell is uninitialized. ’blocksize’ is ignored
 */
static A2Methods_UArray2 new_with_blocksize(int width, int height, int size,
                                            int blocksize)
{
        (void)blocksize;
        return UArray2_new(width,height,size);
}

/*
 *  frees *array2p and overwrites the pointer with NULL
 */
static void a2free(A2 * array2p)
{
        UArray2_free((UArray2_T *) array2p);
}

/*
 * Returns the width of array2
 */
static int width(A2 array2)
{
        return UArray2_width(array2);
}

/*
 * Returns the height of array2
 */
static int height(A2 array2)
{
        return UArray2_height(array2);
}

/*
 * Returns the size of each element in array2
 */
static int size(A2 array2)
{
        return UArray2_size(array2);
}

/*
 * Returns 1 for an unblocked array
 */
static int blocksize(A2 array2)
{
        (void) array2;
        return 1;
}

/*
 * Returns a pointer to the object in column i, row j
 * (checked runtime error if i or j is out of bounds)
 */
static A2Methods_Object *at(A2 array2, int i, int j)
{
        return UArray2_at(array2, i, j);
}


typedef void applyfun(int i, int j, UArray2_T array2b, void *elem, void *cl);

/*
 * row_major visits each row before the next, in order of increasing
 * row index; within a row, column numbers increase
 */
static void map_row_major(A2 array2, A2Methods_applyfun apply, void *cl)
{
        UArray2_map_row_major(array2,(applyfun *)apply, cl);
}

/*
 * col_major visits each column before the next, in order of
 * increasing column index; within a column, row numbers increase
 */
static void map_col_major(A2 array2, A2Methods_applyfun apply, void *cl)
{
        UArray2_map_col_major(array2,(applyfun *)apply, cl);
}

/*
 * map_default uses a row_major order that has good locality
 */
static void map_default(A2 array2, A2Methods_applyfun apply, void *cl)
{
        UArray2_map_row_major(array2, (applyfun *)apply, cl);
}

struct small_closure {
        A2Methods_smallapplyfun *apply;
        void *cl;
};

static void apply_small(int i, int j, UArray2_T array2, void *elem, void *vcl)
{
        struct small_closure *cl = vcl;
        (void)i;
        (void)j;
        (void)array2;
        cl->apply(elem, cl->cl);
}

/*
 * THE FOLLOWING APPLIES TO ALL SMALL_MAP FUNCTIONS: 
 * alternative mapping functions that pass only cell pointer and closure
 */

static void small_map_row_major(A2 array2, A2Methods_smallapplyfun apply, 
                void *cl)
{
        struct small_closure mycl = { apply, cl };
        UArray2_map_row_major(array2, apply_small, &mycl);
}

static void small_map_col_major(A2 array2, A2Methods_smallapplyfun apply, 
                void *cl)
{
        struct small_closure mycl = { apply, cl };
        UArray2_map_col_major(array2, apply_small, &mycl);
}

static void small_map_default(A2 array2, A2Methods_smallapplyfun apply, 
                void *cl) 
{
        struct small_closure mycl = { apply, cl };
        UArray2_map_row_major(array2, apply_small, &mycl);
}

static struct A2Methods_T uarray2_methods_plain_struct = {
        new,
        new_with_blocksize,
        a2free,
        width,
        height,
        size,
        blocksize,
        at,
        map_row_major,
        map_col_major,
        NULL,                   /* map_block_major */
        map_default,            /* map_default */
        small_map_row_major,
        small_map_col_major,
        NULL,                   /*small_map_block_major */
        small_map_default,      /* small_map_default */
};

A2Methods_T uarray2_methods_plain = &uarray2_methods_plain_struct;
