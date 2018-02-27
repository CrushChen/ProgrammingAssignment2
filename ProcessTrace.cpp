void ProcessTrace::CmdAlloc(const string &line,
        const string &cmd,
        const vector<uint32_t> &cmdArgs) {
    Addr vaddr = cmdArgs.at(0);
    Addr num_bytes = cmdArgs.at(1);
    if(num_bytes % 0x1000 != 0){
        cerr << "Allocation not a multiple of page frame size" << std::endl;
        exit(3);
    }
    /* Switch to physical mode */
    memory->set_PMCB(physical_pmcb);
    
    uint32_t numFrames = num_bytes / 0x1000;
    uint32_t numPTs = 0;
    uint32_t count = 0;
    
    /* Verify that we have enough free page frames to accommodate the entire 
     * alloc command */
    PageTable dir;
    Addr dir_base = physical_pmcb.page_table_base;// Get our page directory (1st level page table)
    /* Now read the page directory */
    try {
        memory->get_bytes(reinterpret_cast<uint8_t*> (&dir), dir_base, kPageTableSizeBytes);
    } catch (PageFaultException e) {
        cout << "Page fault exception while reading page directory.\n";
    }
    
    /* Count how many page tables we will need to allocate (if any) 
     * It's possible that we have to both allocate a page table and then
     * the page in that page table, requiring us to allocate extra frames */
    uint32_t tempVAddr = vaddr;
    while(count++ < numFrames) {
        Addr dir_index = ((tempVAddr >> (kPageSizeBits + kPageTableSizeBits)) & kPageTableIndexMask);
        bool pageTable_exists = dir[dir_index] & kPTE_PresentMask; 
        if(pageTable_exists){
            numPTs++;
        }
        tempVAddr += count*kPageSize;
    }
    
    count = 0;
    numFrames += numPTs;
    
    if(allocator->get_page_frames_free() >= numFrames){
        /* Get our page directory (1st level page table) */
        PageTable dir;
        Addr dir_base = physical_pmcb.page_table_base;
        /* Now read the page directory */
        try {
            memory->get_bytes(reinterpret_cast<uint8_t*>(&dir), dir_base, kPageTableSizeBytes);
        } catch (PageFaultException e){
            cout << "Page fault exception while reading page directory.\n";
        }
        
        /* While we have page frames to allocate */
        while(count++ < numFrames){
            Addr frame_pAddr = allocator->get_free_list_head();
            Addr dir_index = ((vaddr >> (kPageSizeBits + kPageTableSizeBits)) & kPageTableIndexMask);
            Addr l2_offset = (vaddr >> kPageSizeBits) & kPageTableIndexMask;
            
            /* Find if a page table at this vaddr already exists by checking
             * its present bit */
            bool pageTable_exists = dir[dir_index] & kPTE_PresentMask;             

            /* If we have room to allocate */
            if(allocator->Allocate(1)){
                /* Compute some offsets */
                PageTable l2_temp;
                
                /* If we don't already have a page table at this addr, allocate
                 * another frame. */
                if(!pageTable_exists){
                    Addr ptAddr = allocator->get_free_list_head();
                    /* Check that we can allocate another frame */
                    if(allocator->Allocate(1)){                       
                        dir[dir_index] = ptAddr | kPTE_PresentMask | kPTE_WritableMask;
                        memory->put_bytes(dir_base, kPageTableSizeBytes, 
                                reinterpret_cast<uint8_t*>(&dir));
                    }
                    memory->put_bytes(ptAddr, kPageTableSizeBytes,
                            reinterpret_cast<uint8_t*>(&l2_temp));                    
                }
                
                /* Specific (L3) page inside of our L2 page table */
                Addr l2_pAddr = (dir[dir_index] & 0xFFFFF000);
                try {
                    /* If we DO have a page table, read that page table
                     * into l2_temp */
                    memory->get_bytes(reinterpret_cast<uint8_t*> (&l2_temp), 
                            l2_pAddr, kPageTableSizeBytes);
                } catch (PageFaultException e) {
                    cout << "Page fault exception while reading L2 PT.\n";
                }
                
                /* Determine if page in L2 table maps to something */
                bool pageEntry_exists = l2_temp[l2_offset] & kPTE_PresentMask;
                if(!pageEntry_exists){
                    l2_temp[l2_offset] = frame_pAddr | kPTE_PresentMask | kPTE_WritableMask;
                    memory->put_bytes(l2_pAddr, kPageTableSizeBytes,
                            reinterpret_cast<uint8_t*>(&l2_temp));       
                }
            }
            
            /* Move to the next vaddr */
            vaddr += count*kPageSize;
        }    
    }
    /* Switch back to virtual mode */
    memory->set_PMCB(virtual_pmcb);       
}
