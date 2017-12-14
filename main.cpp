#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/calib3d/calib3d.hpp"
#include "opencv2/highgui/highgui.hpp"
#include <iostream>
#include <algorithm>
#include <vector>
#include "barcode.h"

using namespace cv;

// How to compile:
// g++ main.cpp barcode.cpp -O3 -std=c++14 -o barcode_detector `pkg-config --cflags --libs opencv`
// QT: LIBS += `pkg-config --cflags --libs opencv`, QMAKE_CXXFLAGS+=-std=c++14

int find_and_crop_barcode (const Mat &orig_im, Mat &selection_im, Mat &barcode_im);

int main (int argc, char **argv)
{
  if (argc < 2)
  {
    std::cout << "Need input image file name!" << std::endl;
    return -1;
  }
  // 1: Load original image
  namedWindow ("Original image", WINDOW_NORMAL);
  
  Mat orig_im = imread (argv[1]);
  imshow ("Original image", orig_im);
  waitKey (0);

  // 2: Find and crop barcode
  Mat selection_im, barcode_im;
  if (find_and_crop_barcode (orig_im, selection_im, barcode_im) < 0)
    {
      std::cout << "Cannot find barcode location." << std::endl;
      return -1;
    }

  namedWindow ("Barcode detection", WINDOW_NORMAL);
  imshow ("Barcode detection", selection_im);
  waitKey (0);
  imshow ("Barcode detection", barcode_im);
  waitKey (0);

  Mat barcode_thresh_im = barcode_im.clone ();
  cvtColor (barcode_thresh_im, barcode_thresh_im, CV_BGR2GRAY);
  threshold (barcode_thresh_im, barcode_thresh_im, 25 /*default_threshold_value*/, 
             255 /*max_BINARY_value*/, CV_THRESH_BINARY | CV_THRESH_OTSU);  

  // 3: Read barcode
  Barcode barcode (&barcode_thresh_im);
  if (barcode.is_decoded ())
    {
      std::cout << "Barcode is detected: ";
      barcode.print_barcode ();
    }
  else
    std::cout << "Cannot read barcode." << std::endl;
  
  imshow ("Barcode detection", barcode_thresh_im);
  waitKey (0);
  return 0;
}

int find_and_crop_barcode (const Mat &orig_im, Mat &selection_im, Mat &barcode_im)
{
  // 1: Convert to grayscale image
  Mat gray_im;
  cvtColor (orig_im, gray_im, CV_BGR2GRAY);

  // 2: compute the Scharr gradient magnitude representation of the images 
  // in both the x and y direction
  Mat grad_x_im, grad_y_im, gradient_im;
  Sobel (gray_im, grad_x_im, CV_32F /*depth*/, 1 /*order of the derivative x*/, 
         0 /*order of the derivative y*/, CV_SCHARR);
  Sobel (gray_im, grad_y_im, CV_32F /*depth*/, 0 /*order of the derivative x*/, 
         1 /*order of the derivative y*/, CV_SCHARR);

  // subtract the y-gradient of the Scharr operator from the x-gradient of
  // the Scharr operator. By performing this subtraction we are left with
  // regions of the image that have high horizontal gradients and low vertical gradients.
  subtract (grad_x_im, grad_y_im, gradient_im);
  convertScaleAbs (gradient_im, gradient_im);

  // 3: Filter out the noise using threshold
  Mat thresh_im;
  blur (gradient_im, thresh_im, Size (9, 9));
  threshold (thresh_im, thresh_im, 225 /*default_threshold_value*/, 
             255 /*max_BINARY_value*/, CV_THRESH_BINARY);

  // Applying closing morphological operations to close the gap between barcode stripes.
  Mat morph_kernel_im = getStructuringElement (MORPH_RECT, Size (21, 7) /* Size of the structuring element */);
  morphologyEx (thresh_im, thresh_im, MORPH_CLOSE /*closing operation*/, morph_kernel_im);

  // Remove small blobs that are not an actual part of barcode
  // Eroding and Dilating FAQ: https://docs.opencv.org/2.4/doc/tutorials/imgproc/erosion_dilatation/erosion_dilatation.html
  // Dilation operator causes bright regions within an image to “grow”
  erode (thresh_im, thresh_im, morph_kernel_im, Point (-1,-1), 2 /*iterations*/);
  dilate (thresh_im, thresh_im, morph_kernel_im, Point (-1,-1), 2 /*iterations*/);    

  // 4: find max area contour and min are rectangular with which contains this contour
  std::vector<std::vector<Point> > contours;
  std::vector<Vec4i> hierarchy;
  findContours (thresh_im, contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE);
  if (contours.empty ())
    return -1;
  std::sort(contours.begin(), contours.end(), 
    [](const std::vector<Point> &a, const std::vector<Point> &b) -> bool
  { 
    return contourArea (a) > contourArea (b); 
  });

  selection_im = orig_im.clone ();  
  RotatedRect min_rot_rect = minAreaRect (contours[0]);
  Point2f vertices[4];
  min_rot_rect.points (vertices);
  for (int i = 0; i < 4; i++)
    {
      line (selection_im, vertices[i], vertices[(i+1)%4], Scalar (0, 255, 0), 2);
      // std::cout << vertices[i] << std::endl;
    }    

  /*
  std::vector<Point2f> pts_src;
  std::vector<Point2f> pts_dst;
  for (unsigned int i = 0; i < 4; i++)
    pts_src.push_back (vertices[i]);
  pts_dst.push_back(Point2f(130.312, 437.707));
  pts_dst.push_back(Point2f(128.589, 224.003));
  pts_dst.push_back(Point2f(551.658, 220.591));
  pts_dst.push_back(Point2f(553.381, 434.295));

  // Calculate Homography
  Mat h = findHomography(pts_src, pts_dst);
 
  // Output image
  Mat im_out = orig_im.clone ();
  // Warp source image to destination based on homography
  warpPerspective (im_out, im_out, h, area_selected_im.size());  
  imshow ("My Window", im_out);
  waitKey (0);
  */
  
  // get angle and size from the bounding box
  float rect_angle = min_rot_rect.angle;
  Size rect_size = min_rot_rect.size;
  if (rect_angle < -45.)
    {
      rect_angle += 90.0;
      swap (rect_size.width, rect_size.height);
    }
  // get the rotation matrix
  Mat rotation_mat = getRotationMatrix2D (min_rot_rect.center, rect_angle, 1.0);
  // perform the affine transformation
  Mat rotated_im = orig_im.clone ();
  warpAffine (rotated_im, rotated_im, rotation_mat, orig_im.size(), INTER_CUBIC);
  Mat cropped_im;
  getRectSubPix (rotated_im, rect_size + Size (10, 10), min_rot_rect.center, cropped_im);
  barcode_im = cropped_im;

  return 0;
}