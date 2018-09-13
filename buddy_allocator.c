
/* ----------- Graphic description of the architecture that we follow in the implementation ---------------- *
 * We use the concept of levels to handle the memory. The below scheme indicates this concept:               *
 * -----------------------------------------------                                                           *
 * |                                             |                                                           *
 * |                    0                        |  128     level = 0                                        *
 * |---------------------------------------------|                                                           *
 * |                    |                        |                                                           *
 * |         1          |          2             |  64      level = 1                                        *
 * |---------------------------------------------|                                                           *
 * |         |          |          |             |                                                           *
 * |    3    |    4     |     5    |     6       |  32      level = 2                                        *
 * |---------------------------------------------|                                                           *
 * |    |    |    |     |     |    |     |       |                                                           *
 * | 7  | 8  | 9  | 10  | 11  | 12 | 13  |  14   |  16      level = 3                                        *
 * |---------------------------------------------|                                                           *
 *                                                                                                           *
 * In the above scheme, we can see the indexing we follow in our design, the levels and the size of each     *
 * block in each level.                                                                                      *
 * The minimum block size is 16 bytes.                                                                       *
 * When you run the program just give the amount of the total buddy memory you want to allocate,             *
 * e.g. 16, 32, 64, 128, 256 etc, and any number among them.                                                 *
 *                                                                                                           *
 * IMPORTANT NOTE                                                                                            *
 * --------------                                                                                            *
 * For a complete run that executes all the set of commands, including an error scenario, please use         *
 * an 64 byte or less memory size when you are prompted. Otherwise, you won't be able to test an             *
 * error scenario.                                                                                           *
 *************************************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/* ------------------------- Explanation of FREE,USED,SPLIT,FULL flags -------------------------------- *
 * a block is FREE when it has not been allocated.                                                      *
 * a block is USED when it has been allocated.                                                          *
 * a parent block is SPLIT when it has two child blocks with the following combinations: FREE and FREE  *
 *                                                                                       FREE and SPLIT *
 *                                                                                       USED and FREE  *
 *                                                                                       USED and SPLIT *
 *                                                                                                      *
 * a parent block is FULL when it has two child blocks with the following combinations:  USED and USED  *
 *                                                                                       USED and FULL  *
 ********************************************************************************************************/

#define FREE 0
#define USED 1
#define SPLIT 2
#define FULL 3
#define LEAF_SIZE 16     // the minimum block size in bytes

#define IS_POWER_OF_2(x) (!(x & (x-1)))

typedef struct {
    uint8_t level;	//organize the memory in levels. this variable keeps the #number of levels that the whole memory can be divided.
                        //The last level has memory blocks of size 16
    uint32_t remaining_size;	//the size of memory that is available for allocations
    uint8_t buddy_status[1];	//the status of each assigned memory block. Each block can be FREE, USED,SPLIT or FULL
} buddy_allocator_t;

// returns the num itself if it is power of two or the next greater number than num which is power of two
static size_t inline find_next_power_of_two(size_t num) {

    size_t power = 2;

    if (IS_POWER_OF_2(num))
        return num;

    while (num >>= 1) 
        power <<= 1;

    return power;

}

//returns the level that a memory size can be divided. The last level has blocks of size 16
static uint8_t inline find_level(size_t num) {

     uint8_t level = 0;

     while((num >>=1) && (num >= LEAF_SIZE))
         ++level;

     return level;
}

// create a buddy_allocator and initialize it.
buddy_allocator_t *buddy_allocator_create(void *raw_memory, size_t raw_memory_size) {
   
      uint8_t level = find_level(raw_memory_size);

      buddy_allocator_t *new_buddy = (buddy_allocator_t *) raw_memory;

      // if the requested memory is less than LEAF_SIZE then allocate the minimum memory block that can be provided by the buddy allocator.
      if(raw_memory_size < LEAF_SIZE)      
          new_buddy = (buddy_allocator_t *)malloc(sizeof(uint8_t)*LEAF_SIZE + sizeof(buddy_allocator_t));    // malloc raw_memory_size bytes
      else
          new_buddy = (buddy_allocator_t *)malloc(sizeof(uint8_t)*raw_memory_size + sizeof(buddy_allocator_t));
      memset(new_buddy, FREE, raw_memory_size);

      new_buddy->level = level;
      new_buddy->remaining_size = (1<<level) * LEAF_SIZE;
  
      return new_buddy;
}

// each time a new allocation takes place, we have to mark the parent node.
// If the buddy block of the allocated block is USED or FULL then mark the parent block of those blocks as FULL
void inform_parent(buddy_allocator_t *buddy_allocator, uint8_t index){

    while(1){
        uint8_t buddy = index - 1 + (index & 1) * 2;
        if (buddy > 0 && (buddy_allocator->buddy_status[buddy] == USED ||buddy_allocator->buddy_status[buddy] == FULL)) {
            index = (index + 1) / 2 - 1;
            buddy_allocator->buddy_status[index] = FULL;
        } else {
            return;
        }
    }
}

void *buddy_allocator_alloc(buddy_allocator_t *buddy_allocator, size_t size){

    if(!buddy_allocator)
        return NULL;

    size_t buddy_size;    
    uint8_t level = 0, buddy_index = 0;
    uint32_t total_size = (1 << buddy_allocator->level)*LEAF_SIZE; 
    uint32_t available_size = buddy_allocator->remaining_size;

    if(size == 0){
        printf("Invalid memory size. Memory size requested is zero bytes.\n");
        return NULL;
    } else
        buddy_size = find_next_power_of_two(size);


    if (buddy_size > available_size){
        printf("There is no available block in your memory to allocate a block size %zu\n", buddy_size);
        return NULL;
    }

    while(level <= buddy_allocator->level && buddy_allocator->remaining_size >= 16){
        // if the desired memory size is equal with available memory size then allocate it
        if(buddy_size == available_size){
            if(buddy_allocator->buddy_status[buddy_index] == FREE){
                buddy_allocator->buddy_status[buddy_index] = USED;
                buddy_allocator->remaining_size -= buddy_size;
                inform_parent(buddy_allocator,buddy_index);
                printf("Allocation of block size %zu was successful.\n", buddy_size);
                printf("Available memory size: %u\n", buddy_allocator->remaining_size);
                return (buddy_allocator + (((buddy_index + 1) - (1 << level)) << (buddy_allocator->level - level)));
            }
        } else {
            // if the current block is USED or FULL check if its buddy in the same level is available in case we are in the left block
            if(buddy_allocator->buddy_status[buddy_index] == USED || buddy_allocator->buddy_status[buddy_index] == FULL){
               if(buddy_index & 1){
                   ++buddy_index;
                   continue;
               } 

            } else if(buddy_allocator->buddy_status[buddy_index] == FREE){	// if the current block is FREE then split it in two new blocks in next level.
                                                                                // Mark each one as FREE
                buddy_allocator->buddy_status[buddy_index] = SPLIT;
                buddy_allocator->buddy_status[buddy_index*2+1] = FREE;
                buddy_allocator->buddy_status[buddy_index*2+2] = FREE;

            }
  
            // go to the first block of the next level
            buddy_index = buddy_index*2+1;
            ++level;
            available_size = total_size/(1<<level);
            continue;

        }

        // if we are in the correct level and the left block is not free check its buddy in the same level
        if(buddy_index & 1){
            ++buddy_index;
            continue;
        }
        
        // if the buddy_size == available_size but we are not in the correct level. Go to the next level and use the correct index.
        ++buddy_index;
        ++level;

    }

    return NULL;
}

// During the deletion of a memory block we have to mark the parent block status with the new one.
// We mark a parent as SPLIT when it was FULL and now has one child FREE and one USED.
// The deleted block is marked as FREE 
void merge_buddy(buddy_allocator_t *buddy_allocator, uint8_t index) {

    uint8_t buddy;
   
    while(1){ 
        buddy = index - 1 + (index & 1) * 2;
        if (buddy < 0 || buddy_allocator->buddy_status[buddy] != FREE) {
            buddy_allocator->buddy_status[index] = FREE;
            while (((index = (index + 1) / 2 - 1) >= 0) &&  buddy_allocator->buddy_status[index] == FULL)
                buddy_allocator->buddy_status[index] = SPLIT;
       
            return;
        }
        index = (index + 1) / 2 - 1;
    }
}

// free a buddy memory block
void buddy_allocator_free(buddy_allocator_t *buddy_allocator, void *ptr){

    if(!ptr)
        return;

    // calculate the offset of the desired memory block from the address "zero" of the memory
    buddy_allocator_t *free_ptr = (buddy_allocator_t*)ptr;
    uint32_t offset = (uint32_t)(free_ptr-buddy_allocator)*LEAF_SIZE;     

    uint8_t buddy_index = 0;
    uint32_t memory_start = 0;
    uint32_t length = (1 << buddy_allocator->level) * LEAF_SIZE;

    // find the index of the memory block that we want to delete
    while(1){
        
        // we found the desired block. Increase the available memory size and perform the merge.
        if(buddy_allocator->buddy_status[buddy_index] == USED){
            if(offset == memory_start){
                buddy_allocator->remaining_size+=offset;
                merge_buddy(buddy_allocator, buddy_index);
                printf("Deletion of block size %u was successful.\n", offset);
                printf("Available memory size: %u\n", buddy_allocator->remaining_size);
                return;
            }
        } else if (buddy_allocator->buddy_status[buddy_index] == FREE){	   // the desired block is already free
            return;
        } else{ 	// search for the block in the next level and check all the buddies in this level
            length /=2;
            if(offset < memory_start + length){
                buddy_index = buddy_index*2+1;
            } else {
                buddy_index = buddy_index*2+2;
                memory_start +=length;
            }
        }
    }
}

// deallocate the whole buddy memory. Return the memory to the system
void buddy_allocator_destroy(buddy_allocator_t *buddy_allocator){

    if(!buddy_allocator)
        return;

    free(buddy_allocator);
}

int main(int argc, char *argv[]) {

    int raw_memory_size = 0;
    buddy_allocator_t *buddy;
    void *raw_memory = NULL;
   
    printf("Enter the size of memory you want to allocate: ");
    scanf("%d", &raw_memory_size);

    // If the requested size is 0 or less, then the allocation will not take place.
    if (raw_memory_size <= 0){
        printf("Invalid requested size. Allocation aborted\n");
        return -1;
    }

    size_t allocated_size = find_next_power_of_two(raw_memory_size);

    // create the buddy allocator
    buddy = buddy_allocator_create(raw_memory, allocated_size);

    if(!buddy){
        printf("Buddy memory allocation failed!\n");
        return -1;
    }
    
    // ask from buddy memory to allocate a block of memory with size 32
    void *buddy1 = buddy_allocator_alloc(buddy, 32);
   
    if(!buddy1)
        printf("buddy1 allocation failed.\n");

    // ask to allocate one more block of size 32
    void *buddy2 = buddy_allocator_alloc(buddy, 32);

    if(!buddy2)
        printf("buddy2 allocation failed.\n");
 
    // ask to allocate a block of size 15
    void *buddy3 = buddy_allocator_alloc(buddy,15);

    if(!buddy3)
        printf("buddy3 allocation failed.\n");

    // free the buddy2 block of size 32
    buddy_allocator_free(buddy, buddy2);
 
    // ask again to allocate the block of size 15
    void *buddy4 = buddy_allocator_alloc(buddy,15);

    if(!buddy4)
        printf("buddy4 allocation failed.\n"); 

    // destroy buddy allocator    
    buddy_allocator_destroy(buddy);

    return 0;

}
