/*
 * ProcessTrace implementation 
 */

/* 
 * File:   ProcessTrace.cpp
 * Author: Mike Goss <mikegoss@cs.du.edu>
 * 
 */

#include "ProcessTrace.h"

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <iostream>
#include <sstream>

using namespace mem;
using std::cin;
using std::cout;
using std::cerr;
using std::getline;
using std::istringstream;
using std::string;
using std::vector;

ProcessTrace::ProcessTrace(std::string file_name_, MMU &memory_, PageFrameAllocator &allocator_)
: file_name(file_name_), line_number(0) {
    // Open the trace file.  Abort program if can't open.
    trace.open(file_name, std::ios_base::in);
    if (!trace.is_open()) {
        cerr << "ERROR: failed to open trace file: " << file_name << "\n";
        exit(2);
    }
    memory = &memory_;
    allocator = &allocator_;
    
    
    //Build an empty page-directory
    PageTable page_directory;
    memory->set_PMCB(physical_pmcb);
    Addr directory_physical = allocator->get_free_list_head() * mem::kPageSize;
    allocator->Allocate(1);
    memory->put_bytes(directory_physical, kPageTableSizeBytes, //Write page directory to memory
            reinterpret_cast<uint8_t*> (&page_directory));
    // load to start virtual mode
    const PMCB virtual_pmcb(true, directory_physical);
    memory->set_PMCB(virtual_pmcb);  
}

ProcessTrace::~ProcessTrace() {
    trace.close();
}

void ProcessTrace::Execute(void) {
    // Read and process commands
    string line; // text line read
    string cmd; // command from line
    vector<uint32_t> cmdArgs; // arguments from line

    // Select the command to execute
    while (ParseCommand(line, cmd, cmdArgs)) {
        if (cmd == "alloc") {
            CmdAlloc(line, cmd, cmdArgs); // allocate memory
        } else if (cmd == "compare") {
            CmdCompare(line, cmd, cmdArgs); // get and compare multiple bytes
        } else if (cmd == "put") {
            CmdPut(line, cmd, cmdArgs); // put bytes
        } else if (cmd == "fill") {
            CmdFill(line, cmd, cmdArgs); // fill bytes with value
        } else if (cmd == "copy") {
            CmdCopy(line, cmd, cmdArgs); // copy bytes to dest from source
        } else if (cmd == "dump") {
            CmdDump(line, cmd, cmdArgs); // dump byte values to output
        } else if (cmd == "writable") {
            CmdWritable(line, cmd, cmdArgs);
        } else if (cmd == "#") {
            CmdComment(line);
        } else {
            cerr << "ERROR: invalid command at line " << line_number << ":\n"
                    << line << "\n";
            exit(2);
        }
    }
}

bool ProcessTrace::ParseCommand(
        string &line, string &cmd, vector<uint32_t> &cmdArgs) {
    cmdArgs.clear();
    line.clear();

    // Read next line
    if (std::getline(trace, line)) {
        ++line_number;
        cout << std::dec << line_number << ":";

        // Make a string stream from command line
        istringstream lineStream(line);

        // Get command
        lineStream >> cmd;

        // Get arguments
        if (cmd != "#") {//remainder of line is not a comment
            cout << line << std::endl; //print remainder of command line
            uint32_t arg;
            while (lineStream >> std::hex >> arg) {
                cmdArgs.push_back(arg);
            }
        }
        return true;
    } else if (trace.eof()) {
        return false;
    } else {
        cerr << "ERROR: getline failed on trace file: " << file_name
                << "at line " << line_number << "\n";
        exit(2);
    }
}

/*
 * Must build and modify page tables for the process
 * On initialization of ProcessTrace, build an empty page-directory
 * The alloc command will add or modify second-level page tables
 * 
 * Allocate virtual memory for size bytes, starting at virtual address vaddr. 
 * The starting address, vaddr, and the byte count, size, must be exact multiple of the page size
 * (0x1000). The first line of the file must be an alloc command
 * Subsequent alloc commands add additional blocks of allocated virtual memory
 * they do not remove earlier allocations. All pages should be marked Writable in the
 * 1st and 2nd level page tables when initially allocated. All newly-allocated
 * memory must be initialized to 0
 */
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
    PMCB temp_pmcb;
    memory->get_PMCB(temp_pmcb);
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
        
        /* While we have page frames to allocate */
        while(count++ < numFrames){
            Addr frame_pAddr = allocator->get_free_list_head() * kPageSize;
            Addr dir_index = ((vaddr >> (kPageSizeBits + kPageTableSizeBits)) & kPageTableIndexMask);
            Addr l2_offset = (vaddr >> kPageSizeBits) & kPageTableIndexMask;
            
            /* Find if a page table at this vaddr already exists by checking
             * its present bit */
            bool pageTable_exists = dir[dir_index] & kPTE_PresentMask;             

            /* If we have room to allocate */
            if(allocator->Allocate(1)){
            //allocator->Allocate(1);
                /* Compute some offsets */
                PageTable l2_temp;
                
                /* If we don't already have a page table at this addr, allocate
                 * another frame. */
                if(!pageTable_exists){
                    Addr ptAddr = allocator->get_free_list_head() *kPageSize;
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
    memory->set_PMCB(temp_pmcb);       
}

void ProcessTrace::CmdCompare(const string &line,
        const string &cmd,
        const vector<uint32_t> &cmdArgs) {
    uint32_t addr = cmdArgs.at(0);

    // Compare specified byte values
    size_t num_bytes = cmdArgs.size() - 1;
    uint8_t buffer[num_bytes];
    memory->get_bytes(buffer, addr, num_bytes);
    for (int i = 1; i < cmdArgs.size(); ++i) {
        if (buffer[i - 1] != cmdArgs.at(i)) {
            cout << "compare error at address " << std::hex << addr
                    << ", expected " << static_cast<uint32_t> (cmdArgs.at(i))
                    << ", actual is " << static_cast<uint32_t> (buffer[i - 1]) << "\n";
        }
        ++addr;
    }
}

void ProcessTrace::CmdPut(const string &line,
        const string &cmd,
        const vector<uint32_t> &cmdArgs) {
    // Put multiple bytes starting at specified address
    uint32_t addr = cmdArgs.at(0);
    size_t num_bytes = cmdArgs.size() - 1;
    uint8_t buffer[num_bytes];
    for (int i = 1; i < cmdArgs.size(); ++i) {
        buffer[i - 1] = cmdArgs.at(i);
    }
    memory->put_bytes(addr, num_bytes, buffer);
}

void ProcessTrace::CmdCopy(const string &line,
        const string &cmd,
        const vector<uint32_t> &cmdArgs) {
    // Copy specified number of bytes to destination from source
    Addr dst = cmdArgs.at(0);
    Addr src = cmdArgs.at(1);
    Addr num_bytes = cmdArgs.at(2);
    uint8_t buffer[num_bytes];
    memory->get_bytes(buffer, src, num_bytes);
    memory->put_bytes(dst, num_bytes, buffer);
}

void ProcessTrace::CmdFill(const string &line,
        const string &cmd,
        const vector<uint32_t> &cmdArgs) {
    // Fill a sequence of bytes with the specified value
    Addr addr = cmdArgs.at(0);
    Addr num_bytes = cmdArgs.at(1);
    uint8_t val = cmdArgs.at(2);
    for (int i = 0; i < num_bytes; ++i) {
        memory->put_byte(addr++, &val);
    }
}

void ProcessTrace::CmdDump(const string &line,
        const string &cmd,
        const vector<uint32_t> &cmdArgs) {
    uint32_t addr = cmdArgs.at(0);
    uint32_t count = cmdArgs.at(1);

    // Output the address
    cout << std::hex << addr;

    // Output the specified number of bytes starting at the address
    for (int i = 0; i < count; ++i) {
        if ((i % 16) == 0) { // line break every 16 bytes
            cout << "\n";
        }
        uint8_t byte_val;
        memory->get_byte(&byte_val, addr++);
        cout << " " << std::setfill('0') << std::setw(2)
                << static_cast<uint32_t> (byte_val);
    }
    cout << "\n";
}

void ProcessTrace::CmdWritable(const std::string& line,
        const std::string& cmd,
        const std::vector<uint32_t>& cmdArgs) {
    //Command Format:
    //writable vaddr size status
    
    uint32_t vaddr = cmdArgs.at(0);
    uint32_t count = cmdArgs.at(1);
    bool status = cmdArgs.at(2);
    
    /* Set to physical -- we're modifying physical page table entries */
    PMCB temp_pmcb;
    memory->get_PMCB(temp_pmcb);
    memory->set_PMCB(physical_pmcb);
    
    PageTable dir;
    Addr dir_base = physical_pmcb.page_table_base;// Get our page directory (1st level page table)
    /* Now read the page directory */
    try {
        memory->get_bytes(reinterpret_cast<uint8_t*> (&dir), dir_base, kPageTableSizeBytes);
    } catch (PageFaultException e) {
        cout << "Page fault exception while reading page directory.\n";
    }
    Addr l2_offset;
    Addr dir_index = ((vaddr >> (kPageSizeBits + kPageTableSizeBits)) & kPageTableIndexMask);

    PageTable l2_temp; 
    Addr l2_pAddr = (dir[dir_index] & 0xFFFFF000);
    try {
        /* If we DO have a page table, read that page table
         * into l2_temp */
        memory->get_bytes(reinterpret_cast<uint8_t*> (&l2_temp),
                l2_pAddr, kPageTableSizeBytes);
    } catch (PageFaultException e) {
        cout << "Page fault exception while reading L2 PT.\n";
    }
    
    uint32_t num_frames = count/kPageSize;
    uint32_t i = 0;
    
    while(i++ < num_frames){
        l2_offset = (vaddr >> kPageSizeBits) & kPageTableIndexMask;
        
        /* Determine if page in L2 table maps to something */
        bool pageEntry_exists = l2_temp[l2_offset] & kPTE_PresentMask;
        if (pageEntry_exists) {
            if(!status){
                l2_temp[l2_offset] &= ~(kPTE_WritableMask);
            } else if (status){
                l2_temp[l2_offset] |= kPTE_WritableMask;
            }
            
            memory->put_bytes(l2_pAddr, kPageTableSizeBytes,
                    reinterpret_cast<uint8_t*> (&l2_temp));
        }
        vaddr += (i*kPageSize);
    }
    memory->set_PMCB(temp_pmcb);    
}

void ProcessTrace::CmdComment(const std::string& line) {
    cout << line << std::endl;
}
