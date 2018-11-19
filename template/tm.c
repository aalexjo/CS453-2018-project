/**
 * @file   tm.c
 * @author Alexander Johansen
 *
 * @section LICENSE
 *
 * [...]
 *
 * @section DESCRIPTION
 *
 * Implementation of your own transaction manager.
 * You can completely rewrite this file (and create more files) as you wish.
 * Only the interface (i.e. exported symbols and semantic) must be preserved.
**/

// Requested features
#define _GNU_SOURCE
#define _POSIX_C_SOURCE   200809L
#ifdef __STDC_NO_ATOMICS__
    #error Current C11 compiler does not support atomic operations
#endif

// External headers
#include <pthread.h>
#include <stdatomic.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#if (defined(__i386__) || defined(__x86_64__)) && defined(USE_MM_PAUSE)
	#include <xmmintrin.h>
#else
	#include <sched.h>
#endif
// Internal headers
#include <tm.h>

// -------------------------------------------------------------------------- //

/** Define a proposition as likely true.
 * @param prop Proposition
**/
#undef likely
#ifdef __GNUC__
    #define likely(prop) \
        __builtin_expect((prop) ? 1 : 0, 1)
#else
    #define likely(prop) \
        (prop)
#endif

/** Define a proposition as likely false.
 * @param prop Proposition
**/
#undef unlikely
#ifdef __GNUC__
    #define unlikely(prop) \
        __builtin_expect((prop) ? 1 : 0, 0)
#else
    #define unlikely(prop) \
        (prop)
#endif

/** Define one or several attributes.
 * @param type... Attribute names
**/
#undef as
#ifdef __GNUC__
    #define as(type...) \
        __attribute__((type))
#else
    #define as(type...)
    #warning This compiler has no support for GCC attributes
#endif

// -------------------------------------------------------------------------- //

struct region {
		struct lock_t lock; // Global lock
		void* start;        // Start of the shared memory region
		void* start_cpy;	// Adress of the copy of 
		struct link allocs; // Allocated shared memory regions
		size_t size;        // Size of the shared memory region (in bytes)
		size_t align;       // Claimed alignment of the shared memory region (in bytes)
		size_t align_alloc; // Actual alignment of the memory allocations (in bytes)

		int g_clock;		// TODO: make atomic increment
		bool is_copy;
		bool read_copy;
		bool w_lock;		//this can be unchecked while we are draining readers so the next write can begin
		int copy_clock;		//time at which the copy was created
		int readers_in_obj;	
		int readers_in_cpy;
	};

	struct thread {
		int l_clock;
		int w_clock;

};
/** Create (i.e. allocate + init) a new shared memory region, with one first non-free-able allocated segment of the requested size and alignment.
 * @param size  Size of the first shared segment of memory to allocate (in bytes), must be a positive multiple of the alignment
 * @param align Alignment (in bytes, must be a power of 2) that the shared memory region must support
 * @return Opaque shared memory region handle, 'invalid_shared' on failure
**/
shared_t tm_create(size_t size as(unused), size_t align as(unused)) {
	struct region* region = (struct region*) malloc(sizeof(struct region));
	//struct region* region_cpy = (struct region*) malloc(sizeof(struct region));

	if (unlikely(!region) && unlikely(!region_cpy) {
		return invalid_shared;
	}
	size_t align_alloc = align < sizeof(void*) ? sizeof(void*) : align; // Also satisfy alignment requirement of 'struct link'
	if (unlikely(posix_memalign(&(region->start), align_alloc, size) != 0) && unlikely(posix_memalign(&(region->start_cpy), align_alloc, size) != 0) ){//check that the memory is alligned just in case?
		free(region);
		//free(region_cpy);
		return invalid_shared;
	}
	memset(region->start, 0, size); //initialize memory region to 0
	memset(region->start_cpy, 0, size); //initialize memory region to 0

	link_reset(&(region->allocs)); //??
	region->size = size;
	region->align = align;
	region->align_alloc = align_alloc;
	return region;
}

/** Destroy (i.e. clean-up + free) a given shared memory region.
 * @param shared Shared memory region to destroy, with no running transaction
**/
void tm_destroy(shared_t shared as(unused)) {
	//TODO: destroy allocated region, shouldnt be a problem
}

/** [thread-safe] Return the start address of the first allocated segment in the shared memory region.
 * @param shared Shared memory region to query
 * @return Start address of the first allocated segment
**/
void* tm_start(shared_t shared as(unused)) { 
	return ((struct region*) shared)->start;
	//return NULL;
}

/** [thread-safe] Return the size (in bytes) of the first allocated segment of the shared memory region.
 * @param shared Shared memory region to query
 * @return First allocated segment size
**/
size_t tm_size(shared_t shared as(unused)) {
	return ((struct region*) shared)->size;
//	return 0;
}

/** [thread-safe] Return the alignment (in bytes) of the memory accesses on the given shared memory region.
 * @param shared Shared memory region to query
 * @return Alignment used globally
**/
size_t tm_align(shared_t shared as(unused)) {
    // TODO: tm_align(shared_t)
	return ((struct region*) shared)->align;
    //return 0;
}

/** [thread-safe] Begin a new transaction on the given shared memory region.
 * @param shared Shared memory region to start a transaction on
 * @param is_ro  Whether the transaction is read-only
 * @return Opaque transaction ID, 'invalid_tx' on failure
**/
tx_t tm_begin(shared_t shared as(unused), bool is_ro as(unused)) {
	thread_d *t = tx_init();
	tx_reader_lock(t);
	t->is_ro = is_ro;
	t->start = tx_deref();

	if (!is_ro) {
		shared.start_copy = tx_try_write_lock(); //check if it succedes
			//if(write_lock failed) return invalid_tx;

	}

    
}

/** [thread-safe] End the given transaction.
 * @param shared Shared memory region associated with the transaction
 * @param tx     Transaction to end
 * @return Whether the whole transaction committed
**/
bool tm_end(shared_t shared as(unused), tx_t tx as(unused)) {
		
	tx_reader_unlock();
	if (!tx->is_ro) {
		tx_commit_write_log();
	}

	tx_finish();

	return false;
}

/** [thread-safe] Read operation in the given transaction, source in the shared region and target in a private region.
 * @param shared Shared memory region associated with the transaction
 * @param tx     Transaction to use
 * @param source Source start address (in the shared region)
 * @param size   Length to copy (in bytes), must be a positive multiple of the alignment
 * @param target Target start address (in a private region)
 * @return Whether the whole transaction can continue
**/
bool tm_read(shared_t shared as(unused), tx_t tx as(unused), void const* source as(unused), size_t size as(unused), void* target as(unused)) {
    // TODO: tm_read(shared_t, tx_t, void const*, size_t, void*)
	actual_target = tx_deref(tx_t, shared) + ( shared->start - target) ;
	memcpy(actual_target, source, size);
	return true;
    //return false;
}

/** [thread-safe] Write operation in the given transaction, source in a private region and target in the shared region.
 * @param shared Shared memory region associated with the transaction
 * @param tx     Transaction to use
 * @param source Source start address (in a private region)
 * @param size   Length to copy (in bytes), must be a positive multiple of the alignment
 * @param target Target start address (in the shared region)
 * @return Whether the whole transaction can continue
**/
bool tm_write(shared_t shared as(unused), tx_t tx as(unused), void const* source as(unused), size_t size as(unused), void* target as(unused)) {
    // TODO: tm_write(shared_t, tx_t, void const*, size_t, void*)
	actual_target = tx_deref(tx_t, shared) + (shared->start - target);

	memcpy(actual_target, source, size);
	return true;
    
}



/*---------------------------------------------------------------------------------------------*/


/** [thread-safe] Memory allocation in the given transaction.
 * @param shared Shared memory region associated with the transaction
 * @param tx     Transaction to use
 * @param size   Allocation requested size (in bytes), must be a positive multiple of the alignment
 * @param target Pointer in private memory receiving the address of the first byte of the newly allocated, aligned segment
 * @return Whether the whole transaction can continue (success/nomem), or not (abort_alloc)
**/
alloc_t tm_alloc(shared_t shared as(unused), tx_t tx as(unused), size_t size as(unused), void** target as(unused)) {
    // TODO: tm_alloc(shared_t, tx_t, size_t, void**)
    return abort_alloc;
}

/** [thread-safe] Memory freeing in the given transaction.
 * @param shared Shared memory region associated with the transaction
 * @param tx     Transaction to use
 * @param target Address of the first byte of the previously allocated segment to deallocate
 * @return Whether the whole transaction can continue
**/
bool tm_free(shared_t shared as(unused), tx_t tx as(unused), void* target as(unused)) {
    // TODO: tm_free(shared_t, tx_t, void*)
    return false;
}
