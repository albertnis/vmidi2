#pragma once

#include <string>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

// This is start of the header guard.  ADD_H can be any unique name.  By convention, we use the name of the header file.
#ifndef SOURCE_H
#define SOURCE_H

extern cv::Mat cvFrame;

// This is the content of the .h file, which is where the declarations go
void onMouse(int event, int x, int y, int, void*);

std::string type2str(int type);

					   // This is the end of the header guard
#endif