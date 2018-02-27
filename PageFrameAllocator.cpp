/*  PageAllocator - allocate pages in memory
 * 
 * File:   PageFrameAllocator.cpp
 * Author: Mike Goss <mikegoss@cs.du.edu>
 * 
 * Created on February 2, 2018, 3:47 PM
 */

#include "PageFrameAllocator.h"

#include <cstring>
#include <sstream>

PageFrameAllocator::PageFrameAllocator(MMU &mmu_mem) {
    //Set our internal MMU pointer to the pointer provided in our constructor
    mem = &mmu_mem;

    //Build our free list
    page_frames_total = mem->get_frame_count();
    page_frames_free = mem->get_frame_count();
    free_list_head = 0;

    //Add all page frames to free list
    /* This code puts a value of 0, 1, 2, ... into the first 4 bytes of 
     * each page frame. */
    for (Addr frame = 0; frame < page_frames_total-1; ++frame) {
        Addr next = frame + 1;
        mem->put_bytes(frame*kPageSize, sizeof(Addr), reinterpret_cast<uint8_t*>(&next));
    }
    
    /* This code sets the first four bytes of the very last page frame
     * equal to 0xFFFFFFFF, which is our end-of-list marker */
    Addr end_list = kEndList;
    mem->put_bytes((page_frames_total-1)*kPageSize, sizeof(Addr), reinterpret_cast<uint8_t*>(&end_list));

}

bool PageFrameAllocator::Allocate(uint32_t count) {
    if (count <= page_frames_free) { // if enough to allocate
        Addr freeListHead_offset = free_list_head*kPageSize;
        
        uint32_t zero = 0;
        while (count-- > 0) {
            /* Clear page frame before handing it off */
            mem->get_bytes(reinterpret_cast<uint8_t*>(&free_list_head), freeListHead_offset, sizeof(Addr));
            for(Addr i = 0; i < kPageSize; i+=sizeof(uint32_t)){
                mem->put_bytes(freeListHead_offset, sizeof(uint32_t), reinterpret_cast<uint8_t*>(&zero));  
            }
            //free_list_head++;
            freeListHead_offset = free_list_head*kPageSize;
            --page_frames_free;
        }
        return true;
    } else {
        return false; // do nothing and return error
    }
}

bool PageFrameAllocator::Deallocate(uint32_t count,
                                    std::vector<uint32_t> &page_frames) {
  // If enough to deallocate
  if(count <= page_frames.size()) {
    while(count-- > 0) {
      // Return next frame to head of free list
      uint32_t frame = page_frames.back();
      page_frames.pop_back();
      memcpy(&memory[frame * kPageSize], &free_list_head, sizeof(uint32_t));
      free_list_head = frame;
      ++page_frames_free;
    }
    return true;
  } else {
    return false; // do nothing and return error
  }
}

std::string PageFrameAllocator::FreeListToString(void) const {
  std::ostringstream out_string;
  
  uint32_t next_free = free_list_head;
  
  while (next_free != kEndList) {
    out_string << " " << std::hex << next_free;
    memcpy(&next_free, &memory[next_free*kPageSize], sizeof(uint32_t));
  }
  
  return out_string.str();
}