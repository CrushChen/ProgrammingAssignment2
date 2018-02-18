/* 
 * AlphaHistogram - compute histogram of characters in source file
 * 
 * File:   AlphaHistogram.cpp
 * Author: Mike Goss <mikegoss@cs.du.edu>
 * 
 * Created on December 14, 2017, 3:48 PM
 */

#include "AlphaHistogram.h"

#include <iostream>
#include <fstream>

AlphaHistogram::AlphaHistogram(const std::string &file_name) {
  // Initialize histogram size and set to all 0
  histogram.resize(char_range, 0);

  // Open the input file
  std::ifstream text_file;
  text_file.open(file_name);
  if (text_file.fail()) {
    std::cerr << "ERROR: file not found: " << file_name << "\n";
    exit(2);
  }

  // Read each character and update histogram
  unsigned char c;
  while (text_file >> c) {                  // while another character in file
    if (c >= low_char && c <= high_char) {  // if in range
      ++histogram[c - low_char];
    }
  }
  
  // If terminated for reason other than end of file
  if (!text_file.eof()) {
    std::cerr << "ERROR: failure while reading file: " << file_name << "\n";
    exit(2);
  }
  
  text_file.close();
}

unsigned long AlphaHistogram::count(unsigned char c) const {
  if (c >= low_char && c <= high_char) {  // if in range
    return histogram[c - low_char];
  } else {
    return 0;  // return 0 if out of range
  }
}
