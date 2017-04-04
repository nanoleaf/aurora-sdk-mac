/*
    Copyright 2017 Nanoleaf Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
 */

/*
 * AuroraPlugin.cpp
 *
 *  Created on: Feb 13, 2017
 *      Author: eski
 *
 *  Description:
 *	This plugin, implements a sound bar effect that responds to the energy of the sound and 'fills'
 *	up the panels from left to right. This plugin demonstrates how to get the layout, use layout processing utilities
 *	provided by the utilities library and update the panels.
 */

#include "AuroraPlugin.h"
#include "LayoutProcessingUtils.h"
#include "ColorUtils.h"
#include "DataManager.h"
#include "SoundUtils.h"
#include <math.h>
#include <limits.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif
	void initPlugin(bool* isSoundPlugin);
	void selectSoundFeature(SoundFeatureRequest_t* soundfeatureRequest);
	void getPluginFrame(SoundFeature_t* soundFeature, Frame_t* frame, int* nPanels, int* sleepTime);
	void pluginCleanup();
#ifdef __cplusplus
}
#endif

LayoutData* layoutData;
FrameSlice_t* frameSlices = NULL;
int nPanelslices = 0;

int findMaxExpanse(){
	int maxDegrees = 0;

	//grab the layout data, this function returns a pointer to a statically allocated buffer. Safe to call as many time as required.
	//Dont delete this pointer. The memory is managed automatically.
	layoutData = getLayoutData();
	int maxExpanse = INT_MIN;
	int d;
	//cycle through 0-360 degrees at multiples of 30 degrees
	for (d = 0; d < 360/30; d++){
		int angleToRotateBy = 30;
		rotateAuroraPanels(layoutData, &angleToRotateBy);
		//for every rotation, find the expanse, and hunt for the maximum
		int maxX = INT_MIN, minX = INT_MAX;
		for (int i = 0; i < layoutData->nPanels; i++){
			if (maxX < layoutData->panels[i].shape->getCentroid().x){
				maxX = (int)layoutData->panels[i].shape->getCentroid().x;
			}
			if (minX > layoutData->panels[i].shape->getCentroid().x){
				minX = (int)layoutData->panels[i].shape->getCentroid().x;
			}
		}
		if (maxExpanse < (maxX - minX)){
			maxExpanse = (maxX - minX);
			maxDegrees = d*30;
		}
	}

	//turn the layout back to the maDegrees
	int angleToRotateBack = maxDegrees - d;
	rotateAuroraPanels(layoutData, &angleToRotateBack);
	return maxDegrees;
}

/**
 * @description: Initialize the plugin. Called once, when the plugin is loaded.
 * This function can be used to load the LayoutData and the colorPalette from the DataManager.
 * Any allocation, if done here, should be deallocated in the plugin cleanup function
 *
 * @param isSoundPlugin: Setting this flag will indicate that it is a sound plugin, and accordingly
 * sound data will be passed in. If not set, the plugin will be considered an effects plugin
 *
 */
void initPlugin(bool* isSoundPlugin){
	//do allocation here
	*isSoundPlugin = true; //very much a music based effect

	//rotate the layout so that right to left have the maximum number of frame slices
	int angle = findMaxExpanse();

	//quantizes the layout into framelices. See SDK documentation for more information
	getFrameSlicesFromLayoutForTriangle(layoutData, &frameSlices, &nPanelslices, angle);
}

/**
 * @description: This function is called only if the isSoundPLugin Flag is set in the initPlugin function.
 * Here the plugin is allowed to choose the soundFeatures it needs to run.
 *
 * @param soundFeatureRequest A pointer to a SoundFeatureRequest_t instance
 */
void selectSoundFeature(SoundFeatureRequest_t* soundfeatureRequest){
	//request the sound features.
	soundfeatureRequest->energy = false;
	soundfeatureRequest->nFftBins = 16;
	soundfeatureRequest->fft = true;
}


/**
 * @description: this the 'main' function that gives a frame to the Aurora to display onto the panels
 * If the plugin is an effects plugin the soundFeature buffer will be NULL.
 * If the plugin is a sound visualization plugin, the sleepTime variable will be NULL and is not required to be
 * filled in
 * This function, if is an effects plugin, can specify the interval it is to be called at through the sleepTime variable
 * if its a sound visualization plugin, this function is called at an interval of 50ms or more.
 *
 * @param soundFeature: Carries the processed sound data from the soundModule, NULL if effects plugin
 * @param frame: a pre-allocated buffer of the Frame_t structure to fill up with RGB values to show on panels.
 * Maximum size of this buffer is equal to the number of panels
 * @param nPanels: fill with the number of frame in frame
 * @param sleepTime: specify interval after which this function is called again, NULL if sound visualization plugin
 */
void getPluginFrame(SoundFeature_t* soundFeature, Frame_t* frame, int* nPanels, int* sleepTime){
	static const uint32_t maxBarLength = 100;
	static int barMarker = 0;
	static const int barMarkerInertialRelaxationStep = 9.0;
	int frameIndex = 0;
	uint16_t energy = 0;
	for (int i = 0; i < soundFeature->nFFTBins; i++){
		//apply an exponential decay filter and sum up fftbins to get energy. This effectively boosts the bass frequencies
		energy += (uint16_t)((double)soundFeature->fftBins[i] * pow(3.5, 1.0/(i+1)));
	}
	uint32_t barLength = (energy * maxBarLength)/2000;
	RGB_t netColor;
	static RGB_t baseColor = {0, 255, 255};
	static RGB_t barColor = {240, 20, 100};
	if (barLength > barMarker){
		barMarker = barLength;
	}

	//map the length of the soundbar to the Aurora Layout. In this case,
	//calculate the number of frame that are affected from the total number of frame slices
	int nPanelsAffected = (barMarker*nPanelslices)/maxBarLength;
	if (nPanelsAffected > nPanelslices){
		nPanelsAffected = nPanelslices;
	}

	int x_start = 325;
	int x = x_start;
	int x_step = (nPanelsAffected == 0) ? 0 : x_start/nPanelsAffected;
	for (int i = 0; i < nPanelsAffected; i++){
		x = x - x_step;
		int x_t = (x > 255) ? 255 : x;
		//the net color is a mix between a weighted base color and bar color.
		//As the frameSlices moves towards the end of the bar, the effect of the bar color decreases
		//and the base color becomes stronger and stronger.
		//In other words the bar color fades into the base color
		netColor = (((barColor*x_t)/255) + ((baseColor*(255-x_t)))/255);
		netColor = limitRGB(netColor, 255, 0);
		for (int j = 0; j < frameSlices[i].panelIds.size(); j++){
			frame[frameIndex].panelId = frameSlices[i].panelIds[j];
			frame[frameIndex].r = netColor.R;
			frame[frameIndex].g = netColor.G;
			frame[frameIndex].b = netColor.B;
			frame[frameIndex].transTime = 1;
			frameIndex++;
		}
	}
	for (int i = nPanelsAffected; i < nPanelslices; i++){
		for (int j = 0; j < frameSlices[i].panelIds.size(); j++){
			frame[frameIndex].panelId = frameSlices[i].panelIds[j];
			frame[frameIndex].r = baseColor.R;
			frame[frameIndex].g = baseColor.G;
			frame[frameIndex].b = baseColor.B;
			frame[frameIndex].transTime = 2;
			frameIndex++;
		}
	}

	if (barMarker > 0){
		//the soundbar relaxes to 0 slowly, instead of jumping to the level of the sound immediately.
		//gives a more 'alive' effect.
		barMarker -= barMarkerInertialRelaxationStep;
		if (barMarker < 0){
			barMarker = 0;
		}
	}

	*nPanels = frameIndex;
}

/**
 * @description: called once when the plugin is being closed.
 * Do all deallocation for memory allocated in initplugin here
 */
void pluginCleanup(){
	//do deallocation here
	freeFrameSlices(frameSlices);
}
