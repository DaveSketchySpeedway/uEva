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

#include "s2enginethread.h"

S2EngineThread::S2EngineThread(QObject *parent)
	: QThread(parent)
{
	mutex.lock();
	idle = true;
	mutex.unlock();
}

S2EngineThread::~S2EngineThread()
{
	mutex.lock();

	mutex.unlock();
}




//// THREAD OPERATIONS

void S2EngineThread::setSettings(const UevaSettings &s)
{
	mutex.lock();
	settings = s;
	mutex.unlock();
}

void S2EngineThread::setData(const UevaData &d)
{
	mutex.lock();
	data = d;
	mutex.unlock();
}

void S2EngineThread::wake()
{
	mutex.lock();
	idle = false;
	mutex.unlock();
}



//// SINGLE TIME
void S2EngineThread::setCalib(double micronLength)
{
	mutex.lock();

	if (!settings.mouseLines.empty())
	{
		QLine mouseLine = settings.mouseLines[0];
		double pixelLength = std::sqrt(
			std::pow((double)mouseLine.dx(), 2.0) + std::pow((double)mouseLine.dy(), 2.0));
		micronPerPixel = micronLength *settings.displayScale / pixelLength;
		qDebug() << "micronPerPixel = " << micronPerPixel << endl;
	}

	mutex.unlock();
}

void S2EngineThread::setBkgd()
{
	mutex.lock();

	bkgd = data.rawGray.clone();
	qDebug() << "New Background" << endl;

	mutex.unlock();
}

void S2EngineThread::separateChannels(int &numChan)
{
	mutex.lock();
		
	CV_Assert(!allChannels.empty());
		
	channelContours.clear();
	channels.clear();
	// find all channel contours
	cv::findContours(allChannels, channelContours, CV_RETR_LIST, CV_CHAIN_APPROX_NONE);
	numChan = channelContours.size();
	// seprate into vector of UevaChannel
	for (int i = 0; i < numChan; i++)
	{
		UevaChannel channel;
		channel.index = i;
		channel.mask = Ueva::contour2Mask(channelContours[i], allChannels.size());
		channel.rect = cv::boundingRect(channelContours[i]);
		channels.push_back(channel);
	}


	mutex.unlock();
}

void S2EngineThread::sortChannels(std::map<std::string, std::vector<int> > &channelInfo)
{
	mutex.lock();
	
	// fill info
	for (int i = 0; i < channels.size(); i++)
	{
		channels[i].index = channelInfo["newIndices"][i];
		channels[i].direction = channelInfo["directions"][i];
	}
	// sort
	std::sort(channels.begin(), channels.end(),
		[](const UevaChannel &a, const UevaChannel &b)
	{
		return a.index < b.index;
	});
	// update channel contours after sort
	channelContours.clear();
	for (int i = 0; i < channels.size(); i++)
	{
		channelContours.push_back(Ueva::mask2Contour(channels[i].mask));
	}

	mutex.unlock();
}

void S2EngineThread::loadCtrl(std::string fileName,
	int *numPlantState, int *numPlantInput, int *numPlantOutput, int *numCtrl, double *ctrlTs)
{
	mutex.lock();

	ctrls.clear();
	cv::FileStorage fs(fileName, cv::FileStorage::READ);
	*numCtrl = (int)fs["numCtrl"];
	*ctrlTs = (double)fs["samplePeriod"];
	*numPlantState = (int)fs["numPlantState"];
	*numPlantInput = (int)fs["numPlantInput"];
	*numPlantOutput = (int)fs["numPlantOutput"];
	UevaCtrl::samplePeriod = (double)fs["samplePeriod"];
	UevaCtrl::numPlantState = (int)fs["numPlantState"];
	UevaCtrl::numPlantInput = (int)fs["numPlantInput"];
	UevaCtrl::numPlantOutput = (int)fs["numPlantOutput"];

	for (int i = 0; i < *numCtrl; i++)
	{
		std::string ctrlName = "ctrl " + std::to_string(i);
		cv::FileNode c = fs[ctrlName];
		UevaCtrl ctrl;

		ctrl.uncoUnob = (int)c["uncoUnob"];
		ctrl.n = (int)c["n"];
		ctrl.m = (int)c["m"];
		ctrl.p = (int)c["p"];
		c["outputIdx"] >> ctrl.outputIndices;
		c["stateIdx"] >> ctrl.stateIndices;
		c["A"] >> ctrl.A;
		c["B"] >> ctrl.B;
		c["C"] >> ctrl.C;
		c["D"] >> ctrl.D;
		c["K1"] >> ctrl.K1;
		c["K2"] >> ctrl.K2;
		c["H"] >> ctrl.H;
		c["Ad"] >> ctrl.Ad;
		c["Bd"] >> ctrl.Bd;
		c["Cd"] >> ctrl.Cd;
		c["Wd"] >> ctrl.Wd;

		ctrls.push_back(ctrl);

		std::cerr << "controller " << ctrlName << std::endl;
		std::cerr << "unco unob " << ctrl.uncoUnob << std::endl;
		std::cerr << "n " << ctrl.n << std::endl;
		std::cerr << "m " << ctrl.m << std::endl;
		std::cerr << "p " << ctrl.p << std::endl;
		std::cerr << "output indices " << ctrl.outputIndices << std::endl;
		std::cerr << "state indices " << ctrl.stateIndices << std::endl;
		std::cerr << "A " << ctrl.A << std::endl;
		std::cerr << "B " << ctrl.B << std::endl;
		std::cerr << "C " << ctrl.C << std::endl;
		std::cerr << "D " << ctrl.D << std::endl;
		std::cerr << "K1 " << ctrl.K1 << std::endl;
		std::cerr << "K2 " << ctrl.K2 << std::endl;
		std::cerr << "H " << ctrl.H << std::endl;
		std::cerr << "Ad " << ctrl.Ad << std::endl;
		std::cerr << "Bd " << ctrl.Bd << std::endl;
		std::cerr << "Cd " << ctrl.Cd << std::endl;
		std::cerr << "Wd " << ctrl.Wd << std::endl;
		std::cerr << std::endl;
	}
	fs.release();

	mutex.unlock();
}

void S2EngineThread::initImgproc()
{
	mutex.lock();

	// nothing to do

	mutex.unlock();
}

void S2EngineThread::finalizeImgproc()
{
	mutex.lock();

	UevaMarker::counter = 0;
	newMarkers.clear();
	oldMarkers.clear();
	activatedChannelIndices.clear();
	for (int i = 0; i < channels.size(); i++)
	{
		channels[i].biggestDropletIndex = -1;
		channels[i].measuringMarkerIndex = -1;
		channels[i].neckDropletIndex = -1;
	}

	mutex.unlock();
}

void S2EngineThread::initCtrl()
{
	mutex.lock();

	CV_Assert(!channels.empty());
	CV_Assert(!ctrls.empty());
	CV_Assert(!settings.inletRequests.empty());
	
	needSelecting = true;
	needReleasing = true;

	ground = QVector<qreal>(UevaCtrl::numPlantInput, 0.0);
	correction = QVector<qreal>(UevaCtrl::numPlantInput, 0.0);
	reference = QVector<qreal>(UevaCtrl::numPlantOutput, 0.0);
	output = QVector<qreal>(UevaCtrl::numPlantOutput, 0.0);
	outputRaw = QVector<qreal>(UevaCtrl::numPlantOutput, 0.0);
	outputOffset = QVector<qreal>(UevaCtrl::numPlantOutput, 0.0);
	outputLuenburger = QVector<qreal>(UevaCtrl::numPlantOutput, 0.0);
	outputKalman = QVector<qreal>(UevaCtrl::numPlantOutput, 0.0);
	stateKalman = QVector<qreal>(UevaCtrl::numPlantState, 0.0);
	disturbance = QVector<qreal>(UevaCtrl::numPlantInput, 0.0);
	stateLuenburger = QVector<qreal>(UevaCtrl::numPlantState, 0.0);
	stateIntegral = QVector<qreal>(UevaCtrl::numPlantOutput, 0.0);
	command = QVector<qreal>(UevaCtrl::numPlantInput, 0.0);

	for (int i = 0; i < UevaCtrl::numPlantInput; i++)
	{
		ground[i] = settings.inletRequests[i];
	}
	mutex.unlock();
}

void S2EngineThread::finalizeCtrl(QVector<qreal> &inletRegurgitates)
{
	mutex.lock();

	CV_Assert(!channels.empty());
	CV_Assert(!ctrls.empty());
	CV_Assert(!settings.inletRequests.empty());
	
	needSelecting = false;
	needReleasing = false;
	for (int i = 0; i < UevaCtrl::numPlantInput; i++)
	{
		inletRegurgitates.push_back(ground[i] + correction[i]);
	}
	mutex.unlock();
}

//// CONTINUOUS
void S2EngineThread::run()
{
	forever
	{
		if (!idle)
		{
			mutex.lock();
			//QTime entrance = QTime::currentTime();

			//// OPEN LOOP
			data.map["inletWrite"] = settings.inletRequests;

			//// MASK MAKING
			if (settings.flag & UevaSettings::MASK_MAKING)
			{
				CV_Assert(!bkgd.empty());
				// detect walls with adaptive threshold (most time consuming)
				cv::adaptiveThreshold(bkgd, dropletMask,
					HIGH_VALUE,
					cv::ADAPTIVE_THRESH_GAUSSIAN_C,
					cv::THRESH_BINARY_INV,
					settings.maskBlockSize,
					settings.maskThreshold);
				// flood base on user seed point
				if ((settings.mouseLines[0].x1() / settings.displayScale >= 0) &&
					(settings.mouseLines[0].x1() / settings.displayScale < dropletMask.cols) &&
					(settings.mouseLines[0].y1() / settings.displayScale >= 0) &&
					(settings.mouseLines[0].y1() / settings.displayScale < dropletMask.rows))
				{
					seed.x = settings.mouseLines[0].x1() / settings.displayScale;
					seed.y = settings.mouseLines[0].y1() / settings.displayScale;
				}
				floodFillReturn = cv::floodFill(dropletMask, seed, MID_VALUE);
				// eliminate noise and wall by manual morphological opening (second most time consuming)
				structuringElement = cv::getStructuringElement(cv::MORPH_RECT,
					cv::Size_<int>(settings.maskOpenSize + 3, settings.maskOpenSize + 3));
				cv::erode(dropletMask, dropletMask, structuringElement);
				structuringElement = cv::getStructuringElement(cv::MORPH_RECT,
					cv::Size_<int>(settings.maskOpenSize, settings.maskOpenSize));
				cv::dilate(dropletMask, dropletMask, structuringElement);
				// draw
				cv::cvtColor(dropletMask, data.drawnBgr, CV_GRAY2BGR);
				cv::cvtColor(data.drawnBgr, data.drawnRgb, CV_BGR2RGB);
				cv::resize(data.drawnRgb, data.drawnRgb, cv::Size(), settings.displayScale, settings.displayScale, 1);
			}

			//// CHANNEL CUTTING
			else if (settings.flag & UevaSettings::CHANNEL_CUTTING)
			{
				CV_Assert(!dropletMask.empty());
				// further erode to get thinner mask
				structuringElement = cv::getStructuringElement(cv::MORPH_RECT,
					cv::Size_<int>(settings.channelErodeSize, settings.channelErodeSize));
				cv::erode(dropletMask, markerMask, structuringElement);
				// cut into channels
				allChannels = markerMask.clone();
				for (int i = 1; i < settings.mouseLines.size(); i++)
				{
					cv::Point_<int> pt1 = cv::Point_<int>(
						settings.mouseLines[i].x1()/settings.displayScale,
						settings.mouseLines[i].y1() / settings.displayScale);
					cv::Point_<int> pt2 = cv::Point_<int>(
						settings.mouseLines[i].x2() / settings.displayScale,
						settings.mouseLines[i].y2() / settings.displayScale);
					cv::line(allChannels, pt1, pt2, cv::Scalar(0), settings.channelCutThickness);
				}
				// draw
				cv::Mat drawn;
				cv::add(dropletMask, allChannels, drawn);
				cv::cvtColor(drawn, data.drawnBgr, CV_GRAY2BGR);
				cv::cvtColor(data.drawnBgr, data.drawnRgb, CV_BGR2RGB);
				cv::resize(data.drawnRgb, data.drawnRgb, cv::Size(), settings.displayScale, settings.displayScale, 1);
			}
			else
			{	
				//// IMGPROC
				if (settings.flag & UevaSettings::IMGPROC_ON) 
				{
					CV_Assert(!bkgd.empty());
					CV_Assert(!dropletMask.empty());
					CV_Assert(!markerMask.empty());
					CV_Assert(!channels.empty());

					// background subtraction to get edges
					cv::absdiff(data.rawGray, bkgd, allMarkers);
					cv::threshold(allMarkers, allMarkers,
						settings.imgprogThreshold,
						HIGH_VALUE,
						cv::THRESH_BINARY);
					// flood and complement to get internals
					allDroplets = allMarkers.clone();
					seed = cv::Point(0, 0);
					floodFillReturn = cv::floodFill(allDroplets, seed, HIGH_VALUE);
					allDroplets = HIGH_VALUE - allDroplets;
					// combine edges and internals to get whole droplets 
					cv::bitwise_or(allMarkers, allDroplets, allDroplets);
					// Exclude noise with masks
					cv::bitwise_and(allMarkers, markerMask, allMarkers);
					cv::bitwise_and(allDroplets, dropletMask, allDroplets);
					// polish droplets with erosion for better kink detection
					structuringElement = cv::getStructuringElement(cv::MORPH_RECT,
						cv::Size_<int>(settings.imgprogErodeSize, settings.imgprogErodeSize));
					cv::erode(allDroplets, allDroplets, structuringElement);
					// find countours
					dropletContours.clear();
					markerContours.clear();
					cv::findContours(allMarkers, markerContours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);
					cv::findContours(allDroplets, dropletContours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_NONE);
					// filter contours base on size	
					Ueva::bigPassFilter(markerContours, settings.imgprogContourSize);
					Ueva::bigPassFilter(dropletContours, settings.imgprogContourSize);

					// vector of marker
					newMarkers.clear();
					for (int i = 0; i < markerContours.size(); i++)
					{
						UevaMarker marker;
						mom = cv::moments(markerContours[i]);
						marker.centroid.x = mom.m10 / mom.m00;
						marker.centroid.y = mom.m01 / mom.m00;
						marker.rect = cv::Rect_<int>(
							marker.centroid.x - settings.ctrlMarkerSize / 2,
							marker.centroid.y - settings.ctrlMarkerSize / 2,
							settings.ctrlMarkerSize,
							settings.ctrlMarkerSize);
						newMarkers.push_back(marker);
					}
					
					// old to new markers (object tracking)
					Ueva::trackMarkerIdentities(newMarkers, oldMarkers, settings.imgprogTrackTooFar);
					
					// vector of droplet
					droplets.clear();
					for (int i = 0; i < dropletContours.size(); i++)
					{
						UevaDroplet droplet;
						droplet.mask = Ueva::contour2Mask(dropletContours[i], allDroplets.size());
						droplet.kinkIndex = Ueva::detectKink(dropletContours[i], settings.imgprocConvexSize);
						if (droplet.kinkIndex != -1)
						{
							droplet.neckIndex = Ueva::detectNeck(dropletContours[i],
								droplet.kinkIndex,
								droplet.neckDistance,
								settings.imgprocPersistence);
						}
						droplets.push_back(droplet);
					}
					if (UevaDroplet::fileStream.is_open())
					{
						UevaDroplet::fileStream << std::endl;
					}

					// droplet to channel
					for (int i = 0; i < channels.size(); i++)
					{
						channels[i].biggestDropletIndex = -1;
						int maxOverlap = 0;
						for (int j = 0; j < droplets.size(); j++)
						{
							int overlap = Ueva::masksOverlap(droplets[j].mask, channels[i].mask);
							if (overlap > maxOverlap)
							{
								maxOverlap = overlap;
								channels[i].biggestDropletIndex = j;
							}
						}
					}

					// renew marker index base on identity and whether to keep using neck
					for (int i = 0; i < channels.size(); i++)
					{
						// last cycle has marker
						if (channels[i].measuringMarkerIndex != -1 && channels[i].neckDropletIndex == -1)
						{
							int stillExistMarkerIndex = -1;
							for (int j = 0; j < newMarkers.size(); j++)
							{
								if (newMarkers[j].identity == oldMarkers[channels[i].measuringMarkerIndex].identity)
								{
									stillExistMarkerIndex = j;
									break;
								}
							}
							if (stillExistMarkerIndex != -1)
							{
								if (Ueva::isMarkerInChannel(newMarkers[stillExistMarkerIndex], channels[i], 0, 0))
								{
									// marker in channel 
									channels[i].measuringMarkerIndex = stillExistMarkerIndex;
								}
								else
								{
									// marker escaped channel
									channels[i].measuringMarkerIndex = -1;
									Ueva::deleteFromCombination(activatedChannelIndices, i);
									alwaysTrue = Ueva::isCombinationPossible(activatedChannelIndices, ctrls);
									needReleasing = true;
								}
							}
							else
							{
								// marker disappeared from image
								channels[i].measuringMarkerIndex = -1;
								Ueva::deleteFromCombination(activatedChannelIndices, i);
								alwaysTrue = Ueva::isCombinationPossible(activatedChannelIndices, ctrls);
								needReleasing = true;
							}
						}
						// last cycle has neck
						if (channels[i].measuringMarkerIndex == -1 && channels[i].neckDropletIndex != -1)
						{
							if (channels[i].biggestDropletIndex != -1 && settings.useNeckRequests[i])
							{
								if (droplets[channels[i].biggestDropletIndex].kinkIndex != -1 &&
									droplets[channels[i].biggestDropletIndex].neckIndex != -1)
								{
									// neck still exist 
									channels[i].neckDropletIndex = channels[i].biggestDropletIndex;
								}
								else
								{
									// neck no longer exist
									channels[i].neckDropletIndex = -1;
									Ueva::deleteFromCombination(activatedChannelIndices, i);
									alwaysTrue = Ueva::isCombinationPossible(activatedChannelIndices, ctrls);
									needReleasing = true;
								}
							}
							else
							{
								// droplet disappeared from image or neck not used anymore
								channels[i].neckDropletIndex = -1;
								Ueva::deleteFromCombination(activatedChannelIndices, i);
								alwaysTrue = Ueva::isCombinationPossible(activatedChannelIndices, ctrls);
								needReleasing = true;
							}
						}
					}
					oldMarkers.clear();
					oldMarkers = newMarkers;

					// user inputs
					mousePressLeft.x = settings.leftPressPosition.x() / settings.displayScale;
					mousePressLeft.y = settings.leftPressPosition.y() / settings.displayScale;
					mousePressRight.x = settings.rightPressPosition.x() / settings.displayScale;
					mousePressRight.y = settings.rightPressPosition.y() / settings.displayScale;
					mousePressPrevious.x = settings.leftPressMovement.x1() / settings.displayScale;
					mousePressPrevious.y = settings.leftPressMovement.y1() / settings.displayScale;
					mousePressCurrent.x = settings.leftPressMovement.x2() / settings.displayScale;
					mousePressCurrent.y = settings.leftPressMovement.y2() / settings.displayScale;
					mousePressDisplacement = mousePressCurrent - mousePressPrevious;

					// user add or remove marker
					for (int i = 0; i < newMarkers.size(); i++)
					{
						if (mousePressLeft.inside(newMarkers[i].rect))
						{
							for (int j = 0; j < channels.size(); j++)
							{
								if (Ueva::isMarkerInChannel(newMarkers[i], channels[j], 0, 0))
								{
									// channel already activated
									if (channels[j].measuringMarkerIndex != -1 && channels[j].neckDropletIndex == -1)
									{
										if (channels[j].measuringMarkerIndex == i)
										{
											// same marker
										}
										else
										{
											// swap marker 
											channels[j].measuringMarkerIndex = i;
											needSelecting = true;
										}
									}
									// channel not activated
									if (channels[j].measuringMarkerIndex == -1 && channels[j].neckDropletIndex == -1)
									{
										desiredChannelIndices = activatedChannelIndices;
										desiredChannelIndices.push_back(j);
										if (Ueva::isCombinationPossible(desiredChannelIndices, ctrls))
										{
											// activate channel
											activatedChannelIndices = desiredChannelIndices;
											channels[j].measuringMarkerIndex = i;
											needSelecting = true;
										}
									}
									break;
								}
							}
						}
						if (mousePressRight.inside(newMarkers[i].rect))
						{
							for (int j = 0; j < channels.size(); j++)
							{
								if (channels[j].measuringMarkerIndex == i)
								{
									// deactivate channel
									channels[j].measuringMarkerIndex = -1;
									Ueva::deleteFromCombination(activatedChannelIndices, j);
									alwaysTrue = Ueva::isCombinationPossible(activatedChannelIndices, ctrls);
									needReleasing = true;
									break;
								}
							}
						}
					}

					// auto catch and use neck
					for (int i = 0; i < channels.size(); i++)
					{
						// auto catch (high priority)
						if (settings.autoCatchRequests[i] &&
							channels[i].measuringMarkerIndex == -1 &&
							channels[i].neckDropletIndex == -1)
						{
							for (int j = 0; j < newMarkers.size(); j++)
							{
								if (Ueva::isMarkerInChannel(newMarkers[j], channels[i],
									settings.ctrlAutoHorzExcl, settings.ctrlAutoVertExcl))
								{
									desiredChannelIndices = activatedChannelIndices;
									desiredChannelIndices.push_back(i);
									if (Ueva::isCombinationPossible(desiredChannelIndices, ctrls))
									{
										// activate channel with marker
										activatedChannelIndices = desiredChannelIndices;
										channels[i].measuringMarkerIndex = j;
										needSelecting = true;
									}
									break;
								}
							}
						}
						// use neck (low priority)
						if (settings.useNeckRequests[i] &&
							channels[i].measuringMarkerIndex == -1 &&
							channels[i].neckDropletIndex == -1)
						{
							if (channels[i].biggestDropletIndex != -1)
							{
								if (droplets[channels[i].biggestDropletIndex].kinkIndex != -1 &&
									droplets[channels[i].biggestDropletIndex].neckIndex != -1)
								{
									desiredChannelIndices = activatedChannelIndices;
									desiredChannelIndices.push_back(i);
									if (Ueva::isCombinationPossible(desiredChannelIndices, ctrls))
									{
										// activate channel with neck
										activatedChannelIndices = desiredChannelIndices;
										channels[i].neckDropletIndex = channels[i].biggestDropletIndex;
										needSelecting = true;
									}
								}
							}
						}
					}

					// debug
					//std::cerr << std::endl;
					//std::cerr << settings.ctrlNeckDesire << " "
					//	<< settings.ctrlNeckThreshold << " "
					//	<< settings.ctrlNeckGain << std::endl;
					//std::cerr << "dx: " << mousePressDisplacement.x << std::endl;
					//std::cerr << "dy: " << mousePressDisplacement.y << std::endl;
					//for (int i = 0; i < channels.size(); i++)
					//{
					//	std::cerr << "ch" << i << " "
					//		<< channels[i].biggestDropletIndex << " "
					//		<< channels[i].measuringMarkerIndex << " "
					//		<< channels[i].neckDropletIndex << std::endl;
					//}
					//for (int i = 0; i < activatedChannelIndices.size(); i++)
					//{
					//	std::cerr << activatedChannelIndices[i] << " ";
					//}
					//std::cerr << std::endl;
				}
				//// CTRL
				if (settings.flag & UevaSettings::CTRL_ON)
				{
					CV_Assert(!channels.empty());
					CV_Assert(!ctrls.empty());
					CV_Assert(!settings.inletRequests.empty());

					if (!activatedChannelIndices.empty())
					{
						UevaCtrl ctrl = ctrls[UevaCtrl::index];

						// reset
						if (needReleasing || needSelecting)
						{
							posteriorErrorCov = 1000.0 * cv::Mat::eye(
								ctrl.n + ctrl.m, ctrl.n + ctrl.m, CV_64FC1);

							modelNoiseCov = settings.ctrlModelCov * cv::Mat::eye(
								ctrl.n, ctrl.n,	CV_64FC1);

							disturbanceNoiseCov = settings.ctrlDisturbanceCorr * cv::Mat::eye(
								ctrl.m,	ctrl.m, CV_64FC1);

							cv::Mat zerosNbyM = cv::Mat(ctrl.n,	ctrl.m,	CV_64FC1, cv::Scalar(0.0));

							cv::Mat zerosMbyN = cv::Mat(ctrl.m,	ctrl.n,	CV_64FC1, cv::Scalar(0.0));

							cv::Mat onesNbyNplusM;
							cv::Mat onesMbyNplusM;
							cv::hconcat(modelNoiseCov, zerosNbyM, onesNbyNplusM);
							cv::hconcat(zerosMbyN, disturbanceNoiseCov, onesMbyNplusM);
							cv::vconcat(onesNbyNplusM, onesMbyNplusM, processNoiseCov);
							
							sensorNoiseCov = std::pow(micronPerPixel, 2) / 12.0 * cv::Mat::eye(
								ctrl.p, ctrl.p,	CV_64FC1);
						}
						if (needReleasing)
						{
							reference = QVector<qreal>(UevaCtrl::numPlantOutput, 0.0);
							output = QVector<qreal>(UevaCtrl::numPlantOutput, 0.0);
							outputLuenburger = QVector<qreal>(UevaCtrl::numPlantOutput, 0.0);
							outputRaw = QVector<qreal>(UevaCtrl::numPlantOutput, 0.0);
							outputOffset = QVector<qreal>(UevaCtrl::numPlantOutput, 0.0);
							outputKalman = QVector<qreal>(UevaCtrl::numPlantOutput, 0.0);
							stateKalman = QVector<qreal>(UevaCtrl::numPlantState, 0.0);
							disturbance = QVector<qreal>(UevaCtrl::numPlantInput, 0.0);
							stateLuenburger = QVector<qreal>(UevaCtrl::numPlantState, 0.0);
							stateIntegral = QVector<qreal>(UevaCtrl::numPlantOutput, 0.0);
							command = QVector<qreal>(UevaCtrl::numPlantInput, 0.0);
						}

						// from previous
						k = cv::Mat(ctrl.n + ctrl.m, ctrl.p, CV_64FC1, cv::Scalar(0.0));
						pp = cv::Mat(ctrl.n + ctrl.m, ctrl.n + ctrl.m, CV_64FC1, cv::Scalar(0.0));
						pe = posteriorErrorCov;
						rw = processNoiseCov;
						rv = sensorNoiseCov;

						r = cv::Mat(ctrl.p, 1, CV_64FC1, cv::Scalar(0.0));
						dr = cv::Mat(ctrl.p, 1, CV_64FC1, cv::Scalar(0.0));
						y = cv::Mat(ctrl.p, 1, CV_64FC1, cv::Scalar(0.0));
						y_raw = cv::Mat(ctrl.p, 1, CV_64FC1, cv::Scalar(0.0));
						y_off = cv::Mat(ctrl.p, 1, CV_64FC1, cv::Scalar(0.0));
						yk = cv::Mat(ctrl.p, 1, CV_64FC1, cv::Scalar(0.0));
						yl = cv::Mat(ctrl.p, 1, CV_64FC1, cv::Scalar(0.0));
						xp = cv::Mat(ctrl.n + ctrl.m, 1, CV_64FC1, cv::Scalar(0.0));
						xe = cv::Mat(ctrl.n + ctrl.m, 1, CV_64FC1, cv::Scalar(0.0));
						xl = cv::Mat(ctrl.n, 1, CV_64FC1, cv::Scalar(0.0));
						z = cv::Mat(ctrl.p, 1, CV_64FC1, cv::Scalar(0.0));
						u = cv::Mat(ctrl.m, 1, CV_64FC1, cv::Scalar(0.0));

						for (int i = 0; i < ctrl.p; i++)
						{
							r.at<double>(i) = reference[ctrl.outputIndices.at<uchar>(i)];
							y_raw.at<double>(i) = outputRaw[ctrl.outputIndices.at<uchar>(i)];
							y_off.at<double>(i) = outputOffset[ctrl.outputIndices.at<uchar>(i)];
							z.at<double>(i) = stateIntegral[ctrl.outputIndices.at<uchar>(i)];
						}
						for (int i = 0; i < ctrl.n; i++)
						{
							xe.at<double>(i) = stateKalman[ctrl.stateIndices.at<uchar>(i)];
							xl.at<double>(i) = stateLuenburger[ctrl.stateIndices.at<uchar>(i)];
						}
						for (int i = 0; i < ctrl.m; i++)
						{
							u.at<double>(i) = command[i];
							xe.at<double>(i + ctrl.n) = disturbance[i];
						}

						// direct request
						directRequestIndex = -1;
						for (int i = 0; i < activatedChannelIndices.size(); i++)
						{
							if (channels[activatedChannelIndices[i]].measuringMarkerIndex != -1)
							{
								rect = newMarkers[channels[activatedChannelIndices[i]].measuringMarkerIndex].rect;
								if (mousePressPrevious.inside(rect) && mousePressCurrent.inside(rect))
								{
									directRequestIndex = i;
									dr.at<double>(i) = Ueva::screen2ctrl(mousePressDisplacement,
										channels[activatedChannelIndices[i]].direction, micronPerPixel);
									break;
								}
							}
						}
						
						// link requests
						for (int i = 0; i < activatedChannelIndices.size(); i++)
						{
							if (directRequestIndex != i && directRequestIndex != -1)
							{
								if (settings.linkRequests[activatedChannelIndices[i]])
								{
									if (settings.inverseLinkRequests[activatedChannelIndices[i]])
									{
										dr.at<double>(i) = -dr.at<double>(directRequestIndex);
									}
									else
									{
										dr.at<double>(i) = dr.at<double>(directRequestIndex);
									}
								}
							}
						}

						// reference
						r += dr;

						// measurment
						for (int i = 0; i < activatedChannelIndices.size(); i++)
						{
							// marker raw output
							if (channels[activatedChannelIndices[i]].measuringMarkerIndex != -1 &&
								channels[activatedChannelIndices[i]].neckDropletIndex == -1)
							{
								y.at<double>(i) = Ueva::screen2ctrl(
									newMarkers[channels[activatedChannelIndices[i]].measuringMarkerIndex].centroid,
									channels[activatedChannelIndices[i]].direction,
									micronPerPixel);
							}
							// fake raw output
							if (channels[activatedChannelIndices[i]].measuringMarkerIndex == -1 &&
								channels[activatedChannelIndices[i]].neckDropletIndex != -1)
							{
								if (directRequestIndex != i && directRequestIndex != -1)
								{
									if (settings.neckDirectionRequests[activatedChannelIndices[i]])
									{
										y.at<double>(i) = y_raw.at<double>(i) + dr.at<double>(directRequestIndex);
									}
									else
									{
										y.at<double>(i) = y_raw.at<double>(i) - dr.at<double>(directRequestIndex);
									}
								}
								else
								{
									double value = y_raw.at<double>(i); // get around weird behavior of cv::Mat::at
									y.at<double>(i) = value; 
								}
							}
						}
						// update offset
						if (needSelecting || needReleasing)
						{								
							y_off += y - y_raw;
						}
						// save raw output
						y_raw = y.clone();
						// modify output with neck
						for (int i = 0; i < activatedChannelIndices.size(); i++)
						{
							if (channels[activatedChannelIndices[i]].measuringMarkerIndex == -1 &&
								channels[activatedChannelIndices[i]].neckDropletIndex != -1)
							{
								double value = y.at<double>(i); // get around weird behavior of cv::Mat::at
								y.at<double>(i) = value + Ueva::neck2ctrl(
									droplets[channels[activatedChannelIndices[i]].neckDropletIndex].neckDistance,
									micronPerPixel,
									settings.ctrlNeckDesire,
									settings.ctrlNeckThreshold,
									settings.ctrlNeckLowerGain,
									settings.ctrlNeckHigherGain); // assume +ve always into junction for interface
							}
						}
						// output = (raw and modified) - offset
						y -= y_off;	

						// kalman filter
						pp = ctrl.Ad * pe * ctrl.Ad.t() + ctrl.Wd * rw * ctrl.Wd.t();
						cv::Mat mat = ctrl.Cd * pp * ctrl.Cd.t() + rv;
						k = pp * ctrl.Cd.t() * mat.inv();
						cv::Mat eyeNplusM = cv::Mat::eye(ctrl.n + ctrl.m, ctrl.n + ctrl.m, CV_64FC1);
						pe = (eyeNplusM - k * ctrl.Cd) * pp;
						xp = ctrl.Ad * xe + ctrl.Bd * u;
						yk = ctrl.Cd * xp;
						xe = xp + k * (y - yk);

						// luenburger
						yl = ctrl.C * xl + ctrl.D * u;
						xl = ctrl.A * xl + ctrl.B * u + ctrl.H * (y - yl);

						// integral state feed back
						z += UevaCtrl::samplePeriod * (y - r);
						u = -ctrl.K1 * xl - ctrl.K2 * z;

						// carry forward
						
						for (int i = 0; i < ctrl.p; i++)
						{
							uchar outputIndex = ctrl.outputIndices.at<uchar>(i);
							reference[outputIndex] = r.at<double>(i);
							output[outputIndex] = y.at<double>(i);
							outputLuenburger[outputIndex] = yl.at<double>(i);
							outputRaw[outputIndex] = y_raw.at<double>(i);
							outputOffset[outputIndex] = y_off.at<double>(i);
							outputKalman[outputIndex] = yk.at<double>(i);
							stateIntegral[outputIndex] = z.at<double>(i);
						}
						for (int i = 0; i < ctrl.n; i++)
						{
							uchar stateIndex = ctrl.stateIndices.at<uchar>(i);
							stateLuenburger[stateIndex] = xl.at<double>(i);
							stateKalman[stateIndex] = xe.at<double>(i);
						}
						for (int i = 0; i < ctrl.m; i++)
						{
							command[i] = u.at<double>(i);
							disturbance[i] = xe.at<double>(ctrl.n + i);
							correction[i] += settings.ctrlDisturbanceCorr * disturbance[i];
						}
						//posteriorErrorCov = pe.clone(); redundant

						// clean up
						needSelecting = false;
						needReleasing = false;
					}
					// check out
					for (int i = 0; i < UevaCtrl::numPlantInput; i++)
					{
						data.map["inletWrite"][i] = ground[i] + correction[i] + command[i];
					}
					data.map["ctrlGround"] = ground;
					data.map["ctrlCorrection"] = correction;
					data.map["ctrlReference"] = reference;
					data.map["ctrlOutput"] = output;
					data.map["ctrlOutputLuenburger"] = outputLuenburger;
					data.map["ctrlOutputKalman"] = outputKalman;
					data.map["ctrlOutputRaw"] = outputRaw;
					data.map["ctrlOutputOffset"] = outputOffset;
					data.map["ctrlStateKalman"] = stateKalman;
					data.map["ctrlDisturbance"] = disturbance;
					data.map["ctrlStateLuenburger"] = stateLuenburger;
					data.map["ctrlStateIntegral"] = stateIntegral;
					data.map["ctrlcommand"] = command;
				}
				//// DRAW
				if (!data.rawGray.empty())
				{
					cv::cvtColor(data.rawGray, data.drawnBgr, CV_GRAY2BGR);
					// draw channel contour
					if (settings.flag & UevaSettings::DRAW_CHANNEL)
					{
						lineColor = cv::Scalar(255, 255, 255); // white
						lineThickness = CV_FILLED; 
						lineType = 8;
						cv::drawContours(data.drawnBgr, channelContours, -1,
							lineColor, lineThickness, lineType);
					}
					// draw droplet contour
					if (settings.flag & UevaSettings::DRAW_DROPLET)
					{
						lineColor = cv::Scalar(255, 0, 255); // magenta
						lineThickness = 1;
						lineType = 8;
						cv::drawContours(data.drawnBgr, dropletContours, -1,
							lineColor, lineThickness, lineType);
					}
					// draw marker contour
					if (settings.flag & UevaSettings::DRAW_MARKER)
					{
						lineColor = cv::Scalar(255, 255, 0); // cyan
						lineThickness = 1;
						lineType = 8;
						cv::drawContours(data.drawnBgr, markerContours, -1,
							lineColor, lineThickness, lineType);
					}
					// draw kink and neck
					if (settings.flag & UevaSettings::DRAW_NECK)
					{
						lineColor = cv::Scalar(0, 255, 255); // yellow
						lineType = 8;
						for (int i = 0; i < droplets.size(); i++)
						{
							if (droplets[i].kinkIndex != -1)
							{
								lineThickness = 1;
								cv::circle(data.drawnBgr,
									dropletContours[i][droplets[i].kinkIndex],
									settings.ctrlMarkerSize / 2, lineColor, lineThickness, lineType);
								if (droplets[i].neckIndex != -1)
								{
									lineThickness = 3;
									cv::line(data.drawnBgr,
										dropletContours[i][droplets[i].kinkIndex],
										dropletContours[i][droplets[i].neckIndex],
										lineColor, lineThickness, lineType);
								}
							}
						}
					}
					// draw marker rect and identity
					for (int i = 0; i < newMarkers.size(); i++)
					{
						lineColor = cv::Scalar(255, 255, 0); // cyan
						lineThickness = 1;
						lineType = 8;
						cv::rectangle(data.drawnBgr, newMarkers[i].rect,
							lineColor, lineThickness, lineType);
						fontScale = 1;
						anchor.x = newMarkers[i].rect.x - 30; // offset left from leftmost
						anchor.y = newMarkers[i].rect.y - 30; // offset up from top
						str = std::to_string(newMarkers[i].identity);
						cv::putText(data.drawnBgr, str, anchor,
							cv::FONT_HERSHEY_SIMPLEX, fontScale, lineColor, lineThickness, lineType);
					}
					// draw channel text and measuring marker rect
					for (int i = 0; i < channels.size(); i++)
					{
						channels[i].makeChannelText(str, fontScale, lineColor,
							settings.linkRequests[i], settings.inverseLinkRequests[i]);
						anchor.x = channels[i].rect.x + 60; // offset right from leftmost
						anchor.y = channels[i].rect.y + channels[i].rect.height / 2 - 30; // offset up from center
						lineThickness = 1;
						lineType = 8;
						cv::putText(data.drawnBgr, str, anchor,
							cv::FONT_HERSHEY_SIMPLEX, fontScale, lineColor, lineThickness, lineType);
						if (channels[i].measuringMarkerIndex != -1)
						{
							lineColor = cv::Scalar(255, 0, 0); // blue
							cv::rectangle(data.drawnBgr, newMarkers[channels[i].measuringMarkerIndex].rect,
								lineColor, lineThickness, lineType);
						}
					}
					
					cv::cvtColor(data.drawnBgr, data.drawnRgb, CV_BGR2RGB);
					cv::resize(data.drawnRgb, data.drawnRgb, cv::Size(), settings.displayScale, settings.displayScale, 1);
				}
			}

			emit engineSignal(data);
			idle = true;
			//QTime exit = QTime::currentTime();
			//int ms = entrance.msecsTo(exit);
			//qDebug() << "engine used (ms)" << ms;
			mutex.unlock();
		}
	}
}


