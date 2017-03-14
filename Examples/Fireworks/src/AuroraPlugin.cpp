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
 *  The fireworks plugin, essentially randomly lights up panels. The probability of a panel lighting
 *  up is controlled by the energy of the sound from the sound module. The energy itself is calculated from the fft after
 *  applying a filter to it to boost the low frequencies
 */




#include "AuroraPlugin.h"
#include "LayoutProcessingUtils.h"
#include "ColorUtils.h"
#include "DataManager.h"
#include <math.h>
#include <stdlib.h>

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

RGB_t* colorPalette = NULL;
int nColors = 0;
RGB_t bkGrndColor;
RGB_t* fireworkColors = NULL;
int nFireWorkColors = 0;

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
	getColorPalette(&colorPalette, &nColors);
	printf(" --- nColors %d ---\n", nColors);

	if (nColors == 0){
		bkGrndColor = (RGB_t){0, 0, 0};
	}
	else {
		bkGrndColor = colorPalette[0];
	}

	if (nColors - 1 > 0){
		fireworkColors = new RGB_t[nColors - 1];
		nFireWorkColors = nColors - 1;
		for (int i = 1; i < nColors; i++){
			fireworkColors[i-1] = colorPalette[i];
			printf("fireworkColor [%d]: %d %d %d\n", i-1, fireworkColors[i-1].R, fireworkColors[i-1].G, fireworkColors[i-1].B);
		}
	}
	else{
		fireworkColors = new RGB_t[3];
		nFireWorkColors = 3;
		fireworkColors[0] = (RGB_t){255, 0, 0};
		fireworkColors[1] = (RGB_t){0, 255, 0};
		fireworkColors[2] = (RGB_t){0, 0, 255};
	}
	*isSoundPlugin = true;
}

/**
 * @description: This function is called only if the isSoundPLugin Flag is set in the initPlugin function.
 * Here the plugin is allowed to choose the soundFeatures it needs to run.
 *
 * @param soundFeatureRequest A pointer to a SoundFeatureRequest_t instance
 */
void selectSoundFeature(SoundFeatureRequest_t* soundfeatureRequest){
	soundfeatureRequest->energy = false;
	soundfeatureRequest->fft = true;
	soundfeatureRequest->nFftBins = 16;
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
#define SKIP_COUNT 3
	// a tiny block of code that allows the developer to skips calls of this function.
	//if this plugin wishes to run only every 150ms for instance, the skip count would be set to 3
	static int cnt = 0;
	if (cnt < SKIP_COUNT){
		cnt++;
		return;
	}
	cnt = 0;


	uint16_t energy = 0;
	static float bkGrndHue = 0;
	static float bkGrndHueStep = 0.5;
	int frameIndex = 0;
	LayoutData* layoutData = getLayoutData();
	for (int i = 0; i < soundFeature->nFFTBins; i++){
		//apply an exponential decay filter and sum up fftbins to get energy. This effectively boosts the bass frequencies
		energy += (uint16_t)((double)soundFeature->fftBins[i] * pow(3.5, 1.0/(i+1)));
	}

	//perform some scaling, and increase probability exponentially as the energy increases.
	//the louder the sound, the even more probability of panels changing colors
	uint32_t probability = ((int)(pow((energy>>5), 1.5)))>>3;

	//the background color's hue keeps rotating through 0-360 degrees.
	//this leads to the background color changing
	bkGrndHue += bkGrndHueStep;
	bkGrndHue = (bkGrndHue >= 360) ? 0 : bkGrndHue;

	HSVtoRGB((HSV_t){bkGrndHue, 100, 30}, &bkGrndColor);
	for (int i = 0; i < layoutData->nPanels; i++){
		int randomNumber = rand()%128;
		frames[frameIndex].panelId = layoutData->panels[i].panelId;
		//if the random number is less than probability then light up a panel,
		//otherwise relax the panel to the background color
		////higher the probability number, higher the chance of the if block getting executed
		if (randomNumber < probability){
			RGB_t droppedColor = fireworkColors[rand()%nFireWorkColors];
			frames[frameIndex].r = droppedColor.R;
			frames[frameIndex].g = droppedColor.G;
			frames[frameIndex].b = droppedColor.B;
			frames[frameIndex].transTime = 0;
		}
		else {
			frames[frameIndex].r = bkGrndColor.R;
			frames[frameIndex].g = bkGrndColor.G;
			frames[frameIndex].b = bkGrndColor.B;
			frames[frameIndex].transTime = 3;
		}
		frameIndex++;
	}
	*nFrames = frameIndex;

}

/**
 * @description: called once when the plugin is being closed.
 * Do all deallocation for memory allocated in initplugin here
 */
void pluginCleanup(){
	//do deallocation here
	if (fireworkColors){
		delete [] fireworkColors;
	}
}
