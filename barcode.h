#pragma once

#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/calib3d/calib3d.hpp"
#include "opencv2/highgui/highgui.hpp"

#include <iostream>
#include <exception>
#include <utility>
#include <unordered_map>
#include <string>

static const std::unordered_map<int, std::pair<int, char> > LG_EAN_map =
{
  {3211, {0, 'L'}}, {1123, {0, 'G'}},
  {2221, {1, 'L'}}, {1222, {1, 'G'}},
  {2122, {2, 'L'}}, {2212, {2, 'G'}},
  {1411, {3, 'L'}}, {1141, {3, 'G'}},
  {1132, {4, 'L'}}, {2311, {4, 'G'}},
  {1231, {5, 'L'}}, {1321, {5, 'G'}},
  {1114, {6, 'L'}}, {4111, {6, 'G'}},
  {1312, {7, 'L'}}, {2131, {7, 'G'}},
  {1213, {8, 'L'}}, {3121, {8, 'G'}},
  {3112, {9, 'L'}}, {2113, {9, 'G'}}  
};    

static const std::unordered_map<int, std::pair<int, char> > R_EAN_map =
{
  {3211, {0, 'R'}},
  {2221, {1, 'R'}},
  {2122, {2, 'R'}},
  {1411, {3, 'R'}},
  {1132, {4, 'R'}},
  {1231, {5, 'R'}},
  {1114, {6, 'R'}},
  {1312, {7, 'R'}},
  {1213, {8, 'R'}},
  {3112, {9, 'R'}}
};

static const std::unordered_map<std::string, int> EAN_13_map =
{
 {"LLLLLL", 0},
 {"LLGLGG", 1},
 {"LLGGLG", 2},
 {"LLGGGL", 3},
 {"LGLLGG", 4},
 {"LGGLLG", 5},
 {"LGGGLL", 6},  
 {"LGLGLG", 7},  
 {"LGLGGL", 8},  
 {"LGGLGL", 9}
};

namespace barcode_exceptions
{
  class null_image_exception: public std::exception
  {
    virtual const char* what() const throw()
      {
        return "Barcode image is null.";
      }
  };

  class not_gray_image_exception: public std::exception
  {
    virtual const char* what() const throw()
      {
        return "Barcode image is not GRAY.";
      }
  };
}

class Barcode
{
private:
  // Members:
  // code structure
  static constexpr uchar black_color = (uchar) 0;
  static constexpr uchar white_color = (uchar) 255;
  static constexpr int left_guards_size = 3;
  static constexpr int middle_guards_size = 5;
  static constexpr int right_guards_size = 3;
  static constexpr int left_code_size = 24;
  static constexpr int right_code_size = 24;
  int left_guards[3] = {};
  int middle_guards[5] = {};
  int right_guards[3] = {};
  int left_code[24] = {};
  int right_code[24] = {};

  // input image data
  const cv::Mat * const image = nullptr;
  const int max_image_index = 0; // max column number

  // internal flags
  bool has_barcode_structure = false;
  bool is_identified = false;
  bool is_correct = false;

  // decoded number
  static constexpr int barcode_number_size = 13;
  static constexpr int check_sum_coeffs[13] = {1, 3, 1, 3, 1, 3, 1, 3, 1, 3 , 1, 3, 1};
  int barcode_number[13] = {};
  
  // Methods:
  void decode ();
  void decode_row (const uchar * const test_row);
  void constract_row_structure (const uchar * const test_row);
  int find_row_structure_part_beg (const uchar * const test_row, const int beg_index, const int part_color);
  int constract_row_structure_part (const uchar * const test_row, const int beg_index, const uchar first_color, 
                                    const int part_size, int *part, bool &part_filled);
  void identify_barcode_number ();
  int decode_single_number (const float h, const int l_0, const int l_1, const int l_2, const int l_3);
  void check_control_number ();
  void clear_data ();
  
public:
  Barcode (const cv::Mat * const gray_im);  
  bool is_decoded () const { return is_correct; }
  long long get_barcode_number () const;
  void print_barcode () const;
};