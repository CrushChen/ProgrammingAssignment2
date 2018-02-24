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
