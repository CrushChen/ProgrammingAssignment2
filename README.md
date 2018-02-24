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
- PageFrameAllocator
  - Total size of physical memory is 256 pages (1MB).
  #### Allocation *alloc* *vaddr* *size*
  - The number of page frames to allocate will be multiplied by 0x1000, the size of a page frame. 
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
