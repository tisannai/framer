#ifndef FRAMER_H
#define FRAMER_H


/**
 * @file   framer.h
 * @author Tero Isannainen <tero.isannainen@gmail.com>
 * @date   Sun Mar 11 08:35:02 2018
 *
 * @brief  Framer - Unrolled Doubly Linked List library
 *
 */


/** Framer library version. */
extern const char* framer_version;


/* ------------------------------------------------------------
 * Dimensions:
 * ------------------------------------------------------------ */

#ifndef FR_CACHE_LINE_SIZE
/** Default to 64 byte cache line size. */
#define FR_CACHE_LINE_SIZE 64
#endif

/** Alignment directive. */
#define FR_CACHE_LINE_ALIGN __attribute__( ( aligned( FR_CACHE_LINE_SIZE ) ) )

/** Size of Framer node without data segment. */
#define FR_NODE_SIZE ( 2 * sizeof( void* ) + sizeof( fr_size_t ) )

/** Size of item. */
#define FR_ITEM_SIZE ( sizeof( void* ) )

/** Mininum size of data segment. */
#define FR_SEG_MIN 4

/** Default size for segment, i.e. fit complete node to cache line. */
#define FR_SEG_DEFAULT ( ( FR_CACHE_LINE_SIZE - FR_NODE_SIZE ) / FR_ITEM_SIZE )



/* ------------------------------------------------------------
 * Type definitions:
 * ------------------------------------------------------------ */

/** Unsigned size type. */
typedef int64_t fr_size_t;


/**
 * Framer node.
 */
struct fn_struct_s
{
    struct fn_struct_s* prev;      /**< Previous node. */
    struct fn_struct_s* next;      /**< Next node. */
    fr_size_t           used;      /**< Used count for data. */
    void*               data[ 0 ]; /**< Pointer array. */
} FR_CACHE_LINE_ALIGN;
typedef struct fn_struct_s fn_s; /**< Node struct. */
typedef fn_s*              fn_t; /**< Node pointer. */
typedef fn_t*              fn_p; /**< Node pointer reference. */


struct fr_mem_struct_s;

/**
 * Framer position.
 */
struct fr_struct_s
{
    fn_t                    seg;  /**< Segment. */
    fr_size_t               idx;  /**< Segment index. */
    fr_size_t               size; /**< Segment size. */
    fr_size_t               icnt; /**< Framer item count. */
    fr_size_t               ncnt; /**< Framer node count. */
    struct fr_mem_struct_s* mem;  /**< Memory API. */
} FR_CACHE_LINE_ALIGN;
typedef struct fr_struct_s fr_s; /**< Position struct. */
typedef fr_s*              fr_t; /**< Position pointer. */
typedef fr_t*              fr_p; /**< Position pointer reference. */


/**
 * Framer data compare.
 *
 * * Return  1 a is greater than b.
 * * Return  0 on match.
 * * Return -1 a is smaller than b.
 */
typedef int ( *fr_cmp_f )( void* a, void* b );



/* ------------------------------------------------------------
 * Memory API:
 * ------------------------------------------------------------ */

/**
 * Framer memory pooler API reserve/release function type.
 *
 * * pos : Framer position.
 * * env : Memory pooler environment (data).
 */
typedef fn_t ( *fr_mem_f )( fr_t pos, void* env );


/**
 * Framer memory pooler API interface.
 */
struct fr_mem_struct_s
{
    fr_mem_f alloc; /**< Reserve function. */
    fr_mem_f free;  /**< Release function. */
    void*    env;   /**< Environment. */
};
typedef struct fr_mem_struct_s fr_mem_s; /**< Pooler struct. */
typedef fr_mem_s*              fr_mem_t; /**< Pooler pointer. */



#ifdef FRAMER_MEM_API

/*
 * FRAMER_MEM_API allows to use custom memory allocation functions,
 * instead of the default: fr_malloc, fr_free, fr_realloc.
 *
 * If FRAMER_MEM_API is used, the user must provide implementation for the
 * below functions and they must be compatible with malloc etc.
 */

extern void* fr_malloc( size_t size );
extern void fr_free( void* ptr );
extern void* fr_realloc( void* ptr, size_t size );

#else

/* Default to common memory management functions. */

/** Reserve memory. */
#define fr_malloc malloc

/** Release memory. */
#define fr_free free

/** Re-reserve memory. */
#define fr_realloc realloc

#endif



/* ------------------------------------------------------------
 * Framer access macros:
 * ------------------------------------------------------------ */

/** Item at Position with cast from void* to target type. */
#define fr_cur( pos, cast ) ( ( cast )( ( pos )->seg->data[ ( pos )->idx ] ) )


/**
 * Framer item iterator:
 *
 * * iter  : Framer iterator
 * * item  : Item variable
 * * cast  : Item variable type
 *
 */
#define fr_each( iter, item, cast ) \
    for ( item = (cast)fr_item( iter ); ( iter )->seg; item = (cast)fr_next_item( iter ) )


/* ------------------------------------------------------------
 * Framer access:
 * ------------------------------------------------------------ */

/**
 * Create Framer with default segment size.
 *
 * Initial Framer Node is created and associated to Framer
 * Position. Position will refer to start of Framer chain.
 *
 * @return Position.
 */
fr_t fr_create( void );


/**
 * Create Framer with given segment size.
 *
 * See: fr_create().
 *
 * @param size Framer segment size.
 *
 * @return Position.
 */
fr_t fr_create_sized( fr_size_t size );


/**
 * Create Framer based on Position.
 *
 * @param pos Position.
 *
 * @return Position.
 */
fr_t fr_create_using( fr_t pos );


/**
 * Destroy Framer.
 *
 * Everything regarding Framer and Position is destroyed.
 *
 * @param pos Position.
 *
 * @return NULL
 */
fr_t fr_destroy( fr_t pos );


/**
 * Insert item to current Position.
 *
 * Segment data is shifted right, if necessary. If segment is full, a
 * new segment is created including the tail from current Position.
 *
 * @param pos  Position.
 * @param item Item.
 */
void fr_insert( fr_t pos, void* item );


/**
 * Append item after current Position.
 *
 * Position is first moved right, and data is then inserted. Position
 * is not moved if the item is first in Framer.
 *
 * @param pos  Position.
 * @param item Item.
 */
void fr_append( fr_t pos, void* item );


/**
 * Delete item from Framer.
 *
 * @param pos Position.
 *
 * @return Removed item.
 */
void* fr_delete( fr_t pos );


/**
 * Delete item from Framer and even items in peer Node segments.
 *
 * @param pos Position.
 *
 * @return Remove item.
 */
void* fr_delete_even( fr_t pos );


/**
 * Push item to back of Framer.
 *
 * NOTE: User must ensure that Position is at Framer end.
 *
 * @param pos  Position.
 * @param item Item.
 */
void fr_push( fr_t pos, void* item );


/**
 * Pop item from back of Framer.
 *
 * NOTE: User must ensure that Position is at Framer end.
 *
 * @param pos Position.
 *
 * @return Last item.
 */
void* fr_pop( fr_t pos );


/**
 * Even items in peer Node segments.
 *
 * fr_even() evens the items in peer segments, so that segments never
 * have less than half filled. It will combine two Node segments if
 * they fit in one. Otherwise it will fill from next segment (or from
 * previous) upto half for current segment.
 *
 * Return 2 if Node was delete, return 1 if evening occurred, and 0 if
 * nothing was done.
 *
 * @param pos Position.
 *
 * @return 2, 1, or 0.
 */
int fr_even( fr_t pos );


/**
 * Pack Node segments together.
 *
 * Pack starting from current Position upto given end or to end of
 * Framer. Limit defines the amount of items each segment should have
 * after packing. Packing does not occur if limit is too low.
 *
 * @param pos   Position.
 * @param end   End of packing (or NULL).
 * @param limit Segment fill count.
 *
 * @return 1 if packed, else 0.
 */
int fr_pack_range( fr_t pos, fr_t end, fr_size_t limit );


/**
 * Return item count of Framer.
 *
 * @param pos Position.
 *
 * @return Length.
 */
fr_size_t fr_length( fr_t pos );


/**
 * Return item count of Framer tail.
 *
 * @param pos Current Position.
 *
 * @return Tail length.
 */
fr_size_t fr_tail_length( fr_t pos );


/**
 * Return Node count of Framer.
 *
 * @param pos Position.
 *
 * @return Node count.
 */
fr_size_t fr_node_count( fr_t pos );


/**
 * Return segment size.
 * 
 * @param pos Position.
 * 
 * @return Size.
 */
fr_size_t fr_size( fr_t pos );


/**
 * Return current segment index.
 * 
 * @param pos Position.
 * 
 * @return Index.
 */
fr_size_t fr_index( fr_t pos );


/**
 * Return number of items in current segment.
 * 
 * @param pos Position.
 * 
 * @return Used count.
 */
fr_size_t fr_used( fr_t pos );


/**
 * Find item from Framer.
 *
 * Search from the current Position forward. Return Position where
 * item was found or invalid Position.
 *
 * @param pos  Search Position.
 * @param item Item to find.
 *
 * @return Position (or invalid Position).
 */
fr_s fr_find( fr_t pos, void* item );


/**
 * Find item from Framer using compare function.
 *
 * See: fr_find().
 *
 * @param pos  Search Position.
 * @param item Item to find.
 * @param comp Compare function.
 *
 * @return Position (or invalid Position).
 */
fr_s fr_find_with( fr_t pos, void* item, fr_cmp_f comp );


/**
 * Find item from Framer using compare function.
 *
 * NOTE: Assuming sorted set of data. This enables search to skip Node
 * segment content, if useful.
 *
 * @param pos  Search Position.
 * @param item Item to find.
 * @param comp Compare function.
 *
 * @return Position (or invalid Position).
 */
fr_s fr_find_sorted_with( fr_t pos, void* item, fr_cmp_f comp );




/* ------------------------------------------------------------
 * Framer Position:
 * ------------------------------------------------------------ */

/**
 * Create Position.
 *
 * @param size Framer segment size.
 *
 * @return Position.
 */
fr_t fr_pos_new( fr_size_t size );


/**
 * Create Position with custom memory management.
 *
 * Memory pooler interface is created and setup using given
 * arguments. If pos argument is NULL, a new Position is
 * created. Otherwise the given Position is used.
 *
 * @param pos   Position to use (or NULL for new).
 * @param size  Framer segment size.
 * @param alloc Memory reserve function.
 * @param free  Memory release function.
 * @param env   Memory pooler env.
 *
 * @return Position with memapi.
 */
fr_t fr_pos_new_with_mem( fr_t pos, fr_size_t size, fr_mem_f alloc, fr_mem_f free, void* env );


/**
 * Initialize Position with given segment size.
 *
 * @param pos  Position.
 * @param size Segment size.
 *
 * @return Position.
 */
fr_t fr_pos_init( fr_t pos, fr_size_t size );


/**
 * Delete Position.
 *
 * Position is only deleted, not Framer chain. Use fr_destroy() if all
 * should be destroyed.
 *
 * @param pos Position.
 *
 * @return NULL
 */
fr_t fr_pos_del( fr_t pos );


/**
 * Duplicate position.
 *
 * @param pos Pos to duplicate.
 *
 * @return Duplicated pos.
 */
fr_t fr_pos_dup( fr_t pos );


/**
 * Copy position.
 *
 * @param pos_a Copy from.
 * @param pos_b Copy to.
 */
void fr_pos_cpy( fr_t pos_a, fr_t pos_b );


/**
 * Move right N times.
 *
 * Return 0 if could not move N times, and Position is not updated.
 *
 * @param pos Position.
 * @param n   Step count.
 *
 * @return Steps taken.
 */
fr_size_t fr_next_n( fr_t pos, fr_size_t n );


/**
 * Move right.
 *
 * @param pos Position.
 *
 * @return 1 on success, else 0.
 */
fr_size_t fr_next( fr_t pos );


/**
 * Move left N times.
 *
 * Return 0 if could not move N times, and Position is not updated.
 *
 * @param pos Position.
 * @param n   Step count.
 *
 * @return Steps taken.
 */
fr_size_t fr_prev_n( fr_t pos, fr_size_t n );


/**
 * Move left.
 *
 * @param pos Position.
 *
 * @return 1 on success, else 0.
 */
fr_size_t fr_prev( fr_t pos );


/**
 * Move right and return next item.
 *
 * @param pos Position.
 *
 * @return Next item (or NULL);
 */
void* fr_next_item( fr_t pos );


/**
 * Return first Position in Framer.
 *
 * Current Position is not affected.
 *
 * @param pos Current Position.
 *
 * @return First Position.
 */
fr_s fr_first( fr_t pos );


/**
 * Return last Position in Framer.
 *
 * Current Position is not affected.
 *
 * @param pos Current Position.
 *
 * @return Last Position.
 */
fr_s fr_last( fr_t pos );


/**
 * Jump to first Position in Framer.
 *
 * @param pos Position.
 */
void fr_to_first( fr_t pos );


/**
 * Jump to last Position in Framer.
 *
 * @param pos Position.
 */
void fr_to_last( fr_t pos );


/**
 * Is Position at first item?
 *
 * @param pos Position.
 *
 * @return 1 if at first item, else 0.
 */
int fr_at_first( fr_t pos );


/**
 * Is Position at last item?
 *
 * @param pos Position.
 *
 * @return 1 if at last item, else 0.
 */
int fr_at_last( fr_t pos );


/**
 * Return current item.
 *
 * @param pos Position.
 *
 * @return Item.
 */
void* fr_item( fr_t pos );


/**
 * Return item at current segment index.
 *
 * NOTE: This only returns items in current segment. NULL is returned
 * if index is out of bounds.
 *
 * @param pos Position.
 * @param idx Segment index.
 *
 * @return Item (or NULL).
 */
void* fr_item_at( fr_t pos, fr_size_t idx );


/**
 * Is Position valid?
 *
 * @param pos Position.
 *
 * @return 1 if Position is valid, else 0.
 */
int fr_is_valid( fr_t pos );



/* ------------------------------------------------------------
 * Framer Node:
 * ------------------------------------------------------------ */

/**
 * Create Framer node with given segment size.
 *
 * @param size Segment size.
 *
 * @return Node.
 */
fn_t fn_new_sized( fr_size_t size );


/**
 * Return first Node in chain.
 *
 * @param node Current Node.
 *
 * @return First Node.
 */
fn_t fn_first( fn_t node );


/**
 * Append Node after anchor.
 *
 * @param anchor Anchor.
 * @param node   Node.
 *
 * @return Node.
 */
fn_t fn_append( fn_t anchor, fn_t node );


/**
 * Return peer Node.
 *
 * Next Node is returned if possible, else previous. If Node is last,
 * NULL is returned.
 *
 * NOTE: fn_update() is usefull when using Framer memory pooler API.
 *
 * @param node Current Node.
 *
 * @return Peer.
 */
fn_t fn_update( fn_t node );


/**
 * Delete Node.
 *
 * Next valid Node is returned after deletion. Valid Node is next if
 * possible. If last Node is delete, NULL is returned.
 *
 * @param node Node to delete.
 *
 * @return Updated Node position.
 */
fn_t fn_delete( fn_t node );


#endif
