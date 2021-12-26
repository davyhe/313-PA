#include "BuddyAllocator.h"
#include <iostream>
#include <math.h>
using namespace std;

/* This initializes the memory allocator and makes a portion of 
	   ’_total_memory_length’ bytes available. The allocator uses a ’_basic_block_size’ as 
	   its minimal unit of allocation.  */ 
BuddyAllocator::BuddyAllocator (int _basic_block_size, int _total_memory_length){
	total_memory_size=_total_memory_length, basic_block_size = _basic_block_size;
  start = new char[total_memory_size];
  int l = log2(total_memory_size/basic_block_size);
  for(int i=0; i<l; i++){
    FreeList.push_back(LinkedList());
  }
  FreeList.push_back(LinkedList((BlockHeader*)start)); // cast the start to a blockheader and push it to the freelist
  // BlockHeader* header = new (start) BlockHeader (total_memory_size);
  ((BlockHeader*)start)->block_size = total_memory_size;
}

/* Destructor that gives any allocated memory back to the operating system. 
	   There should not be any memory leakage (i.e., memory staying allocated).	*/
BuddyAllocator::~BuddyAllocator (){
  delete [] start;
  
  /*
	for(int i=0; i<FreeList.size(); i++){
    delete FreeList[i].head;
    FreeList[i].listsize=0;
  }*/
}

char* BuddyAllocator::alloc(int _length) {
  /* This preliminary implementation simply hands the call over the 
     the C standard library! 
     Of course this needs to be replaced by your implementation.
  */
  int x = _length + sizeof(BlockHeader);
  int index = ceil(((double) x / basic_block_size));
  index = ceil (log2(index));
  int blockSizeReturn = (1 << index) * basic_block_size;
  if(FreeList[index].head != nullptr){ // found a block of correct size
    BlockHeader* b = FreeList[index].remove();
    b->free = false;
    return (char*)(b+1); // return the b + 1 item of the block
  }

  int indexcorrect = index;
  for(;index<FreeList.size(); index++){
    if(FreeList[index].head !=nullptr){
      break;
    }
  }
  if(index >= FreeList.size()){ // no bigger block found
    return nullptr;
  }

  while(index > indexcorrect){// a bigger block found
    BlockHeader* b = FreeList[index].remove();
    BlockHeader* shb = split(b);
    --index;
    FreeList[index].insert(b);
    FreeList[index].insert(shb);
  }
  
  BlockHeader* block =  FreeList[index].remove();
  block->free = false;
  return (char*) (block+1);
  //return new char [_length]; // dummy line of the code for the program to pass the compiling test
}



int BuddyAllocator::free(char* _a) {
  /* Same here! */
  BlockHeader* b = (BlockHeader*) (_a - sizeof(BlockHeader));
  while(true){
    int size = b->block_size;
    b->free = true;
    int index = getIndex(size);
    if(index == FreeList.size()-1){
      FreeList[index].insert(b);
      break;
    }
    BlockHeader* buddy = getbuddy(b);
    if(buddy->free){
      FreeList[index].remove(buddy);
      b = merge(b,buddy);
    }
    else{
      FreeList[index].insert(b);
      break;
    }
  
  }

  /*
  BlockHeader* block = (BlockHeader*) (_a);
  bool mergeable = true;
  while(mergeable){
    BlockHeader* buddy = getbuddy(block);
    if(arebuddies(block, buddy) && buddy->free){
      block=merge(block, buddy);
    }
    else{
      mergeable = false;
    }
  }
  */
  //delete _a;
  return 0;
}



void BuddyAllocator::printlist (){
  cout << "Printing the Freelist in the format \"[index] (block size) : # of blocks\"" << endl;
  for (int i=0; i<FreeList.size(); i++){
    cout << "[" << i <<"] (" << ((1<<i) * basic_block_size) << ") : ";  // block size at index should always be 2^i * bbs
    int count = 0;
    BlockHeader* b = FreeList [i].head;
    // go through the list from head to tail and count
    while (b){
      count ++;
      // block size at index should always be 2^i * bbs
      // checking to make sure that the block is not out of place
      if (b->block_size != (1<<i) * basic_block_size){
        cerr << "ERROR:: Block is in a wrong list" << endl;
        exit (-1);
      }
      b = b->next;
    }
    cout << count << endl;  
  }
}

