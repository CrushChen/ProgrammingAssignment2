/*
 * Main class for Assignment2
 * The trace file name should be specified as the first (and only) command line
 * argument to the program. 
 */

/* 
 * File:   main.cpp
 * Created By: Peter Gish
 * Last Modified: 2/17/18
 */

#include <cstdlib>
#include <iostream>
#include <MMU.h>

#include "ProcessTrace.h"

using namespace std;

/*
 * Create an instance of the MMU class with 256 (0x100) page frames (1MB of simulated
 * physical memory). Do not enable TLB. Need to enable virtual memory mode
 * and construct page tables
 */
int main(int argc, char** argv) {
    mem::MMU mem(0x100);
    if(argc != 2){
        std::cerr << "usage: Assignment 2 input_file" << std::endl;
        exit(1);
    }
    ProcessTrace trace(argv[1]);
    trace.Execute();
    return 0;
}

