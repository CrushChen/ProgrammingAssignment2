/*
 * ProcessTrace implementation 
 */

/* 
 * File:   ProcessTrace.cpp
 * Author: Mike Goss <mikegoss@cs.du.edu>
 * 
 */

#include "ProcessTrace.h"
#include "Exceptions.h"
#include <algorithm>
#include <cctype>
#include <iomanip>
#include <iostream>
#include <sstream>

using mem::Addr;
using mem::MMU;
using std::cin;
using std::cout;
using std::cerr;
using std::getline;
using std::istringstream;
using std::string;
using std::vector;

ProcessTrace::ProcessTrace(string file_name_)
: file_name(file_name_), line_number(0) {
    // Open the trace file.  Abort program if can't open.
    trace.open(file_name, std::ios_base::in);
    if (!trace.is_open()) {
        cerr << "ERROR: failed to open trace file: " << file_name << "\n";
        exit(2);
    }
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

void ProcessTrace::CmdAlloc(const string &line,
        const string &cmd,
        const vector<uint32_t> &cmdArgs) {
    // Allocate the specified memory size
    Addr page_count = (cmdArgs.at(0) + mem::kPageSize - 1) / mem::kPageSize;
    memory = std::make_unique<MMU>(page_count);
}

void ProcessTrace::CmdCompare(const string &line,
                              const string &cmd,
                              const vector<uint32_t> &cmdArgs) {
  uint32_t addr = cmdArgs.at(0);

  // Compare specified byte values
  size_t num_bytes = cmdArgs.size() - 1;
  uint8_t buffer[num_bytes];
  try{
  memory->get_bytes(buffer, addr, num_bytes);
  }
  catch(mem::PageFaultException e) {
         << "PageFaultException at virtual address "
                << std::hex << "0x" << addr ; mem::PMCB.operation_state=mem::PMCB::NONE ;
      }
  for (int i = 1; i < cmdArgs.size(); ++i) {
    if(buffer[i-1] != cmdArgs.at(i)) {
      cout << "compare error at address " << std::hex << addr
              << ", expected " << static_cast<uint32_t>(cmdArgs.at(i))
              << ", actual is " << static_cast<uint32_t>(buffer[i-1]) << "\n";
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
      try{
     buffer[i-1] = cmdArgs.at(i);
      }
      catch(mem::PageFaultException e) {
         << "PageFaultException at virtual address "
                << std::hex << "0x" << cmdArgs.at(i) ; mem::PMCB.operation_state=mem::PMCB::NONE ;
      }
  }
  try{
  memory->put_bytes(addr, num_bytes, buffer);
  }
   catch(mem::PageFaultException e) {
         << "PageFaultException at virtual address "
                << std::hex << "0x" << cmdArgs.at(i) ; mem::PMCB.operation_state=mem::PMCB::NONE ;
      }
}

void ProcessTrace::CmdCopy(const string &line,
                           const string &cmd,
                           const vector<uint32_t> &cmdArgs) {
  // Copy specified number of bytes to destination from source
  Addr dst = cmdArgs.at(0);
  Addr src = cmdArgs.at(1);
  Addr num_bytes = cmdArgs.at(2);
  uint8_t buffer[num_bytes];
  try{
  memory->get_bytes(buffer, src, num_bytes);
  }
   catch(mem::PageFaultException e) {
        << "PageFaultException at virtual address "
                << std::hex << "0x" << src; mem::PMCB.operation_state=mem::PMCB::NONE ;
      }
  try{
  memory->put_bytes(dst, num_bytes, buffer);
  }
   catch(mem::PageFaultException e) {
         << "PageFaultException at virtual address "
                << std::hex << "0x" << dst ;
          mem::PMCB.operation_state=mem::PMCB::NONE ;
      }
}

void ProcessTrace::CmdFill(const string &line,
                          const string &cmd,
                          const vector<uint32_t> &cmdArgs) {
  // Fill a sequence of bytes with the specified value
  Addr addr = cmdArgs.at(0);
  Addr num_bytes = cmdArgs.at(1);
  uint8_t val = cmdArgs.at(2);
  for (int i = 0; i < num_bytes; ++i) {
      try{
    memory->put_byte(addr++, &val);
      }
       catch(mem::WritePermissionFaultException e) {
     
    
         << "WritePermissionFaultException "
                << std::hex << "0x" << addr ;
          mem::PMCB.operation_state=mem::PMCB::NONE ;
      }
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
  for(int i = 0; i < count; ++i) {
    if((i % 16) == 0) {  // line break every 16 bytes
      cout << "\n";
    }
    uint8_t byte_val;
    try{
    memory->get_byte(&byte_val, addr++);
    }
     catch(mem::PageFaultException e) {
        << "PageFaultException at virtual address "
                << std::hex << "0x" << addr;
        mem::PMCB.operation_state=mem::PMCB::NONE ;
        
      }
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

