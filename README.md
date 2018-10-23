# Framer - Unrolled Doubly Linked List library

## Introduction

Framer is implementation of Unrolled Doubly Linked List, UDLL. Framer
provides an efficient implementation, with reduced set of
functions. Framer tries to perform only the essential operations and
if use case level optimizations are needed, the user should implement
the required features on top of Framer. As an exception to minimal
approach, Framer supports custom memory allocation functions, both at
compile time and runtime.

Data is organized into a number of Nodes. Each Node has links to
previous and next Node. Data items (i.e. pointers to objects) are
stored to Data Segment of the Node.


Framer Node contains the following fields:

* prev : previous node pointer

* next : next node pointer

* used : segment use count

* data : segment (pointer array)

The current position in Frame is maintained in Framer
Position. Position includes global information about Framer. First of
all there is the position information itself. Position is defined by
reference to Node and an index to the Data Segment.

In addition to position information, Position stores segment size,
global Framer item count, and Framer Node count. Optionally Position
can also include a reference to a Custom Memory Manager.


Framer Position contains the following fields:

* seg  : current Node segment (pointer to Node)

* idx  : current segment index

* size : segment size in storable items

* icnt : item count

* ncnt : node count

* mem  : memory API


## Basic usage

Framer is accessed through Position and Nodes are mainly opaque for
the user. Framer can be created simply:

    fr_t pos;
    pos = fr_create();

`pos` is Framer Position and initial Node is allocated with default
segment size. Node segment is empty and Position index is at 0.

Items can be added to the Framer by appending:

    fr_append( pos, item );

or by inserting:

    fr_insert( pos, item );

`fr_append()` first advances Position by one and then stores the item
to the current position. As a special case, `fr_append()` does not
advance index if Frame is empty. `fr_append()` can be used to fill
Framer with items one by one using one command. However the most light
weight operation is `fr_push()`, where it is always assumed that
Position is at the end of Framer.

`fr_insert()` simply inserts the item to the current position and
Position is not updated, unless Nodes needs to be rearranged (see
below: Node segments).

Items can be removed from Framer:

    void* item;
    item = fr_delete( pos );

The removed item is stored to `item`, and Position stays in the same
location, unless Nodes needs to be rearranged (see below: Node
segments).

Position can be explicitly moved forwards or backwards. In order to
move forward 10 steps:

    fr_next_n( pos, 10 );

and the same distance backwards:

    fr_prev_n( pos, 10 );

There are number of other operations regarding: queries, finding,
stacks, and others. Please refer to the Doxygen documentation for
details.

Finally, the Framer memory is released by issuing:

    pos = fr_destroy( pos );



## Node segments

UDLL efficiency is based on having high cache hit rates and sufficient
memory efficiency. This is supported by having continuous data in
Framer segments. CPU is able to prefetch data and in consecutive
accesses the following accesses are mostly found in cache.

UDLL also provides reasonable storage efficiency. Framer can be
operated so that all segments are at least half filled, except the
last segment. If better storage utilization is required, the user can
implement more aggressive packing of segments with custom access
functions.

Default Node size in Framer (used by `fr_create()`) is same as cache
line size. Framer header assumes 64 byte cache line size, and the
assumed value can be overwritten with `FR_CACHE_LINE_SIZE`
define. Note that Node size is not the same as Node segment size. Node
has a header part before the storage for the actual segment. Hence the
number of items that fit a cache line sized Node is:

    ITEMS = ( CACHE_LINE_SIZE - NODE_HEADER ) / ITEM_BYTE_SIZE

Insertion tries to delay creation of a new segment if there is space
in the peer segments, either in next or previous segment. When
current, previous, and next are all full, a new segment is
created. When new segment is created, it will be filled to at least
half regardless where the current Position is within the segment.

There is some "extra" data movement associated in delaying segment
allocation, but it is considered less harmful than allocating new
segments "too" early.

When items are deleted with `fr_delete()`, segment fill might become
below half, and no packing occurs. If `fr_delete_even()` is used,
Framer evens the data items between peers, and ensures that data level
is above half.

`fr_delete()` should be preferred if user knows that there will be
data insertions right after deletion. If the accesses are more random
across the Framer, it would be probably sensible to use
`fr_delete_even()` instead, and improve storage efficiency.


## Memory API

Framer user might want to use custom Memory Managers. By default
Framer uses `malloc()` and friends. The basic allocation functions can
be customized by defining `FRAMER_MEM_API`. See `framer.h` for
details.

In addition to compile time selection, there is possibility to use
custom runtime memory management functions. Framer benefits
significantly for example from a memory pooler. Framer Nodes have the
same size, hence memory pooler works very well with Framer.

Memory Pooler can be taken into use by creating a special Position:

    fr_t pos;
    pos = fr_pos_new_with_mem( NULL, FR_SEG_MIN, my_alloc, my_free, my_pooler );

`fr_pos_new_with_mem()` creates a Position with pooler
interface. Pooler memory reserve function is `my_alloc`, `my_free` is
release function, and `my_pooler` is reference to Pooler data, Pooler
object if you will.

After proper Position is available, Framer is created using:

    fr_create_using( pos );

Position is initialized with initial segment that was provided by the
Pooler. Framer is destroyed with `fr_destroy()` as in "normal" case.

Whenever Node related memory is reserved or released, `my_alloc` or
`my_free` is called with Position and `my_pooler` as arguments.


## Framer API documentation

See Doxygen documentation. Documentation can be created with:

    shell> doxygen .doxygen


## Examples

All functions and their use is visible in tests. Please refer `test`
directory for testcases.


## Building

Ceedling based flow is in use:

    shell> ceedling

Testing:

    shell> ceedling test:all

User defines can be placed into `project.yml`. Please refer to
Ceedling documentation for details.


## Ceedling

Framer uses Ceedling for building and testing. Standard Ceedling files
are not in GIT. These can be added by executing:

    shell> ceedling new framer

in the directory above Framer. Ceedling prompts for file
overwrites. You should answer NO in order to use the customized files.
