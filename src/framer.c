/**
 * @file   framer.c
 * @author Tero Isannainen <tero.isannainen@gmail.com>
 * @date   Sun Mar 11 08:35:02 2018
 *
 * @brief  Framer - Unrolled Doubly Linked List library
 *
 */

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "framer.h"


const char* framer_version = "0.0.1";

static fn_t alloc_node( fr_t pos );


/* ------------------------------------------------------------
 * Macros:
 * ------------------------------------------------------------ */

/** Boolean true. */
#define fr_true 1
/** Boolean false. */
#define fr_false 0


/** Call custom alloc function. */
#define memapi_alloc( pos ) ( pos )->mem->alloc( ( pos ), ( pos )->mem->env )

/** Call custom free function. */
#define memapi_free( pos ) ( pos )->mem->free( ( pos ), ( pos )->mem->env )

/** Half size of segment. */
#define half_seg( pos ) \
    ( ( ( pos )->size & 0x1L ) == 0 ? ( pos )->size / 2 : ( pos )->size / 2 + 1 )



/* ------------------------------------------------------------
 * Framer access:
 * ------------------------------------------------------------ */

fr_t fr_create( void )
{
    return fr_create_sized( FR_SEG_DEFAULT );
}


fr_t fr_create_sized( fr_size_t size )
{
    fn_t fn;
    fr_t pos;

    fn = fn_new_sized( size );
    pos = fr_pos_new( size );
    pos->seg = fn;
    pos->ncnt = 1;

    return pos;
}


fr_t fr_create_using( fr_t pos )
{
    pos->seg = alloc_node( pos );
    pos->ncnt = 1;
    return pos;
}


fr_t fr_destroy( fr_t pos )
{
    fn_t next;

    pos->seg = fn_first( pos->seg );

    if ( pos->mem ) {

        while ( pos->seg ) {
            next = pos->seg->next;
            memapi_free( pos );
            pos->seg = next;
        }

    } else {

        while ( pos->seg ) {
            next = pos->seg->next;
            fr_free( pos->seg );
            pos->seg = next;
        }
    }

    return fr_pos_del( pos );
}


void fr_insert( fr_t pos, void* item )
{
    fn_t s;

    pos->icnt++;
    s = pos->seg;

    if ( pos->idx > 0 && s->used < pos->size ) {

        /* xx.-x..
         *      ^
         */

        if ( pos->idx < s->used ) {

            memmove( &( s->data[ pos->idx + 1 ] ),
                     &( s->data[ pos->idx ] ),
                     ( s->used - pos->idx ) * FR_ITEM_SIZE );
        }

        s->data[ pos->idx ] = item;
        s->used++;

    } else if ( pos->idx == 0 && s->prev && ( s->prev->used < pos->size ) ) {

        /* xx.-x..
         *     ^
         */

        s = s->prev;
        s->data[ s->used ] = item;
        s->used++;
        pos->seg = s;
        pos->idx = s->used - 1;

    } else if ( s->used < pos->size ) {

        /* xx.-x..
         *      ^
         */

        if ( pos->idx < s->used ) {

            memmove( &( s->data[ pos->idx + 1 ] ),
                     &( s->data[ pos->idx ] ),
                     ( s->used - pos->idx ) * FR_ITEM_SIZE );
        }

        s->data[ pos->idx ] = item;
        s->used++;

    } else {

        /* Full segment. */

        if ( pos->idx >= s->used ) {

            /* Only when last segment if full. */

            /* xx.-xxx
             *        ^
             */

            pos->ncnt++;

            s = fn_append( s, alloc_node( pos ) );

            s->data[ 0 ] = item;
            s->used++;

            pos->seg = s;
            pos->idx = 0;

        } else {

            /* Check if peers have space. */

            if ( pos->idx < half_seg( pos ) && pos->seg->prev ) {

                fn_t prev = pos->seg->prev;

                if ( pos->idx != 0 && pos->idx <= ( pos->size - prev->used ) ) {

                    /* All fit to left. */

                    /* xxx..-xxxxx
                     *        ^
                     */

                    memcpy(
                        &( prev->data[ prev->used ] ), pos->seg->data, pos->idx * FR_ITEM_SIZE );
                    prev->used += pos->idx;

                    pos->seg->used -= pos->idx;

                    if ( pos->idx > 1 ) {
                        memmove( &( pos->seg->data[ 1 ] ),
                                 &( pos->seg->data[ pos->idx ] ),
                                 pos->seg->used * FR_ITEM_SIZE );
                    }

                    pos->seg->data[ 0 ] = item;
                    pos->seg->used++;
                    pos->idx = 0;

                    return;
                }

            } else if ( pos->idx >= half_seg( pos ) && pos->seg->next ) {

                fn_t      next = pos->seg->next;
                fr_size_t cnt = pos->seg->used - pos->idx;

                if ( cnt <= ( pos->size - next->used ) ) {

                    /* All fit to right. */

                    /* xxxxx-xxx..
                     *    ^
                     */

                    memmove( &( next->data[ cnt ] ), next->data, cnt * FR_ITEM_SIZE );

                    memcpy( next->data, &( pos->seg->data[ pos->idx ] ), cnt * FR_ITEM_SIZE );

                    next->used += cnt;
                    pos->seg->used -= cnt;
                    pos->seg->data[ pos->idx ] = item;
                    pos->seg->used++;

                    return;
                }
            }


            /*
             * Peers don't have space, hence open a new segment and
             * even out.
             */

            pos->ncnt++;

            fn_append( s, alloc_node( pos ) );

            fr_size_t tail_cnt = s->used - pos->idx;
            memcpy( s->next->data, &( s->data[ pos->idx ] ), tail_cnt * FR_ITEM_SIZE );

            s->next->used = tail_cnt;

            s->data[ pos->idx ] = item;
            s->used = pos->idx + 1;

            fr_even( pos );
        }
    }
}


void fr_append( fr_t pos, void* item )
{
    if ( pos->icnt != 0 && ( pos->idx == pos->seg->used - 1 ) && pos->seg->next == NULL &&
         pos->seg->used < pos->size ) {

        pos->icnt++;
        pos->seg->used++;
        pos->idx++;
        pos->seg->data[ pos->idx ] = item;

    } else {

        if ( pos->icnt == 0 ) {

            /* Empty Framer. */
            fr_insert( pos, item );

        } else {

            if ( pos->seg->used < pos->size ) {

                /* xx.-xx.-xx.  ->  xx.-xx.-xx.
                 *      ^                 ^
                 */
                pos->idx++;

            } else {

                if ( pos->seg->next == NULL ) {

                    /* xx.-xxx  ->  xx.-xxx
                     *       ^             ^
                     */
                    pos->idx++;

                } else {

                    /* xx.-xxx-x..  ->  xx.-xxx-x..
                     *       ^                  ^
                     */
                    fr_next( pos );
                }
            }

            fr_insert( pos, item );
        }
    }
}


void* fr_delete( fr_t pos )
{
    fn_t  s = pos->seg;
    void* ret = s->data[ pos->idx ];

    if ( s->used > 1 ) {

        pos->icnt--;

        if ( pos->idx < s->used - 1 ) {
            memmove( &( s->data[ pos->idx ] ),
                     &( s->data[ pos->idx + 1 ] ),
                     ( s->used - ( pos->idx + 1 ) ) * FR_ITEM_SIZE );
        } else {
            if ( s->next ) {
                pos->seg = s->next;
                pos->idx = 0;
            } else {
                pos->idx--;
            }
        }

        s->used--;

    } else if ( s->used == 1 ) {

        pos->icnt--;

        if ( s->next == NULL && s->prev == NULL ) {

            /* Leave first node. */
            s->used--;
            pos->idx = 0;
            s->data[ 0 ] = NULL;

        } else {

            if ( s->next == NULL )
                pos->idx = s->prev->used - 1;
            else
                pos->idx = 0;

            pos->ncnt--;

            if ( pos->mem )
                memapi_free( pos );
            else
                pos->seg = fn_delete( pos->seg );
        }
    }

    return ret;
}


void* fr_delete_even( fr_t pos )
{
    void* ret = fr_delete( pos );
    fr_even( pos );
    return ret;
}


void fr_push( fr_t pos, void* item )
{
    if ( pos->icnt != 0 && pos->seg->used < pos->size ) {

        pos->icnt++;
        pos->seg->used++;
        pos->idx++;
        pos->seg->data[ pos->idx ] = item;

    } else {

        fr_append( pos, item );
    }
}


void* fr_pop( fr_t pos )
{
    if ( pos->idx > 0 ) {

        void* ret;

        ret = pos->seg->data[ pos->idx ];
        pos->idx--;
        pos->seg->used--;
        pos->icnt--;

        return ret;

    } else {

        return fr_delete( pos );
    }
}


int fr_even( fr_t pos )
{
    if ( pos->seg->next ) {

        fn_t next = pos->seg->next;

        if ( ( pos->seg->used + next->used ) <= pos->size ) {

            /* xx..-xx..
             *  ^
             */

            memcpy( &( pos->seg->data[ pos->seg->used ] ), next->data, next->used * FR_ITEM_SIZE );

            pos->seg->used += next->used;

            pos->ncnt--;

            fn_delete( next );

            return 2;

        } else if ( ( pos->seg->used * 2 ) < pos->size ) {

            /* Fill upto half from next. */

            /* xx...-xxxx.  ->  xxx..-xxx..
             *  ^                ^
             */

            fr_size_t cnt;

            cnt = half_seg( pos ) - pos->seg->used;

            memcpy( &( pos->seg->data[ pos->seg->used ] ), next->data, cnt * FR_ITEM_SIZE );

            memmove( next->data, &( next->data[ cnt ] ), cnt * FR_ITEM_SIZE );

            pos->seg->used += cnt;
            pos->seg->next->used -= cnt;

            return 1;

        } else {

            return 0;
        }

    } else if ( pos->seg->prev && ( ( pos->seg->used * 2 ) < pos->size ) ) {

        /* Fill upto half from prev. */

        /*     .------------------v
         * xxxxx-xx...  ->  xxxx.-xxx..
         *        ^                 ^
         */

        fn_t      prev = pos->seg->prev;
        fr_size_t cnt;

        cnt = half_seg( pos ) - pos->seg->used;

        memmove( &( pos->seg->data[ pos->seg->used ] ), pos->seg->data, cnt * FR_ITEM_SIZE );

        memcpy( pos->seg->data, &( prev->data[ prev->used - cnt ] ), cnt * FR_ITEM_SIZE );

        prev->used -= cnt;
        pos->seg->used += cnt;
        pos->idx += cnt;

        return 1;

    } else {

        return 0;
    }
}


int fr_pack_range( fr_t pos, fr_t end, fr_size_t limit )
{
    fr_size_t min_pack;

    min_pack = pos->icnt / pos->ncnt;
    if ( limit <= min_pack || limit > pos->size ) {
        /* Pack not possible. */
        return 0;
    }

    fr_s      a;
    fr_s      b;
    fr_size_t b_used;

    a = *pos;
    while ( a.seg && a.seg->used >= limit && ( !end || a.seg != end->seg ) )
        a.seg = a.seg->next;

    if ( a.seg == NULL || a.seg->next == NULL || ( end && a.seg == end->seg ) )
        return 0;

    a.idx = a.seg->used;
    b = a;
    b.seg = b.seg->next;
    b.idx = 0;
    b_used = b.seg->used;

    if ( end && b.seg == end->seg )
        return 0;

    while ( ( end == NULL && b.seg != NULL ) || b.seg != end->seg ) {

        a.seg->data[ a.idx++ ] = b.seg->data[ b.idx++ ];
        a.seg->used++;

        if ( a.seg->used >= limit ) {
            a.seg = a.seg->next;
            a.idx = 0;
            a.seg->used = 0;
        }

        if ( b.idx >= b_used ) {
            b.seg = b.seg->next;
            if ( b.seg == NULL )
                break;
            b_used = b.seg->used;
            b.idx = 0;
        }
    }

    /* Remove left-over nodes. */
    fn_t na, nb;
    na = a.seg->next;
    a.seg->next = NULL;

    while ( na ) {
        nb = na->next;
        pos->ncnt--;
        fr_free( na );
        na = nb;
    }

    return 1;
}


fr_size_t fr_length( fr_t pos )
{
    return pos->icnt;
}


fr_size_t fr_tail_length( fr_t pos )
{
    fr_size_t cnt = 0;
    fr_s      tmp = *pos;

    /* Tail of current segment. */
    cnt = tmp.seg->used - tmp.idx;

    /* Rest with full "used" count. */
    while ( tmp.seg->next ) {
        tmp.seg = tmp.seg->next;
        cnt += tmp.seg->used;
    }

    return cnt;
}


fr_size_t fr_node_count( fr_t pos )
{
    return pos->ncnt;
}


fr_size_t fr_size( fr_t pos )
{
    return pos->size;
}


fr_size_t fr_index( fr_t pos )
{
    return pos->idx;
}


fr_size_t fr_used( fr_t pos )
{
    return pos->seg->used;
}


fr_s fr_find( fr_t pos, void* item )
{
    fr_s tmp = *pos;

    while ( tmp.seg ) {
        while ( tmp.idx < tmp.seg->used ) {
            if ( tmp.seg->data[ tmp.idx ] == item ) {
                return tmp;
            }
            tmp.idx++;
        }

        tmp.seg = tmp.seg->next;
        tmp.idx = 0;
    }

    tmp.seg = NULL;
    return tmp;
}


fr_s fr_find_with( fr_t pos, void* item, fr_cmp_f comp )
{
    fr_s tmp = *pos;

    for ( ;; ) {

        while ( tmp.idx < tmp.seg->used ) {
            if ( comp( tmp.seg->data[ tmp.idx ], item ) == 0 )
                return tmp;
            tmp.idx++;
        }

        tmp.seg = tmp.seg->next;
        if ( tmp.seg == NULL ) {
            return tmp;
        } else {
            tmp.idx = 0;
        }
    }
}


fr_s fr_find_sorted_with( fr_t pos, void* item, fr_cmp_f comp )
{
    fr_s tmp = *pos;
    fn_t prev = NULL;

    /* Proceed segment by segment. */
    while ( tmp.seg ) {
        if ( comp( item, tmp.seg->data[ tmp.idx ] ) > 0 ) {
            prev = tmp.seg;
            tmp.seg = tmp.seg->next;
            tmp.idx = 0;
        } else
            break;
    }

    /* Jump back if not first. */
    if ( prev )
        tmp.seg = prev;

    tmp.idx = 0;

    return fr_find_with( &tmp, item, comp );
}



/* ------------------------------------------------------------
 * Framer Position:
 * ------------------------------------------------------------ */

fr_t fr_pos_new( fr_size_t size )
{
    fr_t pos;

    pos = fr_malloc( sizeof( fr_s ) );
    return fr_pos_init( pos, size );
}


fr_t fr_pos_new_with_mem( fr_t pos, fr_size_t size, fr_mem_f alloc, fr_mem_f free, void* env )
{
    fr_mem_t mem;

    mem = fr_malloc( sizeof( fr_mem_s ) );
    mem->alloc = alloc;
    mem->free = free;
    mem->env = env;

    if ( pos ) {
        fr_pos_init( pos, size );
    } else {
        pos = fr_pos_new( size );
    }
    pos->mem = mem;

    return pos;
}


fr_t fr_pos_init( fr_t pos, fr_size_t size )
{
    pos->seg = NULL;
    pos->idx = 0;
    pos->size = size;
    pos->icnt = 0;
    pos->ncnt = 0;
    pos->mem = NULL;

    return pos;
}


fr_t fr_pos_del( fr_t pos )
{
    if ( pos->mem )
        fr_free( pos->mem );
    fr_free( pos );
    return NULL;
}


fr_t fr_pos_dup( fr_t pos )
{
    fr_t dup;

    dup = fr_malloc( sizeof( fr_s ) );
    *dup = *pos;

    return dup;
}


void fr_pos_cpy( fr_t pos_a, fr_t pos_b )
{
    *pos_b = *pos_a;
}


fr_size_t fr_next_n( fr_t pos, fr_size_t n )
{
    fr_size_t steps;
    fn_t      seg = pos->seg;
    fr_size_t idx = pos->idx;

    if ( n > pos->size && seg->next ) {

        /* Fast forwards with segment stepping. */

        steps = n - ( seg->used - idx );
        seg = seg->next;
        idx = 0;

        while ( seg->next && steps >= seg->used ) {
            steps -= seg->used;
            seg = seg->next;
        }

        if ( steps <= seg->used - 1 ) {
            pos->idx = steps;
            pos->seg = seg;
            return n;
        } else {
            return 0;
        }

    } else {

        /* Small distance, go one by one. */

        steps = 0;

        while ( steps < n ) {
            if ( idx < seg->used - 1 ) {
                idx++;
                steps++;
            } else {
                if ( seg->next == NULL ) {
                    return 0;
                } else {
                    seg = seg->next;
                    idx = 0;
                    steps++;
                }
            }
        }

        pos->idx = idx;
        pos->seg = seg;
        return n;
    }
}


fr_size_t fr_next( fr_t pos )
{
    if ( pos->idx < pos->seg->used - 1 ) {

        pos->idx++;
        return 1;

    } else {

        if ( pos->seg->next == NULL ) {
            return 0;
        } else {
            /* Step to next segment. */
            pos->seg = pos->seg->next;
            pos->idx = 0;
            return 1;
        }
    }
}


fr_size_t fr_prev_n( fr_t pos, fr_size_t n )
{
    fr_size_t steps;
    fn_t      seg = pos->seg;
    fr_size_t idx = pos->idx;

    if ( n > pos->size ) {

        /* Fast backwards with segment stepping. */

        steps = n - idx;
        idx = 0;

        while ( seg->prev && steps >= seg->prev->used ) {
            seg = seg->prev;
            steps -= seg->used;
        }

        if ( steps == 0 ) {

            pos->seg = seg;
            pos->idx = 0;
            return n;

        } else if ( seg->prev ) {

            seg = seg->prev;
            pos->idx = ( seg->used - steps );
            pos->seg = seg;
            return n;

        } else {
            return 0;
        }

    } else {

        /* Small distance, go one by one. */

        steps = 0;

        while ( steps < n ) {
            if ( idx > 0 ) {
                idx--;
                steps++;
            } else {
                if ( seg->prev == NULL ) {
                    return 0;
                } else {
                    seg = seg->prev;
                    idx = seg->used - 1;
                    steps++;
                }
            }
        }

        pos->idx = idx;
        pos->seg = seg;
        return n;
    }
}


fr_size_t fr_prev( fr_t pos )
{
    if ( pos->idx > 0 ) {

        pos->idx--;
        return 1;

    } else {

        if ( pos->seg->prev == NULL )
            return 0;

        pos->seg = pos->seg->prev;
        pos->idx = pos->seg->used - 1;
        return 1;
    }
}


void* fr_next_item( fr_t pos )
{
    if ( fr_next( pos ) == 0 ) {
        pos->seg = NULL;
        return NULL;
    } else {
        return pos->seg->data[ pos->idx ];
    }
}


fr_s fr_first( fr_t pos )
{
    fr_s tmp = *pos;

    while ( tmp.seg->prev )
        tmp.seg = tmp.seg->prev;
    tmp.idx = 0;

    return tmp;
}


fr_s fr_last( fr_t pos )
{
    fr_s tmp = *pos;

    while ( tmp.seg->next )
        tmp.seg = tmp.seg->next;
    tmp.idx = tmp.seg->used - 1;

    return tmp;
}


void fr_to_first( fr_t pos )
{
    *pos = fr_first( pos );
}


void fr_to_last( fr_t pos )
{
    *pos = fr_last( pos );
}


int fr_at_first( fr_t pos )
{
    if ( pos->seg && pos->seg->prev == NULL && pos->idx == 0 )
        return fr_true;
    else
        return fr_false;
}


int fr_at_last( fr_t pos )
{
    if ( pos->seg && pos->seg->next == NULL && pos->idx == pos->seg->used - 1 )
        return fr_true;
    else
        return fr_false;
}


void* fr_item( fr_t pos )
{
    return pos->seg->data[ pos->idx ];
}


void* fr_item_at( fr_t pos, fr_size_t idx )
{
    if ( idx < pos->seg->used )
        return pos->seg->data[ idx ];
    else
        return NULL;
}


int fr_is_valid( fr_t pos )
{
    return ( pos->seg != NULL );
}



/* ------------------------------------------------------------
 * Framer node:
 * ------------------------------------------------------------ */

fn_t fn_new_sized( fr_size_t size )
{
    fn_t node;

    assert( size >= FR_SEG_MIN );

    node = fr_malloc( FR_NODE_SIZE + size * FR_ITEM_SIZE );
    node->prev = NULL;
    node->next = NULL;
    node->used = 0;
    node->data[ 0 ] = NULL;

    return node;
}


fn_t fn_first( fn_t node )
{
    while ( node->prev != NULL )
        node = node->prev;

    return node;
}


fn_t fn_append( fn_t anchor, fn_t node )
{
    if ( anchor->next == NULL ) {
        node->prev = anchor;
        anchor->next = node;
    } else {
        anchor->next->prev = node;
        node->next = anchor->next;
        anchor->next = node;
        node->prev = anchor;
    }

    return node;
}


fn_t fn_update( fn_t node )
{
    fn_t ret;

    if ( node->prev && node->next ) {
        node->prev->next = node->next;
        node->next->prev = node->prev;
        ret = node->next;
    } else if ( node->prev ) {
        node->prev->next = NULL;
        ret = node->prev;
    } else if ( node->next ) {
        node->next->prev = NULL;
        ret = node->next;
    } else {
        ret = NULL;
    }

    return ret;
}


fn_t fn_delete( fn_t node )
{
    fn_t ret;

    ret = fn_update( node );
    fr_free( node );

    return ret;
}


/* ------------------------------------------------------------
 * Internal functions:
 * ------------------------------------------------------------ */

static fn_t alloc_node( fr_t pos )
{
    if ( pos->mem )
        return memapi_alloc( pos );
    else
        return fn_new_sized( pos->size );
}
