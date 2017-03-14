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
 *  Description:
 *  Very similar to Aurora's Wheel, but instead of moving in a specific direction,
 *  it moves from edges of the layout to the center in 1 dimension
 */




#include "AuroraPlugin.h"
#include "LayoutProcessingUtils.h"
#include "ColorUtils.h"
#include "DataManager.h"

#ifdef __cplusplus
extern "C" {
#endif

	void initPlugin(bool* isSoundPlugin);
	void selectSoundFeature(SoundFeatureRequest_t* soundfeatureRequest);
	void getPluginFrame(SoundFeature_t* soundFeature, Frame_t* frames, int* nFrames, int* sleepTime);
	void pluginCleanup();

#ifdef __cplusplus
}
#endif

int hue = 0;

FrameSlice_t* frameSlices = NULL;
int nFrameSlices = 0;
int transTime = 15;

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
	*isSoundPlugin = false; //not a sound plugin

	//grab the layout data, this function returns a pointer to a statically allocated buffer. Safe to call as many time as required.
	//Dont delete this pointer. The memory is managed automatically.
	LayoutData* layoutData = getLayoutData();

	rotateAuroraPanels(layoutData, &layoutData->globalOrientation);

	//quantizes the layout into frameslices. See SDK documentation for more information
	getFrameSlicesFromLayoutForTriangle(layoutData, &frameSlices, &nFrameSlices, layoutData->globalOrientation);
	printf("sideLength %d\n", Shape::sideLength);
	printf ("nFrameSlices %d\n", nFrameSlices);
	printf("initPlugin\n");
}

/**
 * @description: This function is called only if the isSoundPLugin Flag is set in the initPlugin function.
 * Here the plugin is allowed to choose the soundFeatures it needs to run.
 *
 * @param soundFeatureRequest A pointer to a SoundFeatureRequest_t instance
 */
void selectSoundFeature(SoundFeatureRequest_t* soundfeatureRequest){

}

/**
 * A helper function thats fills up the frames array at framesIndex with a specified frameslices
 * and a specified hue. the color is the specified hue at 100% saturation and brightness.
 * Note that the FrameSlice_t structure is just a vector of panels at that frame slice
 */
void fillUpFramesArray(FrameSlice_t* frameSlice, Frame_t* frames, int* framesIndex, int hue){
	static RGB_t rgb;
	for (int i = 0; i < frameSlice->panelIds.size(); i++){
		frames[*framesIndex].panelId = frameSlice->panelIds[i];
		HSVtoRGB((HSV_t){hue, 100, 100}, &rgb);
		frames[*framesIndex].r = rgb.R;
		frames[*framesIndex].g = rgb.G;
		frames[*framesIndex].b = rgb.B;
		frames[*framesIndex].transTime = transTime;
		(*framesIndex)++;
	}
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
 * @param frames: a pre-allocated buffer of the Frame_t structure to fill up with RGB values to show on panels.
 * Maximum size of this buffer is equal to the number of panels
 * @param nFrames: fill with the number of frames in frames
 * @param sleepTime: specify interval after which this function is called again, NULL if sound visualization plugin
 */
void getPluginFrame(SoundFeature_t* soundFeature, Frame_t* frames, int* nFrames, int* sleepTime){

	/**
	 * the hue variable increases by 30 degrees everytime this function is called (which is every sleepTime seconds)
	 * the spaitalHue variable, runs over the frameSlices outer to inner, and increments by 15 degrees (hueStep)
	 * The frameSlices are from left to right of the rotated layout, so the outer will be the first and the last elements of the frameslices array
	 * and the inner the middle element of the frameslices array
	 */
	int index = 0;
	int spatialHue = hue;
	int hueStep = 15;
	if (nFrameSlices % 2 != 0){
		fillUpFramesArray(&frameSlices[nFrameSlices/2], frames, &index, spatialHue%360);
		spatialHue += hueStep;
	}

	for (int i = nFrameSlices/2 - 1; i >= 0; i--){
		fillUpFramesArray(&frameSlices[i], frames, &index, spatialHue%360);
		fillUpFramesArray(&frameSlices[nFrameSlices - 1 - i], frames, &index, spatialHue%360);
		spatialHue += hueStep;
	}

	hue += 30;
	if (hue > 360){
		hue = 0;
	}

	*nFrames = index;
	//in a non-music effect, the sleeptime is determined by the plugin itself.
	//Important that this variable is set correctly by the plugin.
	*sleepTime = transTime;
}

/**
 * @description: called once when the plugin is being closed.
 * Do all deallocation for memory allocated in initplugin here
 */
void pluginCleanup(){
	//do deallocation here
	freeFrameSlices(frameSlices);
}
