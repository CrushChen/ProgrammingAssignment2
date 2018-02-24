# ProgrammingAssignment2

## Read test files (MemorySubsystem/tests) for example usage of MMU
## Process Synchronization 1.pdf

### Peter
- Modify ProcessTrace
  - Alloc

### Chenchen
- Modify ProcessTrace
  - All commands (compare, put, fill, copy, dump, *?writeable status?*)

### Tristan
#### PageFrameAllocator
  - Total size of physical memory is 256 (0x100) pages (1MB).
  - You'll need to modify the memory allocator from Lab 3 to allocate pages from the MMU memory. Use the allocator for both page table pages **and** user pages.
  - In Programming Assignment 3, you'll need to have multiple ProcessTrace instances -- **so make the memory allocator and MMU instance seperate objects from ProcessTrace** that can be used by all instances of ProcessTrace. Each instance of ProcessTrace will need to have its own set of page frames allocated. 
  #### Allocation *alloc* *vaddr* *size*
  - The number of page frames to allocate will be multiplied by 0x1000, the size of a page frame. 
  - During initialization, ProcessTrace should allocate an empty 1st level page table.
  - Alloc allocates pages within the **virtual** *and* **physical** address space.
  - Must allocate blocks in MMU physical memory, and map them into the process address space using the page tables.
    - **Must** use MMU and page tables for all address mapping.
  - Can simultaneously allocate multiple blocks of memory starting at different virtual addresses.
  - Each page allocated will be writable by default, but may be made writable or non-writable during trace execution.
  - ##### Input: *alloc* *vaddr* *size*
    - Allocate virtual memory for *size* bytes starting at address *vaddr*. *vaddr* and *size* **must** be exact multiples of page size (0x1000).
    - Subsequent *alloc* commands **do not remove** earlier allocations. 
    - All pages should be marked *Writable* in the 1st and 2nd level page tables when initially allocated, and these pages should be initialized to zero.
        - You will need to switch out of virtual mode to modify page tables, and switch back into virtual mode to execute trace commands.

##### Other Notes about PageFrameAllocator
- For pages not allocated to the process, the first 4 bytes will contain the link to the next page on the free list. When a page is allocated to the process (via Allocate function), you should clear the page to all zeros at that point. When a page is deallocated (via Deallocate function, you would then be able to use the first 4 bytes again for the link. 
    - The basic idea is that when a page is on the free list, it's controlled by the page frame allocator. When the page is removed from the free list, any information in the page should be erased before control of the page is given to the caller to Allocate.
- ProcessTrace allocates pages using PageFrameAllocator. It will need to allocate pages requested by the user **and** pages for the user process page table that will point to the requested pages.
- When ProcessTrace encounters an Allocate request, it should allocate pages from PageFrameAllocator for the pages needed for user memory. It will need to map those pages into the process page table, so if the 2nd level page tables that would be needed to map those pages are not already present in the page table, ProcessTrace will also need to allocate additional page(s) for the 2nd level page table.
- In the lab, PageFrameAllocator did memcpy to read and write to the simulated memory in a vector. In this assignment, put the MMU in non-virtual mode while doing allocations/deallocations, and replace memcpy calls with MMU get_bytes and put_bytes. You'll also need to share the MMU object with ProcessTrace: change both classes to pass a reference or pointer in the constructor.

### MMU
- On initialization (the first *alloc* command), the MMU will be in physical address mode. All virtual addresses are mapped directly to physical addresses. 
- #### Page Tables
    - Each page table is 1024 entries, each entry is 4 bytes (32 bits) long.
    - ##### 1st Level Page Table
        - The 1st level page table (the **directory**) contains entries pointing to second level page tables.
        - The upper 20 bits of each entry contain the page frame number of a second-level page table, **if** the present bit is set. If the present bit **is not** set, the entry is unused. 
        - **To allow writing to an address**, both the **1st** and **2nd** level entries must have the Present and Writable bits set.
    - ##### 2nd Level Page Table
        - Each entry with the Present bit set contains a page frame number in the upper 20 bits. 
        - The Writable bit determins if writes (puts) are allowed to the page.
        - The MMU will set the Accessed bit whenever the page is accessed. 
        - The MMU will set the Modified (dirty) bit whenever the page is written to.
        - You can initialize both the Accessed and Modified bits to zero and read them back later to determine if a page was accessed or modified.
