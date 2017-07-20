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



#include "AuroraPlugin.h"
#include "LayoutProcessingUtils.h"
#include "ColorUtils.h"
#include "DataManager.h"
#include "PluginFeatures.h"
#include "Logger.h"
#include <stdio.h>
#include <limits.h>
#include "AveragingFilter.h"

#ifdef __cplusplus
extern "C" {
#endif

	void initPlugin();
	void getPluginFrame(Frame_t* frames, int* nFrames, int* sleepTime);
	void pluginCleanup();

#ifdef __cplusplus
}
#endif

LayoutData* layoutData;
FrameSlice_t* frameSlices = NULL;
int nFrameSlices = 0;

AveragingFilter af;

static int currentAuroraRotation = 0;

RGB_t * colorPalette = NULL;
int nColors = 0;
RGB_t barColor;
int colorIndex = 1;
RGB_t baseColor;

int findMaxExpanse(){
    int maxDegrees = 0;
    
    //grab the layout data, this function returns a pointer to a statically allocated buffer. Safe to call as many time as required.
    //Dont delete this pointer. The memory is managed automatically.
    layoutData = getLayoutData();
    int maxExpanse = INT_MIN;
    int d;
    //cycle through 0-360 degrees at multiples of 30 degrees
    for (d = 0; d < 360/30; d++){
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
        int angleToRotateBy = 30;
        rotateAuroraPanels(layoutData, &angleToRotateBy);
        printf ("Max expanse : %d\n", maxExpanse);
        printf ("d %d\n", d);
    }
    
    //turn the layout back to the maDegrees
    int angleToRotateBack = maxDegrees - d;
    rotateAuroraPanels(layoutData, &angleToRotateBack);
    return maxDegrees;
}

/**
 * @description: Initialize the plugin. Called once, when the plugin is loaded.
 * This function can be used to enable rhythm or advanced features,
 * e.g., to enable energy feature, simply call enableEnergy()
 * It can also be used to load the LayoutData and the colorPalette from the DataManager.
 * Any allocation, if done here, should be deallocated in the plugin cleanup function
 *
 */
void initPlugin(){
    //do allocation here
    //rotate the layout so that right to left have the maximum number of frame slices
    currentAuroraRotation = findMaxExpanse();
    printf ("max expanse found at angle %d", currentAuroraRotation);
    
    //quantizes the layout into frameslices. See SDK documentation for more information
    getFrameSlicesFromLayoutForTriangle(layoutData, &frameSlices, &nFrameSlices, currentAuroraRotation);
    
    getColorPalette(&colorPalette, &nColors);
    
    if (nColors == 0){
        barColor = {255, 255, 255};
        baseColor = {0, 0, 0};
    }
    else if (nColors == 1){
        barColor = colorPalette[0];
        baseColor = {0, 0, 0};
    }
    else if (nColors >= 2){
        baseColor = colorPalette[0];
        barColor = colorPalette[1];
    }
    
    enableEnergy();

}

/**
 * @description: this the 'main' function that gives a frame to the Aurora to display onto the panels
 * To obtain updated values of enabled features, simply call get<feature_name>, e.g.,
 * getEnergy(), getIsBeat().
 *
 * If the plugin is a sound visualization plugin, the sleepTime variable will be NULL and is not required to be
 * filled in
 * This function, if is an effects plugin, can specify the interval it is to be called at through the sleepTime variable
 * if its a sound visualization plugin, this function is called at an interval of 50ms or more.
 *
 * @param frames: a pre-allocated buffer of the Frame_t structure to fill up with RGB values to show on panels.
 * Maximum size of this buffer is equal to the number of panels
 * @param nFrames: fill with the number of frames in frames
 * @param sleepTime: specify interval after which this function is called again, NULL if sound visualization plugin
 */
void getPluginFrame(Frame_t* frames, int* nFrames, int* sleepTime){
    //	static int rotationCounter = 0;
    //	if (rotationCounter == 100){
    //		int angle = 30; //the angle to rotate the layout by
    //		rotateAuroraPanels(layoutData, &angle);
    //		currentAuroraRotation += angle; //update the currentAuroraRotation by the new angle
    //		//call getFrameSlicesFromLayoutForTriangle to get the new frameSlices as per the new layout
    //		//always call freeFrameSlices before calling getFrameSlicesFromLayoutForTriangle. (safe to call with NULL pointer
    //		//getFrameSlicesFromLayoutForTriangle allocates memory, and hence is important to ensure that the memory allocated from a previous call is properly deallocated
    //		//to ensure no memory leaks occur
    //		freeFrameSlices(frameSlices);
    //		getFrameSlicesFromLayoutForTriangle(layoutData, &frameSlices, &nFrameSlices, currentAuroraRotation);
    //		rotationCounter = 0;
    //		currentAuroraRotation = currentAuroraRotation % 360;
    //	}
    ////	rotationCounter++;
    
#define SKIP_COUNT 1
    // a tiny block of code that allows the developer to skips calls of this function.
    //if this plugin wishes to run only every 150ms for instance, the skip count would be set to 2
    static int cnt = 0;
    if (cnt < SKIP_COUNT){
        cnt++;
        return;
    }
    cnt = 0;
    
    if (nColors >= 2) {
        static int barColorTimer = 0;
        static const int barColorTimerThresh = 30*1000/100;
        if (barColorTimer >= barColorTimerThresh){
            if (colorIndex >= nColors){
                colorIndex = 1;
            }
            barColor = colorPalette[colorIndex++];
            barColorTimer = 0;
        }
        barColorTimer++;
    }
    
    static const int32_t maxBarLength = 100;
    static int32_t barMarker = 0;
    static const int barMarkerInertialRelaxationStep = 45.0;
    int frameIndex = 0;
    uint16_t energy = getEnergy();
    
    af.feedFilter(energy);
    double avgEnergy = af.getAverage();
    
    static double maxEnergy = energy;
    if (avgEnergy > maxEnergy){
        maxEnergy = maxEnergy / 0.8;
    }
    else{
        maxEnergy = maxEnergy * 0.8;
    }
    
    if (maxEnergy < 512){ maxEnergy = 512;}
    int32_t barLength = (energy * maxBarLength) / (2*maxEnergy);
    
    
    RGB_t netColor;
    if (barLength > barMarker){
        barMarker = barLength;
    }
    
    //map the length of the soundbar to the Aurora Layout. In this case,
    //calculate the number of frames that are affected from the total number of frame slices
    int nFramesAffected = (barMarker*nFrameSlices)/maxBarLength;
    if (nFramesAffected > nFrameSlices){
        nFramesAffected = nFrameSlices;
    }
    
    int x_start = 450;
    int x = x_start;
    int x_step = (nFramesAffected == 0) ? 0 : x_start/nFramesAffected;
    for (int i = 0; i < nFramesAffected; i++){
        x = x - x_step;
        int x_t = (x > 255) ? 255 : x;
        //the net color is a mix between a weighted base color and bar color.
        //As the frameSlices moves towards the end of the bar, the effect of the bar color decreases
        //and the base color becomes stronger and stronger.
        //In other words the bar color fades into the base color
        netColor = (((barColor*x_t)/255) + ((baseColor*(255-x_t)))/255);
        netColor = limitRGB(netColor, 255, 0);
        for (unsigned int j = 0; j < frameSlices[i].panelIds.size(); j++){
            frames[frameIndex].panelId = frameSlices[i].panelIds[j];
            frames[frameIndex].r = netColor.R;
            frames[frameIndex].g = netColor.G;
            frames[frameIndex].b = netColor.B;
            frames[frameIndex].transTime = 1;
            frameIndex++;
        }
    }
    for (int i = nFramesAffected; i < nFrameSlices; i++){
        for (unsigned int j = 0; j < frameSlices[i].panelIds.size(); j++){
            frames[frameIndex].panelId = frameSlices[i].panelIds[j];
            frames[frameIndex].r = baseColor.R;
            frames[frameIndex].g = baseColor.G;
            frames[frameIndex].b = baseColor.B;
            frames[frameIndex].transTime = 1;
            frameIndex++;
        }
    }
    
    if (barMarker > 0){
        //the soundbar relaxes to 0 slowly, instead of jumping to the level of the sound immediately.
        //gives a more 'organic' effect.
        barMarker -= barMarkerInertialRelaxationStep;
        if (barMarker < 0){
            barMarker = 0;
        }
    }
    
    *nFrames = frameIndex;
}

/**
 * @description: called once when the plugin is being closed.
 * Do all deallocation for memory allocated in initplugin here
 */
void pluginCleanup(){
	//do deallocation here
    freeFrameSlices(frameSlices);
}
