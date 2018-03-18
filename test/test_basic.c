/**
 * @file   test_basic.c
 * @author Tero Isannainen <tero.isannainen@gmail.com>
 * @date   Sun Mar 11 08:35:02 2018
 *
 * @brief  Test for Framer.
 *
 */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "unity.h"
#include "framer.h"



void gdb_breakpoint( void )
{
}


fn_t my_mem_api_alloc( fr_t pos, void* env )
{
    TEST_ASSERT_EQUAL( NULL, env );
    return fn_new_sized( pos->size );
}


fn_t my_mem_api_free( fr_t pos, void* env )
{
    TEST_ASSERT_EQUAL( NULL, env );
    pos->seg = fn_delete( pos->seg );
    return NULL;
}


void check_pos( fr_t ref, fr_t pos )
{
    TEST_ASSERT_EQUAL( ref->seg, pos->seg );
    TEST_ASSERT_EQUAL( ref->idx, pos->idx );
    TEST_ASSERT_EQUAL( ref->size, pos->size );
    TEST_ASSERT_EQUAL( ref->icnt, pos->icnt );
    TEST_ASSERT_EQUAL( ref->ncnt, pos->ncnt );
    TEST_ASSERT_EQUAL( ref->mem, pos->mem );
}


void test_create_and_destroy( void )
{
    fr_t pos;

    pos = fr_create();
    TEST_ASSERT_EQUAL( 0, fr_index( pos ) );
    TEST_ASSERT_EQUAL( 0, fr_used( pos ) );
    TEST_ASSERT_EQUAL( NULL, fr_item( pos ) );
    TEST_ASSERT_EQUAL( FR_SEG_DEFAULT, fr_size( pos ) );
    fr_destroy( pos );


    pos = fr_create_sized( 128 );
    TEST_ASSERT_EQUAL( 0, pos->idx );
    TEST_ASSERT_EQUAL( 0, pos->seg->used );
    TEST_ASSERT_EQUAL( NULL, fr_item( pos ) );
    TEST_ASSERT_EQUAL( 128, pos->size );
    fr_destroy( pos );

    fr_s pos2;
    fr_mem_t saved;
    fr_pos_new_with_mem( &pos2, FR_SEG_MIN, my_mem_api_alloc, my_mem_api_free, NULL );
    saved = pos2.mem;
    pos = fr_pos_new_with_mem( NULL, FR_SEG_MIN, my_mem_api_alloc, my_mem_api_free, NULL );
    pos2.mem = pos->mem;
    check_pos( &pos2, pos );
    fr_free( saved );
    
    pos = fr_create_using( pos );
    TEST_ASSERT_EQUAL( 0, pos->idx );
    TEST_ASSERT_EQUAL( 0, pos->seg->used );
    TEST_ASSERT_EQUAL( NULL, fr_item( pos ) );
    TEST_ASSERT_EQUAL( FR_SEG_MIN, pos->size );

    int items[ 27 ];
    int i;
    int ret;

    for ( int cnt = 27; cnt >= 1; cnt-- ) {

        /* set */
        i = 0;
        while ( i < cnt ) {
            items[ i ] = i;
            fr_append( pos, &( items[ i ] ) );
            i++;
            TEST_ASSERT_EQUAL( i, fr_length( pos ) );
        }

        TEST_ASSERT_EQUAL( ( ( cnt - 1 ) / FR_SEG_MIN ) + 1, fr_node_count( pos ) );

        /* compare */
        i = 0;
        *pos = fr_first( pos );
        while ( i < cnt ) {
            TEST_ASSERT_EQUAL( i, *( fr_cur( pos, int* ) ) );
            fr_next( pos );
            i++;
        }

        /* delete */
        i = 0;
        *pos = fr_first( pos );
        while ( i < cnt ) {
            ret = *( (int*)fr_delete( pos ) );
            TEST_ASSERT_EQUAL( i, ret );
            i++;
        }
        TEST_ASSERT_EQUAL( 0, pos->idx );
        TEST_ASSERT_EQUAL( 0, pos->seg->used );
        TEST_ASSERT_EQUAL( NULL, fr_item( pos ) );


        /* set */
        i = 0;
        while ( i < cnt ) {
            items[ i ] = i;
            fr_insert( pos, &( items[ i ] ) );
            i++;
        }

        /* compare */
        *pos = fr_first( pos );
        while ( i < cnt ) {
            TEST_ASSERT_EQUAL( i, ( cnt - 1 ) - *( fr_cur( pos, int* ) ) );
            fr_next( pos );
            i++;
        }

        /* delete */
        i = 0;
        *pos = fr_last( pos );
        while ( i < cnt ) {
            fr_delete( pos );
            i++;
        }
        TEST_ASSERT_EQUAL( 0, pos->idx );
        TEST_ASSERT_EQUAL( 0, pos->seg->used );
        TEST_ASSERT_EQUAL( NULL, fr_item( pos ) );
    }

    fr_destroy( pos );
}


void test_pattern( void )
{
    fr_t  pos;
    int   i;
    int   cnt;
    void* item;

    int limit = 3 * FR_SEG_MIN;
    int items[ limit + 1 ];

    for ( int i = 0; i < limit + 1; i++ )
        items[ i ] = i;

    pos = fr_create_sized( FR_SEG_MIN );

    for ( cnt = 1; cnt < limit; cnt++ ) {

        /* push */
        i = 0;
        while ( i < cnt ) {
            fr_push( pos, &( items[ i ] ) );
            i++;
            TEST_ASSERT_EQUAL( i, fr_length( pos ) );
        }

        TEST_ASSERT_EQUAL( ( ( cnt - 1 ) / FR_SEG_MIN ) + 1, fr_node_count( pos ) );

        /* compare */
        i = 0;
        fr_to_first( pos );
        TEST_ASSERT_EQUAL( fr_length( pos ), fr_tail_length( pos ) );
        while ( i < cnt ) {
            TEST_ASSERT_EQUAL( items[ i ], *( fr_cur( pos, int* ) ) );
            fr_next( pos );
            i++;
        }

        /* delete & insert */
        *pos = fr_first( pos );
        for ( int j = 0; j < cnt; j++ ) {
            item = fr_delete( pos );
            TEST_ASSERT_EQUAL( *( (int*)item ), items[ j ] );
            if ( j == cnt - 1 )
                fr_append( pos, item );
            else
                fr_insert( pos, item );
            fr_next( pos );
        }

        /* pop */
        fr_to_last( pos );
        for ( int j = 0; j < cnt; j++ ) {
            item = fr_pop( pos );
            TEST_ASSERT_EQUAL( *( (int*)item ), items[ ( cnt - 1 ) - j ] );
        }
    }

    fr_destroy( pos );
}


void init_hit( int* hits, int limit )
{
    for ( int i = 0; i < limit; i++ ) {
        hits[ i ] = 0;
    }
}

int all_hit( int* hits, int limit )
{
    for ( int i = 0; i < limit; i++ ) {
        if ( hits[ i ] == 0 )
            return 0;
    }
    return 1;
}

int rand_within( int limit )
{
    if ( limit > 0 )
        return ( rand() % limit );
    else
        return 0;
}


void test_random( void )
{
    fr_t pos;
    int  i;
    int  cnt;
    int  move;
    int  ret;

    int limit = 3 * FR_SEG_MIN;
    int items[ limit ];
    int hits[ limit ];

    srand( 1234 );

    for ( i = 0; i < limit; i++ ) {
        items[ i ] = i;
    }

    pos = fr_create_sized( FR_SEG_MIN );

    init_hit( hits, limit );

    for ( ;; ) {

        cnt = rand_within( limit );

        for ( int j = 0; j < cnt; j++ ) {
            *pos = fr_first( pos );
            move = rand_within( pos->icnt );
            if ( pos->icnt > 0 ) {
                TEST_ASSERT( pos->icnt - 1 >= move );
            }
            ret = fr_next_n( pos, move );
            TEST_ASSERT_EQUAL( move, ret );
            fr_insert( pos, &( items[ i ] ) );
            TEST_ASSERT_EQUAL( j + 1, pos->icnt );
        }

        for ( int j = 0; j < cnt; j++ ) {
            *pos = fr_first( pos );
            move = rand_within( pos->icnt );
            if ( pos->icnt > 0 ) {
                TEST_ASSERT( pos->icnt - 1 >= move );
            }
            ret = fr_next_n( pos, move );
            TEST_ASSERT_EQUAL( move, ret );
            fr_delete( pos );

            TEST_ASSERT_EQUAL( ( cnt - 1 ) - j, pos->icnt );
        }

        hits[ cnt ] = 1;

        if ( all_hit( hits, limit ) )
            break;
    }

    fr_destroy( pos );


    int seg_size[ 2 ] = { FR_SEG_MIN, FR_SEG_MIN + 1 };

    for ( int rnd = 0; rnd < 2; rnd++ ) {

        pos = fr_create_sized( 3 * seg_size[ rnd ] );

        cnt = rand_within( 20 * seg_size[ rnd ] );

        for ( int j = 0; j < cnt; j++ ) {
            *pos = fr_first( pos );
            move = rand_within( pos->icnt );
            if ( pos->icnt > 0 ) {
                TEST_ASSERT( pos->icnt - 1 >= move );
            }
            ret = fr_next_n( pos, move );
            TEST_ASSERT_EQUAL( move, ret );
            fr_insert( pos, &( items[ i ] ) );
            TEST_ASSERT_EQUAL( j + 1, pos->icnt );
        }

        for ( int j = 0; j < cnt; j++ ) {
            *pos = fr_first( pos );
            move = rand_within( pos->icnt );
            if ( pos->icnt > 0 ) {
                TEST_ASSERT( pos->icnt - 1 >= move );
            }
            ret = fr_next_n( pos, move );
            TEST_ASSERT_EQUAL( move, ret );
            fr_delete_even( pos );
            TEST_ASSERT_EQUAL( ( cnt - 1 ) - j, pos->icnt );
        }
    }

    fr_destroy( pos );
}


void test_corners( void )
{
    fr_t  pos;
    fr_s  ref;
    void* item;
    fn_t  seg;

    int   limit = 3 * FR_SEG_MIN;
    void* items[ limit ];

    for ( int i = 0; i < limit; i++ )
        items[ i ] = &( items[ i ] );

    pos = fr_create_sized( FR_SEG_MIN );

    TEST_ASSERT_EQUAL( pos->seg, pos->seg );
    TEST_ASSERT_EQUAL( 0, pos->idx );
    TEST_ASSERT_EQUAL( FR_SEG_MIN, pos->size );
    TEST_ASSERT_EQUAL( 0, pos->icnt );
    TEST_ASSERT_EQUAL( 1, pos->ncnt );
    TEST_ASSERT_EQUAL( NULL, pos->mem );

    TEST_ASSERT_EQUAL( 0, pos->seg->used );
    TEST_ASSERT_EQUAL( NULL, pos->seg->prev );
    TEST_ASSERT_EQUAL( NULL, pos->seg->next );

    ref = *pos;

    check_pos( &ref, pos );

    /* .... -> x...
     * ^       ^
     */
    fr_push( pos, items[ 0 ] );
    ref.icnt = 1;
    check_pos( &ref, pos );
    TEST_ASSERT_EQUAL( 1, pos->seg->used );
    TEST_ASSERT_EQUAL( NULL, pos->seg->prev );
    TEST_ASSERT_EQUAL( NULL, pos->seg->next );

    /* x... -> ....
     * ^       ^
     */
    item = fr_pop( pos );
    TEST_ASSERT_EQUAL( item, items[ 0 ] );
    ref.icnt = 0;
    check_pos( &ref, pos );
    TEST_ASSERT_EQUAL( 0, pos->seg->used );
    TEST_ASSERT_EQUAL( NULL, pos->seg->prev );
    TEST_ASSERT_EQUAL( NULL, pos->seg->next );

    /* .... -> xxx.
     * ^         ^
     */
    for ( int i = 0; i < 3; i++ ) {
        fr_push( pos, items[ i ] );
    }
    ref.icnt = 3;
    ref.idx = 2;
    check_pos( &ref, pos );
    TEST_ASSERT_EQUAL( 3, pos->seg->used );
    TEST_ASSERT_EQUAL( NULL, pos->seg->prev );
    TEST_ASSERT_EQUAL( NULL, pos->seg->next );

    /* xxx. -> xx..
     *   ^      ^
     */
    item = fr_pop( pos );
    TEST_ASSERT_EQUAL( item, items[ 2 ] );
    ref.icnt = 2;
    ref.idx = 1;
    check_pos( &ref, pos );
    TEST_ASSERT_EQUAL( 2, pos->seg->used );
    TEST_ASSERT_EQUAL( NULL, pos->seg->prev );
    TEST_ASSERT_EQUAL( NULL, pos->seg->next );

    /* xx.. -> xxxx
     *  ^         ^
     */
    fr_push( pos, items[ 2 ] );
    fr_push( pos, items[ 3 ] );
    ref.icnt = 4;
    ref.idx = 3;
    check_pos( &ref, pos );
    TEST_ASSERT_EQUAL( 4, pos->seg->used );
    TEST_ASSERT_EQUAL( NULL, pos->seg->prev );
    TEST_ASSERT_EQUAL( NULL, pos->seg->next );

    /* xxxx-.... -> xxxx-x...
     *    ^              ^
     */
    seg = ref.seg;
    fr_push( pos, items[ 4 ] );
    ref.icnt = 5;
    ref.idx = 0;
    ref.ncnt = 2;
    ref.seg = pos->seg;
    check_pos( &ref, pos );
    TEST_ASSERT_EQUAL( 1, pos->seg->used );
    TEST_ASSERT_EQUAL( seg, pos->seg->prev );
    TEST_ASSERT_EQUAL( NULL, pos->seg->next );

    /* xxxx-x... -> xxxx-xx...
     *      ^             ^
     */
    fr_push( pos, items[ 5 ] );
    ref.icnt = 6;
    ref.idx = 1;
    ref.ncnt = 2;
    check_pos( &ref, pos );
    TEST_ASSERT_EQUAL( 2, pos->seg->used );
    TEST_ASSERT_EQUAL( seg, pos->seg->prev );
    TEST_ASSERT_EQUAL( NULL, pos->seg->next );

    /* xxxx-xx.. -> xxxx-x...
     *       ^           ^
     */
    item = fr_pop( pos );
    TEST_ASSERT_EQUAL( item, items[ 5 ] );
    ref.icnt = 5;
    ref.idx = 0;
    ref.ncnt = 2;
    check_pos( &ref, pos );
    TEST_ASSERT_EQUAL( 1, pos->seg->used );
    TEST_ASSERT_EQUAL( seg, pos->seg->prev );
    TEST_ASSERT_EQUAL( NULL, pos->seg->next );

    /* xxxx-x... -> xxx5-xx..
     *      ^          ^
     */
    seg = pos->seg;
    fr_prev( pos );
    fr_insert( pos, items[ 5 ] );
    ref.icnt = 6;
    ref.idx = 3;
    ref.ncnt = 2;
    ref.seg = pos->seg;
    check_pos( &ref, pos );
    TEST_ASSERT_EQUAL( 4, pos->seg->used );
    TEST_ASSERT_EQUAL( NULL, pos->seg->prev );
    TEST_ASSERT_EQUAL( seg, pos->seg->next );

    /* xxx5-xx.. -> xxx5-xxx.
     *    ^              ^
     */
    fr_append( pos, items[ 5 ] );
    ref.icnt = 7;
    ref.idx = 0;
    ref.ncnt = 2;
    ref.seg = pos->seg;
    check_pos( &ref, pos );
    TEST_ASSERT_EQUAL( 3, pos->seg->used );

    /* xxx5-xxx. -> xxx5-xxxx
     *      ^             ^
     */
    fr_append( pos, items[ 5 ] );
    ref.icnt = 8;
    ref.idx = 1;
    ref.ncnt = 2;
    check_pos( &ref, pos );
    TEST_ASSERT_EQUAL( 4, pos->seg->used );
    TEST_ASSERT_EQUAL( 3, fr_tail_length( pos ) );

    /* Extra movements. */
    TEST_ASSERT_EQUAL( 0, fr_next_n( pos, 4 ) );
    fr_next( pos );

    TEST_ASSERT_EQUAL( 5, fr_prev_n( pos, 5 ) );
    TEST_ASSERT_EQUAL( 1, fr_prev_n( pos, 1 ) );
    TEST_ASSERT_EQUAL( 0, fr_prev_n( pos, 1 ) );
    TEST_ASSERT_EQUAL( 5, fr_next_n( pos, 5 ) );
    TEST_ASSERT_EQUAL( 3, fr_prev_n( pos, 3 ) );


    TEST_ASSERT_EQUAL( items[ 0 ], fr_item_at( pos, 0 ) );
    TEST_ASSERT_EQUAL( NULL, fr_item_at( pos, 10 ) );


    fr_t dup;
    dup = fr_pos_dup( pos );
    TEST_ASSERT_EQUAL( dup->seg, pos->seg );
    TEST_ASSERT_EQUAL( dup->idx, pos->idx );
    TEST_ASSERT_EQUAL( dup->size, pos->size );
    TEST_ASSERT_EQUAL( dup->icnt, pos->icnt );
    TEST_ASSERT_EQUAL( dup->ncnt, pos->ncnt );
    TEST_ASSERT_EQUAL( dup->mem, pos->mem );

    dup = fr_pos_del( dup );
    TEST_ASSERT_EQUAL( NULL, dup );
    dup = fr_pos_new( pos->size );
    TEST_ASSERT_NOT_EQUAL( dup->seg, pos->seg );
    TEST_ASSERT_NOT_EQUAL( dup->idx, pos->idx );
    TEST_ASSERT_EQUAL( dup->size, pos->size );
    TEST_ASSERT_NOT_EQUAL( dup->icnt, pos->icnt );
    TEST_ASSERT_NOT_EQUAL( dup->ncnt, pos->ncnt );
    TEST_ASSERT_EQUAL( dup->mem, pos->mem );

    fr_pos_cpy( pos, dup );
    TEST_ASSERT_EQUAL( dup->seg, pos->seg );
    TEST_ASSERT_EQUAL( dup->idx, pos->idx );
    TEST_ASSERT_EQUAL( dup->size, pos->size );
    TEST_ASSERT_EQUAL( dup->icnt, pos->icnt );
    TEST_ASSERT_EQUAL( dup->ncnt, pos->ncnt );
    TEST_ASSERT_EQUAL( dup->mem, pos->mem );

    dup = fr_pos_del( dup );

    fr_to_last( pos );
    fr_destroy( pos );


    /* Test fr_even() corners. */
    pos = fr_create_sized( FR_SEG_MIN );

    limit = 2 * FR_SEG_MIN;
    for ( int i = 0; i < limit - 2; i++ )
        fr_append( pos, &( items[ i ] ) );

    fr_to_first( pos );
    fr_next_n( pos, 1 );

    TEST_ASSERT_EQUAL( limit - 2, fr_length( pos ) );
    TEST_ASSERT_EQUAL( 2, fr_node_count( pos ) );

    fr_delete_even( pos );
    TEST_ASSERT_EQUAL( 2, fr_node_count( pos ) );
    fr_delete_even( pos );

    TEST_ASSERT_EQUAL( limit - 4, fr_length( pos ) );
    TEST_ASSERT_EQUAL( 1, fr_node_count( pos ) );
    TEST_ASSERT_EQUAL( 1, pos->idx );

    fr_destroy( pos );
}




int find_comp( void* a, void* b )
{
    int ia = *( (int*)a );
    int ib = *( (int*)b );

    if ( ia > ib )
        return 1;
    else if ( ia < ib )
        return -1;
    else
        return 0;
}


void test_find( void )
{
    fr_t pos;
    fr_s ref;
    int  i;

    int limit = 32 * FR_SEG_MIN;

    int items[ limit + 1 ];

    pos = fr_create_sized( FR_SEG_MIN );

    for ( i = 0; i < limit; i++ ) {
        items[ i ] = i;
        fr_append( pos, &( items[ i ] ) );
    }
    items[ limit ] = limit;

    *pos = fr_first( pos );

    /* Item is found. */
    ref = fr_find( pos, &( items[ 0 ] ) );
    TEST_ASSERT_TRUE( fr_at_first( &ref ) );
    TEST_ASSERT_FALSE( fr_at_last( &ref ) );
    TEST_ASSERT_TRUE( ref.seg == pos->seg );
    TEST_ASSERT_TRUE( ref.idx == pos->idx );

    /* Item is found. */
    ref = fr_find( pos, &( items[ 10 * FR_SEG_MIN ] ) );
    TEST_ASSERT_FALSE( fr_at_first( &ref ) );
    TEST_ASSERT_FALSE( fr_at_last( &ref ) );
    TEST_ASSERT_TRUE( ref.seg != pos->seg );
    TEST_ASSERT_TRUE( ref.idx == pos->idx );

    /* Item is found. */
    ref = fr_find( pos, &( items[ 127 ] ) );
    TEST_ASSERT_FALSE( fr_at_first( &ref ) );
    TEST_ASSERT_TRUE( fr_at_last( &ref ) );
    TEST_ASSERT_EQUAL( NULL, ref.seg->next );
    TEST_ASSERT_TRUE( ref.seg->prev );

    /* Item is not find. */
    ref = fr_find( pos, &( items[ limit ] ) );
    TEST_ASSERT_FALSE( fr_is_valid( &ref ) );

    /* Item is found. */
    ref = fr_find_with( pos, &( items[ FR_SEG_MIN - 1 ] ), find_comp );
    TEST_ASSERT_FALSE( fr_at_last( &ref ) );
    TEST_ASSERT_EQUAL( NULL, ref.seg->prev );
    TEST_ASSERT_TRUE( ref.seg->next );

    /* Item is not found. */
    ref = fr_find_with( pos, &( items[ limit ] ), find_comp );
    TEST_ASSERT_FALSE( fr_is_valid( &ref ) );

    /* Item is found. */
    ref = fr_find_sorted_with( pos, &( items[ 2 * FR_SEG_MIN - 1 ] ), find_comp );
    TEST_ASSERT_FALSE( fr_at_last( &ref ) );
    TEST_ASSERT_TRUE( ref.seg->prev );
    TEST_ASSERT_TRUE( ref.seg->next );

    /* Item is not found. */
    *pos = fr_first( pos );
    ref = fr_find_sorted_with( pos, &( items[ limit ] ), find_comp );
    TEST_ASSERT_FALSE( fr_is_valid( &ref ) );

    fr_destroy( pos );
}


void test_positions( void )
{
    fr_t pos;
    int  i;

    int limit = 3 * FR_SEG_MIN;
    int items[ limit + 1 ];

    pos = fr_create_sized( FR_SEG_MIN );
    for ( i = 0; i < limit; i++ ) {
        items[ i ] = i;
        fr_push( pos, &( items[ i ] ) );
    }

    *pos = fr_first( pos );
    TEST_ASSERT_TRUE( fr_at_first( pos ) );
    TEST_ASSERT_FALSE( fr_at_last( pos ) );
    TEST_ASSERT_EQUAL( limit, fr_tail_length( pos ) );

    fr_next( pos );
    TEST_ASSERT_FALSE( fr_at_first( pos ) );
    TEST_ASSERT_FALSE( fr_at_last( pos ) );
    TEST_ASSERT_EQUAL( limit - 1, fr_tail_length( pos ) );

    *pos = fr_first( pos );
    TEST_ASSERT_TRUE( fr_at_first( pos ) );
    TEST_ASSERT_FALSE( fr_at_last( pos ) );
    TEST_ASSERT_EQUAL( limit, fr_tail_length( pos ) );

    fr_next_n( pos, limit - 1 );
    TEST_ASSERT_FALSE( fr_at_first( pos ) );
    TEST_ASSERT_TRUE( fr_at_last( pos ) );
    TEST_ASSERT_EQUAL( 1, fr_tail_length( pos ) );

    *pos = fr_first( pos );
    TEST_ASSERT_TRUE( fr_at_first( pos ) );
    TEST_ASSERT_FALSE( fr_at_last( pos ) );
    TEST_ASSERT_EQUAL( limit, fr_tail_length( pos ) );

    fr_next( pos );
    fr_next_n( pos, limit - 2 );
    TEST_ASSERT_FALSE( fr_at_first( pos ) );
    TEST_ASSERT_TRUE( fr_at_last( pos ) );
    TEST_ASSERT_EQUAL( 1, fr_tail_length( pos ) );

    fr_next( pos );
    TEST_ASSERT_FALSE( fr_at_first( pos ) );
    TEST_ASSERT_TRUE( fr_at_last( pos ) );
    TEST_ASSERT_EQUAL( 1, fr_tail_length( pos ) );

    *pos = fr_first( pos );
    fr_next_n( pos, limit );
    TEST_ASSERT_TRUE( fr_at_first( pos ) );
    TEST_ASSERT_FALSE( fr_at_last( pos ) );
    TEST_ASSERT_EQUAL( limit, fr_tail_length( pos ) );


    *pos = fr_last( pos );
    TEST_ASSERT_FALSE( fr_at_first( pos ) );
    TEST_ASSERT_TRUE( fr_at_last( pos ) );
    TEST_ASSERT_EQUAL( 1, fr_tail_length( pos ) );

    fr_prev( pos );
    TEST_ASSERT_FALSE( fr_at_first( pos ) );
    TEST_ASSERT_FALSE( fr_at_last( pos ) );
    TEST_ASSERT_EQUAL( 2, fr_tail_length( pos ) );

    *pos = fr_last( pos );
    TEST_ASSERT_FALSE( fr_at_first( pos ) );
    TEST_ASSERT_TRUE( fr_at_last( pos ) );
    TEST_ASSERT_EQUAL( 1, fr_tail_length( pos ) );

    fr_prev_n( pos, limit - 1 );
    TEST_ASSERT_TRUE( fr_at_first( pos ) );
    TEST_ASSERT_FALSE( fr_at_last( pos ) );
    TEST_ASSERT_EQUAL( limit, fr_tail_length( pos ) );

    *pos = fr_last( pos );
    fr_prev( pos );
    fr_prev_n( pos, limit - 2 );
    TEST_ASSERT_TRUE( fr_at_first( pos ) );
    TEST_ASSERT_FALSE( fr_at_last( pos ) );
    TEST_ASSERT_EQUAL( limit, fr_tail_length( pos ) );

    fr_prev( pos );
    TEST_ASSERT_TRUE( fr_at_first( pos ) );
    TEST_ASSERT_FALSE( fr_at_last( pos ) );
    TEST_ASSERT_EQUAL( limit, fr_tail_length( pos ) );

    *pos = fr_last( pos );
    fr_prev_n( pos, limit );
    TEST_ASSERT_FALSE( fr_at_first( pos ) );
    TEST_ASSERT_TRUE( fr_at_last( pos ) );
    TEST_ASSERT_EQUAL( 1, fr_tail_length( pos ) );

    fr_destroy( pos );
}



void test_pack( void )
{
    fr_t pos;
    int  limit = 30 * ( FR_SEG_MIN * 2 );
    int  items[ limit + 1 ];

    pos = fr_create_sized( ( FR_SEG_MIN * 2 ) );

    for ( int i = 0; i < limit; i++ ) {
        items[ i ] = i;
        fr_push( pos, &( items[ i ] ) );
    }
    items[ limit ] = limit;

    TEST_ASSERT_EQUAL( 2 * 30 * FR_SEG_MIN, pos->icnt );
    TEST_ASSERT_EQUAL( 30, pos->ncnt );

    *pos = fr_first( pos );

    fr_next_n( pos, FR_SEG_MIN - 1 );

    while ( fr_next_n( pos, 3 * FR_SEG_MIN / 2 ) ) {
        fr_insert( pos, &( items[ limit ] ) );
    }

    TEST_ASSERT_EQUAL( 2 * 30 * FR_SEG_MIN + 47, pos->icnt );
    TEST_ASSERT_EQUAL( 30 + 18, pos->ncnt );

    *pos = fr_first( pos );

    fr_s tmp = *pos;
    fr_s tmp2 = *pos;
    int  ret;

    tmp = fr_last( &tmp );
    tmp2 = fr_last( &tmp );
    ret = fr_pack_range( &tmp, NULL, 3 * FR_SEG_MIN / 2 );
    TEST_ASSERT_EQUAL( 0, ret );
    ret = fr_pack_range( &tmp, NULL, 3 * FR_SEG_MIN );
    TEST_ASSERT_EQUAL( 0, ret );

    tmp2.seg = tmp.seg->prev;
    ret = fr_pack_range( &tmp2, &tmp, 3 * FR_SEG_MIN / 2 );
    TEST_ASSERT_EQUAL( 0, ret );

    tmp2.seg = tmp.seg->prev;
    ret = fr_pack_range( &tmp2, &tmp, 3 * FR_SEG_MIN / 2 );
    TEST_ASSERT_EQUAL( 0, ret );

    ret = fr_pack_range( pos, NULL, 3 * FR_SEG_MIN / 2 );
    TEST_ASSERT_EQUAL( 1, ret );
    TEST_ASSERT_EQUAL( 2 * 30 * FR_SEG_MIN + 47, pos->icnt );
    TEST_ASSERT_EQUAL( 30 + 18, pos->ncnt );

    ret = fr_pack_range( pos, NULL, 3 * FR_SEG_MIN / 2 );
    TEST_ASSERT_EQUAL( 0, ret );

    fr_destroy( pos );


    pos = fr_create_sized( ( FR_SEG_MIN * 2 ) );

    for ( int i = 0; i < limit; i++ ) {
        items[ i ] = i;
        fr_push( pos, &( items[ i ] ) );
    }
    items[ limit ] = limit;

    *pos = fr_first( pos );

    fr_next_n( pos, FR_SEG_MIN - 1 );

    while ( fr_next_n( pos, 3 * FR_SEG_MIN / 2 ) ) {
        fr_insert( pos, &( items[ limit ] ) );
    }

    *pos = fr_first( pos );
    tmp = *pos;
    fr_next_n( &tmp, 10 * FR_SEG_MIN );
    ret = fr_pack_range( pos, &tmp, 3 * FR_SEG_MIN / 2 );
    TEST_ASSERT_EQUAL( 1, ret );

    fr_destroy( pos );
}


void test_iteration( void )
{
    fr_t pos;
    int  limit = FR_SEG_MIN * 4;
    int  items[ limit + 1 ];
    int* item;

    pos = fr_create();

    for ( int i = 0; i < limit; i++ ) {
        items[ i ] = i;
        fr_push( pos, &( items[ i ] ) );
    }

    int i = 0;
    fr_s iter = fr_first( pos );

    fr_each( &iter, item, int* ) {
        TEST_ASSERT_EQUAL( items[ i ], *item );
        i++;
    }

    fr_destroy( pos );
}
