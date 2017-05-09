/* 
VMIDI 2
Detect piano keyboard keypresses in velocity-sensitive way.
Author: Albert Nisbet
April 2017
*/

#define NOMINMAX

/*______INCLUDES______*/

#include <string>
#include <iostream>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include <Kinect.h>
#include "Source.h"

/*______MACROS______*/

// HRCHECK - Quickly perform result checks on expressions
#define WIDEN2(x) L ## x
#define WIDEN(x) WIDEN2(x)
#define __WFILE__ WIDEN(__FILE__)
#define HRCHECK(__expr) {hr=(__expr);if(FAILED(hr)){wprintf(L"FAILURE 0x%08X (%i)\n\tline: %u file: '%s'\n\texpr: '" WIDEN(#__expr) L"'\n",hr, hr, __LINE__,__WFILE__);goto cleanup;}}

// HRCONTINUE - Performs result checks and breaks if error encountered
#define HRCONTINUE(__expr) {hr=(__expr);if(FAILED(hr)){continue;}}

// MATDUMP - Spit out a cv2 matrix, MATLAB-style
#define MATDUMP(__mat) {std::cout << #__mat << " =" << endl << __mat << endl << endl;}

// RELEASE - Safely release
#define RELEASE(__p) {if(__p!=nullptr){__p->Release();__p=nullptr;}}

/*______TYPEDEFS______*/

// Explicit type sizes to avoid confusion
typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;
typedef unsigned long long uint64;
typedef char int8;
typedef short int16;
typedef int int32;
typedef long long int64;

/*______KEYS______*/

// Basic struct to represent a piano key 
struct PianoKey
{
	int note;
	int octave;
	float depth;
	float prevDepth[5];
	bool isBlack;
	bool isSounding;
	int xRange[2];
	float avgDepth;
	float avgDepth_prev;
	float soundVol;
	int keyHeight;
};

/*______MAIN______*/

using namespace std;
using namespace cv;
 
int main()
{
	// Initialise general variables
	HRESULT hr = S_OK;
	int frame_index = 0;
	bool snatchIr = false;

	// Initialise Kinect variables
	IKinectSensor* a_sensor = nullptr;
	IDepthFrameSource* depthFrameSource = nullptr;
	IDepthFrameReader* depthFrameReader = nullptr;
	IDepthFrame* depthFrame = nullptr;
	IInfraredFrameSource* irFrameSource = nullptr;
	IInfraredFrameReader* irFrameReader = nullptr;
	IInfraredFrame* irFrame = nullptr;
	uint16* depthBuffer = nullptr;
	uint16* irBuffer = nullptr;
	uint16* warpBuffer = nullptr;

	// Raw frame dimensions
	int rawWidth = 512;
	int rawHeight = 424;
	depthBuffer = new uint16[rawWidth * rawHeight];
	irBuffer = new uint16[rawWidth * rawHeight];

	// Initialise OpenCV variables
	Mat cvFrame = Mat(424, 512, CV_16U);
	Mat rawFrame;
	Mat convFrame;
	Mat filterFrame;
	Mat threshMaxFrame, threshMinFrame;
	Mat warpFrame;
	Mat warpFilterFrame;
	Mat normFrame;
	Mat colNormFrame;

	Mat normFrame_first, normFrame_diff, colNormFrame_diff;
	Mat warpFrame_first, warpFrame_diff;
	Mat handFrame, handFrame_blur;

	Mat rawIrFrame;
	Mat warpIrFrame;

	// Correlation kernel applied to warped iamge
	Mat vertKernel = (Mat_<double>(20, 1) << 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 );
	vertKernel = vertKernel / sum(vertKernel).val[0];
	MATDUMP(vertKernel);

	// Structuring elements for opening and dilation of hand image
	int open_size = 5;
	Mat open_element = getStructuringElement(MORPH_ELLIPSE, Size(2 * open_size + 1, 2 * open_size + 1), Point(open_size, open_size));

	int dilate_size = 15;
	Mat dilate_element = getStructuringElement(MORPH_ELLIPSE, Size(2 * dilate_size + 1, 2 * dilate_size + 1), Point(dilate_size, dilate_size));

	// Warped frame dimensions
	int warpWidth = 505;
	int warpHeight = 120;

	// Manual keyboard warping
	Point2f inQ[4];
	Point2f outQ[4];

	// e.g. Piano at flat
	// Be sure to CORRECT THE KINECT'S FLIPPED IMAGE
	inQ[1] = Point2f(80, 122);
	inQ[0] = Point2f(412, 124);
	inQ[3] = Point2f(71, 198);
	inQ[2] = Point2f(418, 203);

	outQ[0] = Point2f(0, 0);
	outQ[1] = Point2f(warpWidth, 0);
	outQ[2] = Point2f(0, warpHeight);
	outQ[3] = Point2f(warpWidth, warpHeight);

	Mat lambda = getPerspectiveTransform(inQ, outQ);
	MATDUMP(lambda);

	// Piano variables
	int maxDistance = 800;
	int minDistance = 600;

	// Normalisation parameters
	FLOAT normScale = (FLOAT)(maxDistance - minDistance);
	FLOAT normOffset = (FLOAT)minDistance / normScale;

	// Keys
	
	int keyHeightWhite = 85; 
	int keyHeightBlack = 60; 
	const int numKeys = 43;  
	int octaveOffset = 3;
	int widthWhite = 13; 
	int widthBlack = 9;
	int keyOffset = 10; 

	PianoKey keys[numKeys];
	bool isBlackKey[12] = { 0,1,0,1,0,0,1,0,1,0,1,0 }; 
	string keyNames[12] = { "C", "Cs", "D", "Ds", "E", "F", "Fs", "G", "Gs", "A", "As", "B" };
	int pos = warpWidth - 7;
	for (int key = numKeys - 1; key >= 0; key--) {
		keys[key].xRange[0] = pos;
		pos -= isBlackKey[(key + keyOffset) % 12] == 1 ? widthBlack : widthWhite;
		keys[key].isBlack = isBlackKey[(key + keyOffset) % 12];
		keys[key].note = (key + keyOffset) % 12;
		keys[key].octave = octaveOffset + key / 12;
		keys[key].xRange[1] = pos;
		keys[key].depth = 0;
		keys[key].prevDepth[0] = 0;
		keys[key].prevDepth[1] = 0;
		keys[key].prevDepth[2] = 0;
		keys[key].prevDepth[3] = 0;
		keys[key].prevDepth[4] = 0;
		keys[key].avgDepth = 0;
		keys[key].avgDepth_prev = 0;
		keys[key].soundVol = 0;
		keys[key].isSounding = false;
		keys[key].keyHeight = isBlackKey[(key + keyOffset) % 12] == 1 ? keyHeightBlack : keyHeightWhite;
	}

	float triggerDist = 1.5;
	float soundDist = 4.5;
	float muteDist = 2.5;

	// Connect to sensor
	HRCHECK(GetDefaultKinectSensor(&a_sensor));
	a_sensor->Open();

	// Get depth reader
	HRCHECK(a_sensor->get_DepthFrameSource(&depthFrameSource));
	HRCHECK(depthFrameSource->OpenReader(&depthFrameReader));
	RELEASE(depthFrameSource); // Source no longer needed

	// Get IR reader
	HRCHECK(a_sensor->get_InfraredFrameSource(&irFrameSource));
	HRCHECK(irFrameSource->OpenReader(&irFrameReader));
	RELEASE(irFrameSource); // Source no longer needed
	
	do {
		if (snatchIr) {
			HRCONTINUE(irFrameReader->AcquireLatestFrame(&irFrame));
			irFrame->CopyFrameDataToArray(rawWidth * rawHeight, irBuffer);
			RELEASE(irFrame);
			rawIrFrame = Mat(rawHeight, rawWidth, CV_16UC1, irBuffer);
			rawIrFrame.convertTo(rawIrFrame, CV_8UC1, 0.02);
			warpIrFrame = rawIrFrame.clone();
			warpPerspective(warpIrFrame, warpIrFrame, lambda, Size(warpWidth, warpHeight));
			imshow("irFrame", warpIrFrame);
			imwrite(format("Capture/warpIrFrame/warpIrFrame%04d.jpg", frame_index), warpIrFrame);
			frame_index++;
			continue;
		}

		// Fetch latest Kinect Frame
		HRCONTINUE(depthFrameReader->AcquireLatestFrame(&depthFrame));
		// Proceed iff there is a new frame ready
		
		// Create array from depth frame
		depthFrame->CopyFrameDataToArray(rawWidth * rawHeight, depthBuffer);
		RELEASE(depthFrame);
		// We are finished with Kinect variables for this frame

		// Read into cv array
		rawFrame = Mat(rawHeight, rawWidth, CV_16UC1, depthBuffer);
		//printf("cvFrame: x=%i, y=%i, d=%i\n", rawWidth/2, rawHeight/2, (int)(rawFrame.at<uint16>(rawWidth/2, rawHeight/2)));
		
		// Convert to 32F for ease of use
		rawFrame.convertTo(convFrame, CV_32FC1);
		//printf("rawFrame: x=%i, y=%i, d=%i\n", rawWidth / 2, rawHeight / 2, (int)(rawFrame.at<float>(rawWidth / 2, rawHeight / 2)));

		// Get filtered image
		bilateralFilter(convFrame, filterFrame, 3, 5, 5);

		// Get max thresholded
		threshold(filterFrame, threshMaxFrame, maxDistance, maxDistance, THRESH_TOZERO_INV);
		//printf("threshMaxFrame: x=%i, y=%i, d=%i\n", rawWidth / 2, rawHeight / 2, (int)(threshMaxFrame.at<float>(rawWidth / 2, rawHeight / 2)));

		// Get min thresholded
		threshold(threshMaxFrame, threshMinFrame, minDistance, minDistance, THRESH_TOZERO);
		//printf("threshMinFrame: x=%i, y=%i, d=%i\n", rawWidth / 2, rawHeight / 2, (int)(threshMinFrame.at<float>(rawWidth / 2, rawHeight / 2)));

		// Get warped
		warpFrame = threshMinFrame.clone();
		warpPerspective(threshMinFrame, warpFrame, lambda, Size(warpWidth, warpHeight));
		double min, max;
		Point minL, maxL;
		minMaxLoc(warpFrame, &min, &max, &minL, &maxL);
		//printf("warpFrame (%s): x=%i, y=%i, d=%i\n", type2str(warpFrame.type()), 250, 100, (int)(warpFrame.at<float>(250, 100)));
		//printf("Min: %.2f at (%i,%i), Max: %.2f at (%i,%i)\n", min, minL.x, minL.y, max, maxL.x, maxL.y);

		// Get vertical box filtered
		filter2D(warpFrame, warpFilterFrame, CV_32F, vertKernel);
		
		// Get normalised (display purposes) - done manually
		normFrame = warpFilterFrame / normScale - normOffset;
		//printf("normFrame: x=%i, y=%i, d=%i\n", warpWidth/2, warpHeight/2, (int)(normFrame.at<uint8>(warpHeight / 2, warpWidth / 2)));

		// Show keys
		cvtColor(normFrame, colNormFrame, COLOR_GRAY2BGR);

		// Process difference images
		if (frame_index <= 0) {
			// Register first images
			normFrame_first = normFrame.clone();
			warpFrame_first = warpFrame.clone();
		}
		else {
			// Norm for display purposes
			normFrame_diff = cv::abs(normFrame - normFrame_first);
			cvtColor(normFrame_diff*12, colNormFrame_diff, COLOR_GRAY2BGR);

			// Calculate difference
			warpFrame_diff = warpFrame - warpFrame_first;

			// Hand area frame
			threshold(-warpFrame_diff, handFrame, 6.0, 1.0, THRESH_BINARY);
			medianBlur(handFrame, handFrame_blur, 5);
			morphologyEx(handFrame_blur, handFrame_blur, MORPH_OPEN, open_element);
			dilate(handFrame_blur, handFrame_blur, dilate_element);
			handFrame_blur.convertTo(handFrame_blur, CV_8UC1, 255);
		}

		// Key depth detection
		for (int key = 0; key < numKeys; key++) {

			// Get key rectangle
			Point pt1 = Point(keys[key].xRange[0], 0);
			Point pt2 = Point(keys[key].xRange[1], keys[key].keyHeight);
			rectangle(colNormFrame, pt1, pt2, Scalar(0.8, 0.7, 0.7));

			if (frame_index > 0) {

				// Create key mask
				Mat mask = Mat::zeros(warpFrame.rows, warpFrame.cols, CV_8UC1);
				rectangle(mask, pt1, pt2, Scalar(255), CV_FILLED);

				// Incorporate hand frame
				Mat combiMask = mask - handFrame_blur;
				Mat intersectMask;
				bitwise_and(mask, handFrame_blur, intersectMask);

				// Display for test key
				if (key == 11) 
				{ 
					imshow("intersect", intersectMask);
					imshow("masked", combiMask); 
				}

				// Extract the key's average depth
				Scalar average = mean(warpFrame_diff, combiMask);
				keys[key].depth = average.val[0];
				
				// Mask cleanup
				mask.release();
				combiMask.release();
				intersectMask.release();

				// Store rolling average
				keys[key].prevDepth[frame_index % 5] = keys[key].depth;
				keys[key].avgDepth_prev = keys[key].avgDepth;
				keys[key].avgDepth = (keys[key].prevDepth[0] +
					keys[key].prevDepth[1] +
					keys[key].prevDepth[2] +
					keys[key].prevDepth[3] +
					keys[key].prevDepth[4]) / 5.0f;
				
				if (key == 11) { printf("%i,%.2f", frame_index, keys[key].avgDepth); }

				// Highlight test key
				Scalar color = key == 11 ? Scalar(0,0,1) : Scalar(0.7, 0.4, 0.4);
				rectangle(colNormFrame_diff, pt1, pt2, color);

				// Reset/Don't calculate a volume if not pressed beyond mute point
				if (keys[key].avgDepth < muteDist) {
					keys[key].soundVol = 0;
				}

				// Calculate a volume iff newly pressed beyond point of sound
				if (keys[key].avgDepth > soundDist && keys[key].soundVol == 0) {
					keys[key].soundVol = (keys[key].avgDepth - keys[key].avgDepth_prev) * 30;
					printf(",%s%i,%.2f", keyNames[keys[key].note], keys[key].octave, keys[key].soundVol);
				}

				if (keys[key].soundVol > 0) {
					String formatted = format("%s%i,%.2f", keyNames[keys[key].note], keys[key].octave, keys[key].soundVol);
					putText(colNormFrame_diff, formatted, Point(160, 100), FONT_HERSHEY_PLAIN, 1, Scalar(0.2, 1, 0.3));
				}
			}
		}

		/*______VISUAL OUTPUT______*/

		putText(colNormFrame_diff, to_string(keys[11].depth), Point(40, 100), FONT_HERSHEY_PLAIN, 1, Scalar(0, 0, 1));
		putText(colNormFrame_diff, to_string(keys[11].avgDepth), Point(40, 115), FONT_HERSHEY_PLAIN, 1, Scalar(1, 0.5, 0.2));

		imshow("rawFrame", rawFrame);

		imshow("rawFrame", convFrame);

		imshow("filterFrame", filterFrame);

		imshow("threshMaxFrame", threshMaxFrame);
		imshow("threshMinFrame", threshMinFrame);

		namedWindow("warpFrame");
		setMouseCallback("warpFrame", onMouse);
		imshow("warpFrame", warpFrame);

		namedWindow("normFrame");
		setMouseCallback("normFrame", onMouse);
		imshow("normFrame", 5*(normFrame-Scalar(0.6))); // Normalisation parameters

		namedWindow("colNormFrame");
		setMouseCallback("colNormFrame", onMouse);
		imshow("colNormFrame", colNormFrame);

		imshow("normFrame_first", normFrame_first);

		if (frame_index > 0) {
			imshow("normFrame_diff", colNormFrame_diff);
			imshow("handFrame", handFrame);
			imshow("handFrame_blur", handFrame_blur);
		}

		if (frame_index >= 1) {
			Mat saveFrame;
			normFrame.convertTo(saveFrame, CV_8UC1, 255.0);
			imwrite(format("Capture/normFrameReg/normFrameReg%04d.png", frame_index), saveFrame);
		}

		// Iterate
		frame_index++;
		printf("\n");

	} while (waitKey(10) > 0);
	

cleanup:

	// Release Kinect stuff safely
	RELEASE(a_sensor);
	RELEASE(depthFrameReader);
	RELEASE(depthFrameSource);
	RELEASE(depthFrame);
	RELEASE(irFrameReader);
	RELEASE(irFrameSource);
	RELEASE(irFrame);

	return 0;

}


/* onMouse
	Prints x, y position to cout on frame click.
	Used as mouse callback for shown windows where desired.
	Useful for getting positions.
	USAGE:
		setMouseCallback("imshowWindowTitleHere", onMouse);
*/
void onMouse(int event, int x, int y, int, void*)
{
	if (event != CV_EVENT_LBUTTONDOWN)
		return;
	Point pt = Point(x, y);
	printf("x=%i, y=%i\n", pt.x, pt.y);
}

/* type2str
	Converts the type of matrix to a string.
	Good for debugging.
	USAGE:
		string typestr = type2str( M.type() );
*/
std::string type2str(int type) {
	std::string r;

	uchar depth = type & CV_MAT_DEPTH_MASK;
	uchar chans = 1 + (type >> CV_CN_SHIFT);

	switch (depth) {
	case CV_8U:  r = "8U"; break;
	case CV_8S:  r = "8S"; break;
	case CV_16U: r = "16U"; break;
	case CV_16S: r = "16S"; break;
	case CV_32S: r = "32S"; break;
	case CV_32F: r = "32F"; break;
	case CV_64F: r = "64F"; break;
	default:     r = "User"; break;
	}

	r += "C";
	r += (chans + '0');

	return r;
}