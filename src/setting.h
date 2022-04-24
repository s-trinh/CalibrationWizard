//
//  setting.h
//
//  Copyright © 2019 Songyou Peng. All rights reserved.
//

#ifndef setting_h
#define setting_h

#include <iostream>
#include <sstream>
#include <fstream>
#include <time.h>
#include <stdio.h>

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/calib3d.hpp>
#include <opencv2/highgui.hpp>

#ifndef _CRT_SECURE_NO_WARNINGS
# define _CRT_SECURE_NO_WARNINGS
#endif

using namespace cv;
using namespace std;

class Settings
{
public:
    Settings() : goodInput(false) {}
    enum Pattern { NOT_EXISTING, CHESSBOARD, CIRCLES_GRID, ASYMMETRIC_CIRCLES_GRID };
    enum InputType {INVALID, CAMERA, VIDEO_FILE, IMAGE_LIST};

    void write(FileStorage& fs) const                        //Write serialization for this class
    {
        fs << "{" << "BoardSize_Width"  << boardSize.width
        << "BoardSize_Height" << boardSize.height
        << "Square_Size"<< squareSize
        << "Calibrate_Pattern" << patternToUse
        << "Calibrate_NrOfFrameToUse" << nrFrames
        << "Calibrate_FixAspectRatio" << aspectRatio
        << "Calibrate_AssumeZeroTangentialDistortion" << calibZeroTangentDist
        << "Calibrate_FixPrincipalPointAtTheCenter" << calibFixPrincipalPoint

        << "Calibrate_AssumeZerok1Distortion" << calibZerok1Dist
        << "Calibrate_AssumeZerok2Distortion" << calibZerok2Dist

        << "Write_DetectedFeaturePoints" << bwritePoints
        << "Write_extrinsicParameters"   << bwriteExtrinsics
//        << "Write_outputFileName"  << outputFileName

        << "Show_UndistortedImage" << showUndistorsed

        << "Input_FlipAroundHorizontalAxis" << flipVertical
        << "Input_Delay" << delay
//        << "Input" << input
        << "}";
    }

    void read(const FileNode& node)                          //Read serialization for this class
    {
        node["BoardSize_Width" ] >> boardSize.width;
        node["BoardSize_Height"] >> boardSize.height;
        node["Calibrate_Pattern"] >> patternToUse;
        node["Square_Size"]  >> squareSize;
        node["Calibrate_NrOfFrameToUse"] >> nrFrames;
        node["Calibrate_FixAspectRatio"] >> aspectRatio;
        node["Write_DetectedFeaturePoints"] >> bwritePoints;
        node["Write_extrinsicParameters"] >> bwriteExtrinsics;
//        node["Write_outputFileName"] >> outputFileName;
        node["Calibrate_AssumeZeroTangentialDistortion"] >> calibZeroTangentDist;
        node["Calibrate_FixPrincipalPointAtTheCenter"] >> calibFixPrincipalPoint;
//        node["Is_FishEye"] >> isFishEye;
        node["Calibrate_AssumeZerok1Distortion"] >> calibZerok1Dist;
        node["Calibrate_AssumeZerok2Distortion"] >> calibZerok2Dist;
        node["Input_FlipAroundHorizontalAxis"] >> flipVertical;
        node["Show_UndistortedImage"] >> showUndistorsed;
//        node["Input"] >> input;
        node["Input_Delay"] >> delay;
        interprate();
    }

    void interprate()
    {
        goodInput = true;
        if (boardSize.width <= 0 || boardSize.height <= 0)
        {
            cerr << "Invalid Board size: " << boardSize.width << " " << boardSize.height << endl;
            goodInput = false;
        }
        if (squareSize <= 10e-6)
        {
            cerr << "Invalid square size " << squareSize << endl;
            goodInput = false;
        }
        if (nrFrames <= 0)
        {
            cerr << "Invalid number of frames " << nrFrames << endl;
            goodInput = false;
        }

        if (input.empty())
        {
            // Check for valid input
            inputType = INVALID;
        }
        else
        {
            if (input[0] >= '0' && input[0] <= '9')
            {
                stringstream ss(input);
                ss >> cameraID;
                inputType = CAMERA;
            }
            else
            {
                if (readStringList(input, imageList))
                {
                    inputType = IMAGE_LIST;
                    nrFrames = (nrFrames < (int)imageList.size()) ? nrFrames : (int)imageList.size();
                }
                else
                {
                    inputType = VIDEO_FILE;
                }
            }
            if (inputType == CAMERA)
            {
                inputCapture.open(cameraID);
            }
            if (inputType == VIDEO_FILE)
            {
                inputCapture.open(input);
            }
            if (inputType != IMAGE_LIST && !inputCapture.isOpened())
            {
                inputType = INVALID;
            }
        }
        if (inputType == INVALID)
        {
            cerr << " Inexistent input: " << input;
            goodInput = false;
        }

        flag = 0;
        if(calibFixPrincipalPoint) flag |= cv::CALIB_FIX_PRINCIPAL_POINT;
        if(calibZeroTangentDist)   flag |= cv::CALIB_ZERO_TANGENT_DIST;
        if(aspectRatio)            flag |= cv::CALIB_FIX_ASPECT_RATIO;
        if(calibZerok1Dist)        flag |= cv::CALIB_FIX_K1;
        if(calibZerok2Dist)        flag |= cv::CALIB_FIX_K2;

        calibrationPattern = NOT_EXISTING;
        if (!patternToUse.compare("CHESSBOARD")) calibrationPattern = CHESSBOARD;
        if (!patternToUse.compare("CIRCLES_GRID")) calibrationPattern = CIRCLES_GRID;
        if (!patternToUse.compare("ASYMMETRIC_CIRCLES_GRID")) calibrationPattern = ASYMMETRIC_CIRCLES_GRID;
        if (calibrationPattern == NOT_EXISTING)
        {
            cerr << " Inexistent camera calibration mode: " << patternToUse << endl;
            goodInput = false;
        }
        atImageList = 0;

    }

    Mat nextImage()
    {
        Mat result;
        if( inputCapture.isOpened() )
        {
            Mat view0;
            inputCapture >> view0;
            view0.copyTo(result);
        }
        else if( atImageList < (int)imageList.size() )
        {
            result = imread(imageList[atImageList++], cv::IMREAD_COLOR);
        }

        return result;
    }

    static bool readStringList( const string& filename, vector<string>& l )
    {
        l.clear();
        FileStorage fs(filename, FileStorage::READ);
        if( !fs.isOpened() )
        {
            return false;
        }
        FileNode n = fs.getFirstTopLevelNode();
        if( n.type() != FileNode::SEQ )
        {
            return false;
        }
        FileNodeIterator it = n.begin(), it_end = n.end();
        for( ; it != it_end; ++it )
        {
            l.push_back((string)*it);
        }
        return true;
    }

public:
    Size boardSize;                 // The size of the board -> Number of items by width and height
    Pattern calibrationPattern;     // One of the Chessboard, circles, or asymmetric circle pattern
    float squareSize;               // The size of a square in your defined unit (point, millimeter,etc).
    int nrFrames;                   // The number of frames to use from the input for calibration
    float aspectRatio;              // The aspect ratio
    int delay;                      // In case of a video input
    bool bwritePoints;              //  Write detected feature points
    bool bwriteExtrinsics;          // Write extrinsic parameters
    bool calibZeroTangentDist;      // Assume zero tangential distortion
    bool calibFixPrincipalPoint;    // Fix the principal point at the center
    bool flipVertical;              // Flip the captured images around the horizontal axis
    string outputFileName;          // The name of the file where to write
    bool showUndistorsed;           // Show undistorted images after calibration
    string input;                   // The input ->
    bool calibZerok1Dist;
    bool calibZerok2Dist;
//    bool isFishEye;

    int cameraID;
    vector<string> imageList;
    int atImageList;
    VideoCapture inputCapture;
    InputType inputType;
    bool goodInput;
    int flag;

private:
    string patternToUse;
};

#endif /* setting_h */
