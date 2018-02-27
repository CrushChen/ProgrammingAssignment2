/*
 * ProcessTrace
 * 
 * Memory Reference Traces:
 * -Total size of physical memory is fixed at 256 pages (1MB)
 * 
 * Traces:
 * ***All addresses are virtual addresses****
 * The program is responsible for allocating physical memory and mapping 
 * virtual addresses to physical addresses uses PAGE TABLES and the MMU class
 * 
 * ***Must use MMU and PAGE TABLES for all address mapping***
 * Trace File Format:
 * Trace records contain multiple fields, separated by white space (blanks and tabs).
 * Each line consists of a command name, followed by optional hexadecimal integer 
 * arguments to the command. The command name is case sensitive (all lower case).
 * 
 * The trace file will contain the following record types. All numerical values
 * (including counts) are hexadecimal, without a leading "0x"
 * ***Any output should also use hexadecimal format for numeric data except where
 * otherwise specified***
 * 
 * COMMANDS:
 * -Allocate Memory
 *      alloc vaddr size
 * Allocate virtual memory for size bytes, starting at virtual address vaddr. 
 * The starting address,vaddr, and the byte count, size, must be exact multiples
 * of the page size (0x1000). The first line of the file must be an alloc command.
 * Subsequent alloc commands add additional blocks of allocated virtual memory, they
 * do not remove earlier allocations. All pages should be marked Writable in the 1st
 * and 2nd level page tables when initially allocated. All newly-allocated memory
 * must be initialized to all 0.
 * 
 * -Compare Bytes
 *      compare addr expected_values
 * Compare bytes starting at addr; expected_values is a list of bytes values, 
 * separated by white space. If the actual values of bytes starting at addr don't match
 * the expected_values, write an error message to standard error for each mismatch with,
 * the address, the expected value, and the actual value (all in hexadecimal). Follow
 * the format shown in the sample output in the assignment
 * 
 * -Put Bytes
 *      put addr values
 * Store values starting at addr; values is a list of byte values, separated by white space
 * 
 * -Fill Bytes
 *      fill addr count value
 * Store count copies of value starting at addr
 * 
 * -Copy Bytes
 *      copy dest_addr src_addr count
 * Copy count bytes from src_addr to dest_addr. The source and destination ranges
 * will not overlap
 * 
 * -Dump Bytes
 *      dump addr count
 * Write a line with addr to standard output, followed by separate lines by count bytes
 * starting at addr. Write 16 bytes per line, with a space between adjacent values. Print
 * each byte as exactly 2 digits with a leading - for values less than 10 (hex). For
 * example, to print 24 bytes starting at 3fa700:
 * 3fa700
 *  00 12 f3 aa 00 00 00 a0 ff ff e7 37 21 08 6e 00
 *  55 a5 9a 9b 9c ba fa f0
 * 
 * -Writable Status
 *      writable vaddr size status
 * Change the writable status of size bytes of memory, starting at virtual address vaddr.
 * The starting address, vaddr, and the byte count, size, must be exact multiples of the 
 * page size (0x1000). If status is 0, the Writable bit in the 2nd level page table should
 * be cleared for all Present pages in the range, otherwise the Writable bit in the 2nd
 * level page table should be set for all Present pages in the range. Any pages in the range 
 * which are not present should be ignored. 
 * ***The first level page table should not be changed***
 *
 * -Comment
 *      # comment text
 * The # character in the first column means the remainder of the line should be treated
 * as a comment. The command should be echoed to output in the same way as other commands,
 * but should otherwise be ignored.
 * 
 * 
 * EXCEPTION HANDLING
 * Some trace files will be designed to generate exceptions of type PageFaultException
 * or WritePermissionFaultException (defined in MMU header file Exceptions.h). When one 
 * of these exceptions occurs, you should catch the exception, and write a message to standard
 * output (not standard error) containing the type of the exception, the virtual address
 * at which the exception occurred, and any string returned by the what() method of the exception.
 * Then set the operation_state field of the PMCB to mem::PMCB::NONE to cancel any partially
 * executed instruction (otherwise the exception will immediately reoccur), and continue executing
 * the trace at the next line.
 * No other exceptions should occur during the execution of trace files. Occurrences of other
 * exceptions will usually indicate an error in your program
 */

/* Modifications:
 * -The alloc command allocates pages within the virtual address space of the process trace.
 * Multiple alloc commands are allowed in the trace file. A trace can simultaneously allocate multiple
 * blocks of memory starting at different virtual addresses. Each block must be a multiple 
 * of the page frame size (0x1000 = 4096 bytes). The program must allocate blocks in MMU
 * physical memory and map them into the process address space using the page tables.
 * ***Need to switch out of virtual mode to modify page tables, then switch back into virtual mode 
 * to execute trace commands***
 * 
 * -Modify the memory allocator to allocate pages from the MMU memory (use the allocator
 * for both page table pages and user pages). Make the memory allocator and MMU 
 * instances separate objects from ProcessTrace that they can be used by all the instances of 
 * ProcessTrace. Each instance of ProcessTrace will need to have its own set of page 
 * frames allocated.
 *
 */

/* 
 * File:   ProcessTrace.h
 * Created By: Peter Gish
 * Last Modified: 2/17/18
 */

#ifndef PROCESSTRACE_H
#define PROCESSTRACE_H

#include <MMU.h>
#include "PageFrameAllocator.h"

#include <fstream>
#include <string>
#include <vector>

class ProcessTrace {
public:
  /**
   * Constructor - open trace file, initialize processing
   * 
   * @param file_name_ source of trace commands
   */
  ProcessTrace(std::string file_name_, mem::MMU &memory_, PageFrameAllocator &allocator_);
  
  /**
   * Destructor - close trace file, clean up processing
   */
  virtual ~ProcessTrace(void);

  // Other constructors, assignment
  ProcessTrace(const ProcessTrace &other) = delete;
  ProcessTrace(ProcessTrace &&other) = delete;
  ProcessTrace operator=(const ProcessTrace &other) = delete;
  ProcessTrace operator=(ProcessTrace &&other) = delete;
  
  /**
   * Execute - read and process commands from trace file using the MMU as 
   * the memory referenced by the commands.
   * Before executing each command, write the decimal line number to standard output,
   * followed by a colon (:), followed by the input line, exactly as read  
   * 
   */
  void Execute(void);
  
private:
  // Trace file
  std::string file_name;
  std::fstream trace;
  long line_number;

  // Memory contents
  mem::MMU* memory;
  PageFrameAllocator* allocator;

  const mem::PMCB physical_pmcb;

  /**
   * ParseCommand - parse a trace file command.
   *   Aborts program if invalid trace file.
   * 
   * @param line return the original command line
   * @param cmd return the command name
   * @param cmdArgs returns a vector of argument bytes
   * @return true if command parsed, false if end of file
   */
  bool ParseCommand(
      std::string &line, std::string &cmd, std::vector<uint32_t> &cmdArgs);
  
  /**
   * Command executors. Arguments are the same for each command.
   *   Form of the function is CmdX, where "X' is the command name, capitalized.
   * @param line original text of command line
   * @param cmd command, converted to all lower case
   * @param cmdArgs arguments to command
   */
  void CmdAlloc(const std::string &line, 
               const std::string &cmd, 
               const std::vector<uint32_t> &cmdArgs);
  void CmdCompare(const std::string &line, 
               const std::string &cmd, 
               const std::vector<uint32_t> &cmdArgs);
  void CmdPut(const std::string &line, 
               const std::string &cmd, 
               const std::vector<uint32_t> &cmdArgs);
  void CmdFill(const std::string &line, 
               const std::string &cmd, 
               const std::vector<uint32_t> &cmdArgs);
  void CmdCopy(const std::string &line, 
               const std::string &cmd, 
               const std::vector<uint32_t> &cmdArgs);
  void CmdDump(const std::string &line, 
               const std::string &cmd, 
               const std::vector<uint32_t> &cmdArgs);
  void CmdWritable(const std::string &line,
               const std::string &cmd, 
               const std::vector<uint32_t> &cmdArgs);
  void CmdComment(const std::string &line);
};

#endif /* PROCESSTRACE_H */

