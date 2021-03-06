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
#ifndef S2ENGINETHREAD_H
#define S2ENGINETHREAD_H

#include <string>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <cstdlib>
#include <cmath>
#include <QtGui >
#include <QImage > 
#include <QThread >
#include <QMutex >
#include "opencv2/core.hpp"
#include "opencv2/core/utility.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/imgcodecs.hpp"

#include "uevastructures.h"
#include "uevafunctions.h"

class S2EngineThread : public QThread
{
	Q_OBJECT

public:
	S2EngineThread(QObject *parent = 0);
	~S2EngineThread();

	//// THREAD OPERATIONS
	void setSettings(const UevaSettings &s);
	void setData(const UevaData &d);
	void wake();

	//// SINGLE TIME FUNCTION
	void setCalib(double micronLength);
	void setBkgd();
	void separateChannels(int &numChan);
	void sortChannels(std::map<std::string, std::vector<int> > &channelInfo);
	void loadCtrl(std::string fileName,
		int *numState, int *numIn, int *numOut, int *numCtrl, double *ctrlTs);
	void initImgproc();
	void finalizeImgproc();
	void initCtrl();
	void finalizeCtrl(QVector<qreal> &inletRegurgitate);

signals:
	void engineSignal(const UevaData &d);

protected:
	//// CONTINUOUS FUNCTION
	void run();

private:
	//// THREAD VARIABLES
	bool idle;
	QMutex mutex;
	UevaSettings settings;
	UevaData data;

	//// MULTI CYCLE VARIABLES
	double micronPerPixel;
	cv::Mat bkgd;
	std::vector<UevaCtrl> ctrls;
	cv::Mat dropletMask;
	cv::Mat markerMask;
	cv::Mat allChannels;
	std::vector<std::vector<cv::Point_<int>>> channelContours;
	std::vector<UevaChannel> channels;
	bool needSelecting;
	bool needReleasing;

	//// DOUBLE CYCLE VARIABLES
	std::vector<UevaMarker> oldMarkers;
	std::vector<UevaMarker> newMarkers;
	std::vector<int> activatedChannelIndices;

	QVector<qreal> ground;
	QVector<qreal> correction;
	cv::Mat posteriorErrorCov;
	cv::Mat modelNoiseCov;
	cv::Mat disturbanceNoiseCov;
	cv::Mat processNoiseCov;
	cv::Mat sensorNoiseCov;
	QVector<qreal> reference;
	QVector<qreal> output;
	QVector<qreal> outputLuenburger;
	QVector<qreal> outputRaw;
	QVector<qreal> outputOffset;
	QVector<qreal> outputKalman;
	QVector<qreal> stateKalman;
	QVector<qreal> disturbance;
	QVector<qreal> stateLuenburger;
	QVector<qreal> stateIntegral;
	QVector<qreal> command;

	//// SINGLE CYCLE VARIABLES
	cv::Mat allDroplets; 
	std::vector<std::vector< cv::Point_<int> >> dropletContours;
	cv::Mat allMarkers;
	std::vector<std::vector< cv::Point_<int> >> markerContours;
	
	std::vector<int> desiredChannelIndices;
	std::vector<UevaDroplet> droplets;

	cv::Point_<int> mousePressLeft;
	cv::Point_<int> mousePressRight;
	cv::Point_<int> mousePressPrevious;
	cv::Point_<int> mousePressCurrent;
	cv::Point_<int> mousePressDisplacement;

	int directRequestIndex;

	cv::Mat k;
	cv::Mat pp;
	cv::Mat pe;
	cv::Mat rw;
	cv::Mat rv;
	cv::Mat r;
	cv::Mat dr;
	cv::Mat y;
	cv::Mat yl;
	cv::Mat y_raw;
	cv::Mat y_off;
	cv::Mat yk;
	cv::Mat xp;
	cv::Mat xe;
	cv::Mat xl;
	cv::Mat z;
	cv::Mat u;

	//// CONVENIENCE VARIABLES
	enum EngineConstants
	{
		LOW_VALUE = 0,
		MID_VALUE = 127,
		HIGH_VALUE = 255,
	};
	cv::Mat structuringElement;
	cv::Point_<int> seed;
	int floodFillReturn;
	cv::Scalar_<int> lineColor;
	int lineThickness;
	int lineType;
	double fontScale;
	cv::Point_<int> anchor;
	cv::Moments mom;
	cv::Rect rect;
	std::string str;
	bool alwaysTrue;

	private slots:
};



#endif
