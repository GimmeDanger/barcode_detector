#include "barcode.h"
#include <algorithm>

constexpr uchar Barcode::black_color;
constexpr uchar Barcode::white_color;
constexpr int   Barcode::left_guards_size;
constexpr int   Barcode::middle_guards_size;
constexpr int   Barcode::right_guards_size;
constexpr int   Barcode::left_code_size;
constexpr int   Barcode::right_code_size;
constexpr int   Barcode::barcode_number_size;
constexpr int   Barcode::check_sum_coeffs[];

Barcode::Barcode (const cv::Mat * const gray_im) : image (gray_im), max_image_index (gray_im->cols)
{ 
  if (image->channels () > 1)
    {
      barcode_exceptions::not_gray_image_exception notgray_im_ex;
      throw notgray_im_ex;  
    }
    
  decode ();  
}

void Barcode::decode ()
{ 
  for (int row_i = 0; row_i < image->rows; row_i++)
    {
      const uchar *test_row = image->ptr (row_i);
      decode_row (test_row);
      if (is_correct)
        return;
      clear_data ();
    }
  return;
}

void Barcode::decode_row (const uchar * const test_row)
{
  constract_row_structure (test_row);
  identify_barcode_number ();
  check_control_number ();
}

void Barcode::clear_data ()
{  
  memset (left_guards, 0, sizeof (int) * left_guards_size);
  memset (left_code, 0, sizeof (int) * left_code_size);
  memset (middle_guards, 0, sizeof (int) * middle_guards_size);  
  memset (right_code, 0, sizeof (int) * right_code_size);
  memset (right_guards, 0, sizeof (int) * right_guards_size);
  memset (barcode_number, 0, sizeof (int) * barcode_number_size);
}

void Barcode::print_barcode () const
{
  if (!is_correct)
    return;
  std::cout << barcode_number[0] << " ";
  for (int i = 0; i < 6; i++)
    std::cout << barcode_number[1 + i];
  std::cout << " ";
  for (int i = 0; i < 6; i++)
    std::cout << barcode_number[7 + i];
  std::cout << std::endl;
}

long long Barcode::get_barcode_number () const
{
  long long ans = 0;
  long long decimals = 1;
  for (int i = 0; i < barcode_number_size; i++)
    {
      ans += barcode_number[barcode_number_size - 1 - i] * decimals;
      decimals *= 10;
    }
  return ans;
}

void Barcode::constract_row_structure (const uchar * const test_row)
{
  bool is_left_guards_filled = false;
  bool is_middle_guards_filled = false;
  bool is_right_guards_filled = false;
  bool is_left_code_filled = false;
  bool is_right_code_filled = false;
  
  int left_guards_beg   = find_row_structure_part_beg (test_row, 0, black_color);
  int left_code_beg     = constract_row_structure_part (test_row, left_guards_beg, black_color, left_guards_size, 
                                                        left_guards, is_left_guards_filled);
  int middle_guards_beg = constract_row_structure_part (test_row, left_code_beg, white_color, left_code_size,
                                                        left_code, is_left_code_filled);
  int right_code_beg    = constract_row_structure_part (test_row, middle_guards_beg, white_color, middle_guards_size,
                                                        middle_guards, is_middle_guards_filled);
  int right_guards_beg  = constract_row_structure_part (test_row, right_code_beg, black_color, right_code_size,
                                                        right_code, is_right_code_filled);
  int right_guards_end  = constract_row_structure_part (test_row, right_guards_beg , black_color, right_guards_size, 
                                                        right_guards, is_right_guards_filled);
  
  if (is_left_guards_filled && is_left_code_filled && is_middle_guards_filled &&
      is_right_code_filled && is_right_guards_filled)
    {
      has_barcode_structure = true;
    }
  return;
}

int Barcode::find_row_structure_part_beg (const uchar * const test_row, const int beg_index, const int part_color)
{
  for (int index = beg_index; index < max_image_index; index++)
    if (test_row[index] == part_color)
      return index;
  return max_image_index;
}

int Barcode::constract_row_structure_part (const uchar * const test_row, const int beg_index, const uchar first_color,
                                           const int part_size, int *part, bool &part_filled)
{
  int index, part_index;
  for (part_index = 0; part_index < part_size; part_index++)
    part[part_index] = 0;
  
  uchar curr_color = first_color;
  uchar prev_color = (first_color == black_color) ? white_color : black_color;
  for (index = beg_index, part_index = 0; index < max_image_index && part_index < part_size; index++)
    {
      if (test_row[index] == prev_color)
        {
          std::swap (curr_color, prev_color);
          part_index++;
        }
      part[part_index]++;         
    }
  if (part_index != part_size)
    index = max_image_index;
  if (index < max_image_index)
    part_filled = true;
  return index; 
}

void Barcode::identify_barcode_number ()
{
  if (!has_barcode_structure) 
    return;

  std::string EAN_13;
  // guard length
  float h = 0.f;
  for (int i = 0; i < left_guards_size; i++)
    h += left_guards[i];
  for (int i = 0; i < middle_guards_size; i++)
    h += middle_guards[i];
  for (int i = 0; i < right_guards_size; i++)
    h += right_guards[i];
  h /= left_guards_size + middle_guards_size + right_guards_size;
  
  // std::cout << "decode left code" << std::endl;
  for (int i = 0; i < left_code_size; i += 4)
    {
      int n = decode_single_number (h, left_code[i], left_code[i + 1], left_code[i + 2], left_code[i + 3]);
      auto got = LG_EAN_map.find (n);
      if (got == LG_EAN_map.end ())
        {
          // std::cout << "Can`t decode number " << i / 4 + 1 << " in L-code." << std::endl;
          return;
        }
      else
        {
          barcode_number[i / 4 + 1] = (got->second).first;
          EAN_13 += (got->second).second;
        }
    }
  // std::cout << "decode right code" << std::endl;
  for (int i = 0; i < right_code_size; i += 4)
    {
      int n = decode_single_number (h, right_code[i], right_code[i + 1], right_code[i + 2], right_code[i + 3]);
      auto got = R_EAN_map.find (n);
      if (got == R_EAN_map.end ())
        {
          // std::cout << "Can`t decode number " << i / 4  + 1 << " in R-code." << std::endl;
          return;
        }
      else
        barcode_number[i / 4 + 7] = (got->second).first;
    }
  
  // std::cout << "decode 13-th number" << std::endl;
  auto got = EAN_13_map.find (EAN_13);
  if (got == EAN_13_map.end ())
    {
      // std::cout << "Can`t decode 13-th number!" << std::endl;
      return;
    }
  else
    barcode_number[0] = got->second;      
  
  is_identified = true;
}

int Barcode::decode_single_number (const float h, const int l_0, const int l_1, const int l_2, const int l_3)
{
  int max_bit_num = 7;
  float hl_0 = l_0 / h;
  float hl_1 = l_1 / h;
  float hl_2 = l_2 / h;
  float hl_3 = l_3 / h;
  
  // std::cout << hl_0 << " " << hl_1 << " " << hl_2 << " " << hl_3 << " " << std::endl;
  
  float hl_0_1 = hl_0 + hl_1;
  float hl_2_3 = hl_2 + hl_3;
  
  if (hl_0 < 1.f || hl_1 < 1.f)
    {
      if (hl_0 < 1.f) 
        {
          hl_0 = 1.f;
          hl_1 = hl_0_1 - 1;
          hl_1 = (int) hl_1;
        }
      else if (hl_1 < 1.f) 
        {
          hl_1 = 1.f;
          hl_0 = hl_0_1 - 1;
          hl_0 = (int) hl_0;
        }
    }
  else if ((int) hl_0_1 > (int) hl_0 + (int) hl_1)
    {
      if (hl_0 - (int) hl_0 > hl_1 - (int) hl_1)
        hl_0 = (int) hl_0 + 1, hl_1 = (int) hl_1;
      else if (hl_0 - (int) hl_0 < hl_1 - (int) hl_1)
        hl_1 = (int) hl_1 + 1, hl_0 = (int) hl_0;
      else
        {
          if ((int) (hl_1 + hl_2) > (int) hl_1 + (int) hl_2)
            hl_0 = (int) hl_0 + 1, hl_1 = (int) hl_1;
          else
            hl_1 = (int) hl_1 + 1, hl_0 = (int) hl_0;
        }
      
    }            
  
  if (hl_2 < 1.f || hl_3 < 1.f)
    {
      if (hl_2 < 1.f) 
        {
          hl_2 = 1.f;
          hl_3 = hl_2_3 - 1;
          hl_3 = (int) hl_3;
        }
      else if (hl_3 < 1.f) 
        {
          hl_3 = 1.f;
          hl_2 = hl_2_3 - 1;
          hl_2 = (int) hl_2;
        }
    }
  else if ((int) hl_2_3 > (int) hl_2 + (int) hl_3)
    {
      if (hl_2 - (int) hl_2 > hl_3 - (int) hl_3)
        hl_2 = (int) hl_2 + 1, hl_3 = (int) hl_3;
      else
        hl_3 = (int) hl_3 + 1, hl_2 = (int) hl_2;
    }
  hl_0 = std::min (4.f, hl_0);
  hl_1 = std::min (4.f, hl_1);
  hl_2 = std::min (4.f, hl_2);
  hl_3 = std::min (4.f, hl_3);

  return (int) hl_0 * 1000 + (int) hl_1 * 100 + (int) hl_2 * 10 + (int) hl_3;
}

void Barcode::check_control_number ()
{
  if (!is_identified) 
    return;
  int check_sum = 0;
  for (int i = 0; i < 13; i++)
    check_sum += barcode_number[i] * check_sum_coeffs[i];
  if (check_sum % 10 == 0)
    is_correct = true;
}