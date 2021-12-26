#ifndef _BuddyAllocator_h_                   // include file only once
#define _BuddyAllocator_h_
#include <iostream>
#include <vector>
#include <assert.h>
#include <math.h>
using namespace std;
typedef unsigned int uint;

/* declare types as you need */

class BlockHeader{
public:
	// think about what else should be included as member variables
	int block_size;  // size of the block
	BlockHeader* next; // pointer to the next block
	bool free;
	BlockHeader (int _s = 0){
		block_size=_s;
		free=true;
		next=nullptr;
	}
};

class LinkedList{
	// this is a special linked list that is made out of BlockHeader s. 
public:
	BlockHeader* head;		// you need a head of the list
public:
	BlockHeader* gethead() {return head;}
	LinkedList(BlockHeader* h = nullptr){ // linkedlist default constructor
		head=h;
	}

	void insert (BlockHeader* b){	// adds a block to the list
		if(head==nullptr){
			head=b;
			b->free=true;
		}
		else{	
			BlockHeader* node = head; // set a temporary equal to head
			while(node->next!=nullptr){
				node=node->next;	
			}
			node->next=b;
			b->next=nullptr;
			b->free=true;
		}
	}

	void remove (BlockHeader* b){  // removes a block from the list
		BlockHeader* node = head; // set a temporary equal to head
		if(node==b){
			head=node->next;
			b->free=false;	
		}

		else{
			while(node->next!=b && node->next!=nullptr){
				node=node->next;
			}
			
			node->next=node->next->next;
			b->next = nullptr;
			b->free=false;
			
		}// end of big else
	}// end of the remove function


	BlockHeader* remove(){ // return the first item
		assert (head!=nullptr);
		BlockHeader* b = head;
		head = head->next;
		b->next = nullptr;
		return b;
	}
};


class BuddyAllocator{
private:
	/* declare more member variables as necessary */
	vector<LinkedList> FreeList;
	int basic_block_size;
	int total_memory_size;
	char* start;

private:
	/* private function you are required to implement
	 this will allow you and us to do unit test */
	
	BlockHeader* getbuddy (BlockHeader * addr){// given a block address, this function returns the address of its buddy 
		//BlockHeader *buddy = (BlockHeader *)((unsigned long long)addr^addr->block_size);// bitwise(xor) header with block size to get the buddy address
		return (BlockHeader*) ((((char*)addr - start)^addr->block_size) + start);
		//return buddy;
	}
	
	
	
	bool arebuddies (BlockHeader* block1, BlockHeader* block2){// checks whether the two blocks are buddies are not
		if(block2->free && block1->block_size == block2->block_size){
			return true;
		}
		return false;
		//return (block2 == (BlockHeader *)((unsigned long long)block1^block1->block_size));
	}

	// this function merges the two blocks returns the beginning address of the merged block
	// note that either block1 can be to the left of block2, or the other way around
	BlockHeader* merge (BlockHeader* block1, BlockHeader* block2){
		//assume block 1 is the smaller address
		if(block1>block2){
			swap(block1, block2);
		}
		block1->block_size*=2;
		return block1;

		/*BlockHeader* leftbuddy;
		BlockHeader* rightbuddy;

		if(block1<block2){
			leftbuddy=block1;
			rightbuddy=block2;
		}
		else if(block1>block2){
			leftbuddy=block2;
			rightbuddy=block1;
		}

		if(block1->block_size == block2->block_size && block1->free && block2->free){
			int newsize= block1->block_size *2;
			BlockHeader* mergeblock = leftbuddy;
			mergeblock->block_size= newsize;
			return mergeblock;
		}*/
	}
	

	// splits the given block by putting a new header halfway through the block
	// also, the original header needs to be corrected
	BlockHeader* split (BlockHeader* block){ ////??????
		int bs = block->block_size;
		block->block_size /=2;
		block->next=nullptr;

		BlockHeader* sh = (BlockHeader*) ((char*)block + block->block_size);
		sh->next = nullptr, sh->block_size = block->block_size;
		//BlockHeader* temp = new (sh) BlockHeader (block->block_size);
		sh->free = true;
		block->free = true;
		return sh;

		/*BlockHeader* splitblock = block;
		splitblock->block_size = block->block_size/2;

		BlockHeader* rightblock = getbuddy(splitblock); // find right buddy of the split block
		rightblock->block_size = splitblock->block_size;
		rightblock->next = nullptr;
		rightblock->free = true;
		return rightblock;*/
	}

	int getIndex (int size){
		return log2(size/basic_block_size);
	}
	


public:
	BuddyAllocator (int _basic_block_size, int _total_memory_length); 
	/* This initializes the memory allocator and makes a portion of 
	   ’_total_memory_length’ bytes available. The allocator uses a ’_basic_block_size’ as 
	   its minimal unit of allocation.  
	*/ 

	~BuddyAllocator(); 
	/* Destructor that gives any allocated memory back to the operating system. 
	   There should not be any memory leakage (i.e., memory staying allocated).
	*/ 

	char* alloc(int _length); 
	/* Allocate _length number of bytes of free memory and returns the 
		address of the allocated portion. Returns 0 when out of memory. */ 

	int free(char* _a); 
	/* Frees the section of physical memory previously allocated 
	   using ’my_malloc’. Returns 0 if everything ok. */ 
   
	void printlist ();
	/* Mainly used for debugging purposes and running short test cases */
	/* This function should print how many free blocks of each size belong to the allocator
	at that point. The output format should be the following (assuming basic block size = 128 bytes):

	[0] (128): 5
	[1] (256): 0
	[2] (512): 3
	[3] (1024): 0
	....
	....
	 which means that at this point, the allocator has 5 128 byte blocks, 3 512 byte blocks and so on.*/
};

#endif
