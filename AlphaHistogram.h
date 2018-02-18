/* 
 * AlphaHistogram - compute histogram of characters in source file
 * 
 * File:   AlphaHistogram.h
 * Author: Mike Goss <mikegoss@cs.du.edu>
 *
 * Created on December 14, 2017, 3:48 PM
 */

#ifndef ALPHAHISTOGRAM_H
#define ALPHAHISTOGRAM_H

#include <string>
#include <vector>

class AlphaHistogram {
public:
  /**
   * Constructor - read specified file and build histogram array
   * 
   * Throws exception on error.
   * 
   * @param file_name name of file to histogram
   */
  AlphaHistogram(const std::string &file_name);
  ~AlphaHistogram() {}
  
  // Rule of Five - disable other functionality (not strictly required
  //  since the class would work fine with the defaults of all of these,
  //  but required for this assignment)
  AlphaHistogram(const AlphaHistogram &orig) = delete;
  AlphaHistogram(AlphaHistogram &&orig) = delete;
  AlphaHistogram operator=(const AlphaHistogram &orig) = delete;
  AlphaHistogram operator=(AlphaHistogram &&orig) = delete;
  
  /**
   * count - return number of occurrences of specified character
   * 
   * @param c character value
   * @return number of occurrences in histogram if in range, 0 otherwise
   */
  unsigned long count(unsigned char c) const;

  // Define range of characters to histogram
  static const unsigned char low_char = 0x21;
  static const unsigned char high_char = 0x7e;
  static const size_t char_range = high_char - low_char + 1;
  
private:
  // Histogram array
  std::vector<unsigned long> histogram;
};

#endif /* ALPHAHISTOGRAM_H */

