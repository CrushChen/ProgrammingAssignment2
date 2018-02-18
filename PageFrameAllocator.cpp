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

PageFrameAllocator::PageFrameAllocator(uint32_t page_frame_count) 
: memory(page_frame_count * kPageSize),
        page_frames_total(page_frame_count),
        page_frames_free(page_frame_count),
        free_list_head(0)
{
  // Add all page frames to free list
  for (uint32_t frame = 0; frame < page_frame_count-1; ++frame) {
    uint32_t next = frame + 1;
    memcpy(&memory[frame*kPageSize], &next, sizeof(uint32_t));
  }
  
  // Last page frame has end of list marker
  uint32_t end_list = kEndList;
  memcpy(&memory[(page_frame_count-1)*kPageSize], &end_list, sizeof(uint32_t));
}

bool PageFrameAllocator::Allocate(uint32_t count, 
                                  std::vector<uint32_t> &page_frames) {
  if (count <= page_frames_free) {  // if enough to allocate
    while (count-- > 0) {
      // Return next free frame to caller
      page_frames.push_back(free_list_head);
      
      // De-link frame from head of free list
      memcpy(&free_list_head, &memory[free_list_head*kPageSize], sizeof(uint32_t));
      --page_frames_free;
    }
    return true;
  } else {
    return false;  // do nothing and return error
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