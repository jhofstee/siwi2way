#include "platform.h"

#ifdef VE_MEM_USED

#define VE_MOD VE_MOD_MEM

// This is defined in platform.h to prevent adl_memory.h from being included.
// Undefining it allows the ADL memory API to be used here.
#undef __adl_memory_H__
#include "adl_memory.h"
#include "ve_trace.h"
#include "str_utils.h"
#include "dev_reg.h"

// A descriptor used to keep track of memory allocations.
typedef struct {
	void* 	ptr;	// The start of the allocated memory.
	u32		size;	// The number of bytes allocated.
	void*	next;	// A pointer to the next descriptor (NULL if none).
	void*	prev;	// A pointer to the previous descriptor (NULL if none).
} VeMallocFrame;

// The first frame in the list of allocated memory.
VeMallocFrame* firstFrame = NULL;
// The last frame in the list of allocated memory.
VeMallocFrame* lastFrame = NULL;

static veMemStats stats = {0, 0, 0};
/*
 * Adds a frame to the linked list after a specific frame.
 *
 * Parameters:
 *		new		- The frame to add.
 *		before	- The which new will be added after.
 */
void addFrameAfter(VeMallocFrame* new, VeMallocFrame* before){
	VeMallocFrame* after = before->next;

	if(!new){
		ve_error("Attempted to link a NULL frame!");
		return;
	}

	if(!before){
		ve_error("Attempted to link after a NULL frame!");
		return;
	}

	if(firstFrame == NULL || lastFrame == NULL){
		ve_error("Memory inconsistent!");
		return;
	}

	before->next = new;
	new->prev = before;

	if(after){
		after->prev = new;
		new->next = after;
	}

	if(before == lastFrame){
		lastFrame = new;
	}
}

/*
 * Adds a frame to the linked list before a specific frame.
 *
 * Parameters:
 *		new		- The frame to add.
 *		before	- The which new will be fore after.
 */
void addFrameBefore(VeMallocFrame* new, VeMallocFrame* after){
	VeMallocFrame* before = after->prev;

	if(!new){
		ve_error("Attempted to link a NULL frame!");
		return;
	}

	if(!after){
		ve_error("Attempted to link before a NULL frame!");
		return;
	}

	if(firstFrame == NULL || lastFrame == NULL){
		ve_error("Memory inconsistent!");
		return;
	}

	after->prev = new;
	new->next = after;

	if(before){
		before->next = new;
		new->prev = before;
	}

	if(after == firstFrame){
		firstFrame = new;
	}
}

/*
 * Removes a frame from the list.
 *
 * Note:  The frame memory is not released here.
 *
 * Parameters:
 * 		old	- The frame to remove.
 */
void removeFrame(VeMallocFrame *old){
	VeMallocFrame* before;
	VeMallocFrame* after;

	if(!old){
		ve_error("Attempted to release a NULL frame!");
		return;
	}

	if(old == firstFrame){
		firstFrame = firstFrame->next;
	}

	if(old == lastFrame){
		lastFrame = lastFrame->prev;
	}

	before = old->prev;
	after = old->next;

	if(before){
		before->next = after;
	}

	if(after){
		after->prev = before;
	}
}

/*
 * Locates the frame in the list that corresponds to a specific memory
 * allocation.
 *
 * Parameters:
 * 		ptr		- The pointer returned by the ve_memGet() call that was used to
 * 				  allocate this memory.
 *
 * Returns:
 * 		NULL	- ptr was not allocated with ve_memGet(), or has already been
 * 				  released.
 * 		Other	- A pointer to the frame that corresponds to the allocated
 * 				  memory.
 */
VeMallocFrame* findFrame(void* ptr){
	VeMallocFrame* next = firstFrame;

	while(next){
		if(next->ptr == ptr){
			break;
		}

		next = next->next;
	}

	return next;
}

/*
 * Creates and initialised a new frame.
 *
 * Returns:
 * 		NULL	- The frame could not be created.
 * 		Other	- A pointer to the new frame.
 */
VeMallocFrame* newFrame(void){
	VeMallocFrame* frame = adl_memGet(sizeof(*firstFrame));

	if(frame){
		frame->ptr = NULL;
		frame->size = 0;
		frame->next = NULL;
		frame->prev = NULL;

		stats.frameOverhead++;
	}

	return frame;
}

/*
 * Allocates a block of memory of a given size.  This should be used in place of
 * adl_memGet().
 *
 * Note:  The behaviour in the event of a memory allocation failure depends on
 * the usage of the ADL error service.  Refer to adl_memGet() for more
 * information.
 *
 * See also:
 * 		ve_memRelease()
 * 		ve_memReleaseLater()
 *
 * Parameters:
 * 		size	- The number of bytes to allocate.
 *
 * Returns:
 * 		NULL	- The allocation failed.
 * 		Other	- A pointer to the allocated memory.
 */
void* ve_malloc(u32 size){
	VeMallocFrame* new;
	u32 totalUsage;
	char temp[50];

	// Create a descriptor to keep track of this allocation
	if(firstFrame == NULL){
		// There are currently no allocated frames, so the list is empty.
		// Create a new frame, and initialise the list.
		if(!(firstFrame = newFrame())){
			return NULL;
		}else{
			lastFrame = firstFrame;
		}
	}else{
		// Create a new frame, and add it to the list.
		if(!(new = newFrame())){
			return NULL;
		}else{
			addFrameBefore(new, firstFrame);
		}
	}

	new = firstFrame;

	// Calculate the total memory (including overhead).
	totalUsage = stats.heapUsed + size + stats.frameOverhead * sizeof(VeMallocFrame);

	// If this allocation is the largest memory usage so far, print it.
	if(stats.heapUsed > stats.maxHeapUsed){
		stats.maxHeapUsed = stats.heapUsed;
		// note: can't use trace directly, will recurse!
		if (ve_traceEnabled(2))
		{
			sprintf(temp, "\r\n+VMEM: \"peak\",%d,%d\r\n", stats.heapUsed, totalUsage);
			adl_atSendResponse(ADL_AT_UNS, temp);
		}
	}

	// Allocate the requested memory.
	if((new->ptr = adl_memGet(size))){
		new->size = size;
		stats.heapUsed += size;
		return new->ptr;
	}else{
		removeFrame(new);
		adl_memRelease(new);
		return NULL;
	}
}

/*
 * This function should be used when control of memory allocated with
 * ve_memGet() is to be passed to a library that does not use the VE memory
 * interface (such as the OS, or a plugin library).  This releases any overhead
 * used for tracking the memory allocation, but does not release the memory
 * itself.  This memory will no longer be included in the current memory usage
 * reported by AT+VMEM.
 *
 * See also:
 * 		ve_memGet()
 * 		ve_memRelease()
 *
 * Parameters:
 * 		ptr	- The pointer returned by the ve_memGet() call used to allocate the
 * 			  memory.
 *
 * Returns:
 *		TRUE	- The memory is no longer managed by ve_mem.
 *		FALSE	- ptr was not valid.
 */
veBool ve_memReleaseLater(void* ptr){
	VeMallocFrame* frame = findFrame(ptr);

	if(frame){
		removeFrame(frame);
		stats.heapUsed -= frame->size;
		stats.frameOverhead--;
		adl_memRelease(frame);

		return veTrue;
	}else{
		ve_error("Attempt to free invalid pointer %08X!", ptr);
		return veFalse;
	}
}

/*
 * Releases a block of memory allocated with ve_memGet().
 *
 * Note:  This function should not be called directly.  The ve_memRelease()
 * macro should be used instead.
 *
 * See also:
 *		ve_memGet()
 *		ve_memRelease()
 *		ve_memReleaseLater()
 *
 * Parameters:
 * 		ptr		- A pointer to the pointer returned by the ve_memGet() call that
 * 				  was used to allocate this memory.  The referenced pointer will
 * 				  be set to NULL on success.
 */
void _ve_memRelease(void** ptr){
	// Release the descriptor.
	if(ve_memReleaseLater(*ptr)){
		// Release the memory.
		_adl_memRelease(ptr);
	}
}

veMemStats const* ve_memStats(void)
{
	stats.overhead = stats.frameOverhead * sizeof(VeMallocFrame);
	return &stats;
}

#endif
