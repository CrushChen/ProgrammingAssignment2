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

using std::cin;
using std::cout;
using std::cerr;
using std::getline;
using std::istringstream;
using std::string;
using std::vector;

using namespace mem;

ProcessTrace::ProcessTrace(std::string file_name_, mem::MMU &memory_, PageFrameAllocator &allocator_)
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
    //Addr directory_offset = (DIRECTORY_BASE >> (kPageSizeBits + mem::kPageTableSizeBits)) & mem::kPageTableIndexMask;
    //const Addr kVaddrStart = (255 << (mem::kPageTableSizeBits + kPageSizeBits)) 
    //+ ((mem::kPageTableEntries - 1) << kPageSizeBits);
    Addr directory_physical = allocator->get_free_list_head() * mem::kPageSize;
    memory->put_bytes(directory_physical, mem::kPageTableSizeBytes, //Write page directory to memory
            reinterpret_cast<uint8_t*> (&page_directory));
    virtual_pmcb(true, directory_physical); // load to start virtual mode
    memory->set_PMCB(vm_pmcb);
   
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
    int num_frames = std::ceil(num_bytes / 0x1000); //# of frames needed
    
    if(allocator->get_page_frames_free() <= 0){ //make sure there is room for alloc
        cerr << "No free frames to allocate" << std::endl;
        exit(3);
    } else {
        //Check if existing page table (examine starting virtual address & present bit in page directory)
        Addr directory_offset = (vaddr >> (mem::kPageSizeBits + mem::kPageTableSizeBits) & mem::kPageTableIndexMask);
        PageTable directory;
        memory->set_PMCB(physical_pmcb);
        Addr directory_base = physical_pmcb.page_table_base;
        try{
             memory->get_bytes(reinterpret_cast<uint8_t*> (&directory), directory_base, mem::kPageTableSizeBytes);
        }
        catch(mem::PageFaultException e){
            std::cout << "PageFaultException while accessing page directory";
        }
        
        
        
        
        uint8_t present;
        try {
            memory->get_byte(&present, VIRTUAL_BASE - 1);
        } catch(mem::PageFaultException e){
            std::cout << "PageFaultException while allocating at virtual address: "
                    << std::hex << "0x" << VIRTUAL_BASE;
        }
        
        
        
        //Addr table_offset = ((vaddr + mem::kPageSize) >> kPageSizeBits) & mem::kPageTableIndexMask;
        //uint8_t table_exists;
        //memory->get_byte(&table_exists, vaddr);
     
        //if(vaddr >> DIRECTORY_BASE & kPTE_PresentMask){ //page table already exists
            /***
             * TODO: 
             * check bounds of allocation
             * ->check for existing page frames
             * ->check if need to write another page table
             * allocate page frames based on num_bytes
             ***/
            
            
            
            
            
            //Allocating new page table
            //switch to physical mode
            
            memory->set_PMCB(physical_pmcb);
            
            
            PageTable temp;
            Addr temp_directory_offset = (VIRTUAL_BASE >> (mem::kPageSizeBits + mem::kPageTableSizeBits)) & mem::kPageTableIndexMask;
            Addr temp_physical = allocator->get_free_list_head();
            Addr temp_offset = (VIRTUAL_BASE >> mem::kPageSizeBits) & mem::kPageTableIndexMask;
            temp[temp_offset] = temp_physical | mem::kPTE_PresentMask | mem::kPTE_WritableMask;
            
            memory->get_bytes(reinterpret_cast<uint8_t*> (&directory), directory_base, mem::kPageTableSizeBytes);
            directory[temp_directory_offset] = temp_physical | mem::kPTE_PresentMask | mem::kPTE_WritableMask;
            
            memory->put_bytes(directory_base, mem::kPageTableSizeBytes, reinterpret_cast<uint8_t*>(&directory));            
            memory->put_bytes(temp_physical, mem::kPageTableSizeBytes, reinterpret_cast<uint8_t*>(&temp));
            
            //switch back to virtual mode
            //PMCB virtual_pmcb(true, directory_base); // load to start virtual mode
            memory->set_PMCB(virtual_pmcb);
        //}
        
    }
        
    
    
    // Allocate the specified memory size
    //  Addr page_count = (cmdArgs.at(0) + mem::kPageSize - 1) / mem::kPageSize;
    //  memory = std::make_unique<MMU>(page_count);
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

    /***
     * TODO:
     * change writable status of size bytes of memory starting at virtual
     * address vaddr. The starting address, vaddr, and the byte count, size,
     * must be exact multiples of the pages size (0x1000).
     * -If status is 0, the Writable bit in the 2nd level page table should be
     * cleared for all Present pages in the range
     * -If status isn't 0, the Writable bit in the 2nd level page table should be 
     * set for all Present pages in the range
     * -Any pages in the range which are not Present should be ignored
     * -The first level page table should not be changed
     ***/
}

void ProcessTrace::CmdComment(const std::string& line) {
    cout << line << std::endl;
}
