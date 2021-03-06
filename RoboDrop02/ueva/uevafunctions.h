/*
Copyright 2016 David Wong

This file is part of RoboDrop from the uEVA project. https://github.com/DaveSketchySpeedway/uEVA

RoboDrop is free software : you can redistribute it and / or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
any later version.

RoboDrop is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with RoboDrop. If not, see <http://www.gnu.org/licenses/>
*/


#ifndef UEVAFUNCTIONS_H
#define UEVAFUNCTIONS_H

#include "opencv2/core.hpp"
#include "opencv2/core/utility.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/imgcodecs.hpp"
#include <QtGui >
#include <iostream>
#include <fstream>
#include <algorithm>
#include "persistence1d.hpp"

#include "uevastructures.h"

namespace Ueva
{
	cv::Mat qImage2cvMat(const QImage &qImage);

	QImage cvMat2qImage(const cv::Mat &cvMat);

	cv::Mat contour2Mask(const std::vector<cv::Point_<int>> &contour, const cv::Size_<int> &sz);

	std::vector<cv::Point_<int>> mask2Contour(const cv::Mat &mask);

	void bigPassFilter(std::vector<std::vector< cv::Point_<int> >> &contours, const int size);

	void trackMarkerIdentities(std::vector<UevaMarker> &newMarkers, std::vector<UevaMarker> &oldMarkers, int trackTooFar);

	int detectKink(std::vector< cv::Point_<int>> &contour, const int convexSize);

	int detectNeck(std::vector< cv::Point_<int>> &contour, int &kinkIndex, float &neck, const int threshold);

	int masksOverlap(cv::Mat &mask1, cv::Mat &mask2);

	bool isMarkerInChannel(UevaMarker &marker, UevaChannel &channel, int xMargin, int yMargin);

	bool isCombinationPossible(std::vector<int> &combination, std::vector<UevaCtrl> &ctrls);

	void deleteFromCombination(std::vector<int> &combination, const int value);

	double screen2ctrl(const cv::Point_<int> &point, const int &direction, const double &multiplier);

	double neck2ctrl(const float &neckPix, const double &umPerPix, double desire, double thresh, double lowGain, double highGain);
}
#endif