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

   AuroraPlugin.cpp
 
    Created on: Mar 22, 2017
        Author: Tom Rodinger
    Description:
    At each beat, an illumination source will be placed at full brightness onto a panel. Over time,
    the colour will diffuse outwards and becomeless bright until it disappears. Several beats can cause
    multiple diffusing light sources all blending their colours together in a fluid way.
 */


#include "AuroraPlugin.h"
#include "LayoutProcessingUtils.h"
#include "ColorUtils.h"
#include "DataManager.h"
#include <stdlib.h>
#include <math.h>

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

#define MAX_PALETTE_COLOURS 16  // if more colours then this, we will use just the first this many
#define MAX_SOURCES 10          // this is the maximum number of sources that can propogate at the same time
#define BASE_COLOUR_R 0         // these three settings defined the background colour; set to black
#define BASE_COLOUR_G 0
#define BASE_COLOUR_B 0
#define TRANSITION_TIME 1  // the transition time to send to panels; set to 100ms currently
#define MIN_SIMULTANEOUS_COLOURS 2 // keep this many colours or more active at the ame time if possible
#define MAX_SOURCES 10             // this is the maximum number of diffusing sources that can be present at the same time
#define FRACTION_COLOUR_TO_KEEP 0.05 // always keep this fraction of a colour so that the Aurora shows some
                                     // colour even if there were no beats for a while
#define PROPOGATION_FRAMES 40  // after this many frames, a diffusing colour will go away completely


static RGB_t* paletteColours = NULL; // this is our saved pointer to the colour palette
static int nColours = 0;             // the number of colours in the palette
static LayoutData *layoutData; // this is our saved pointer to the panel layout information


// Here we store the information accociated with each light source like current
// position, age, and colour. The information is stored in a list called sources.
typedef struct {
	float x;
	float y;
	int diffusion_age; // starts from zero and increments for each frame
	int R;
	int G;
	int B;
} source_t;
static source_t sources[MAX_SOURCES];
static int nSources = 0;


/**
 * @description: Initialize the plugin. Called once, when the plugin is loaded.
 * This function can be used to load the LayoutData and the colorPalette from the DataManager.
 * Any allocation, if done here, should be deallocated in the plugin cleanup function
 *
 * @param isSoundPlugin: Setting this flag will indicate that it is a sound plugin, and accordingly
 * sound data will be passed in. If not set, the plugin will be considered an effects plugin
 *
 */
void initPlugin(bool* isSoundPlugin) {
	*isSoundPlugin = true;

	getColorPalette(&paletteColours, &nColours);  // grab the palette colours and store a pointer to them for later use
	printf("The palette has %d colours:\n", nColours);
    if(nColours > MAX_PALETTE_COLOURS) {
        printf("There are too many colours in the palette. Using only the first %d of them.\n", MAX_PALETTE_COLOURS);
        nColours = MAX_PALETTE_COLOURS;
    }

    for (int i = 0; i < nColours; i++) {
        printf("   RGB = (%d, %d, %d)\n", paletteColours[i].R, paletteColours[i].G, paletteColours[i].B);
    }
    
    layoutData = getLayoutData(); // grab the layour data and store a pointer to it for later use
    
	printf("The layout has %d panels:\n", layoutData->nPanels);
    for (int i = 0; i < layoutData->nPanels; i++) {
        printf("   Id: %d   X, Y: %lf, %lf\n", layoutData->panels[i].panelId,
               layoutData->panels[i].shape->getCentroid().x, layoutData->panels[i].shape->getCentroid().y);
    }

}

/**
 * @description: This function is called only if the isSoundPLugin Flag is set in the initPlugin function.
 * Here the plugin is allowed to choose the soundFeatures it needs to run.
 *
 * @param soundFeatureRequest A pointer to a SoundFeatureRequest_t instance
 */
void selectSoundFeature(SoundFeatureRequest_t* soundfeatureRequest) {
    soundfeatureRequest->energy = true;
    soundfeatureRequest->fft = false;
}

/** Removes a light source from the list of light sources */
void removeSource(int idx)
{
	memmove(sources + idx, sources + idx + 1, sizeof(source_t) * (nSources - idx - 1));
	nSources--;
}

/** Compute cartesian distance between two points */
float distance(float x1, float y1, float x2, float y2)
{
    float dx = x2 - x1;
    float dy = y2 - y1;
    return sqrt(dx * dx + dy * dy);
}

/**
  * @description: Adds a light source to the list of light sources. The light source will have a random colour
  * and be centred on a randomly chosen panel
*/
void addSource(void)
{
    int r = (int)(drand48() * layoutData->nPanels);
    float x = layoutData->panels[r].shape->getCentroid().x;
    float y = layoutData->panels[r].shape->getCentroid().y;
    r = (int)(drand48() * nColours);
    int R = paletteColours[r].R;
    int G = paletteColours[r].G;
    int B = paletteColours[r].B;

    // if we have too many light sources then remove the oldest one
    if(nSources >= MAX_SOURCES) {
    	removeSource(0);
    }
    
    // save the information in the list
    sources[nSources].x = x;
    sources[nSources].y = y;
    sources[nSources].diffusion_age = 0;
    sources[nSources].R = R;
    sources[nSources].G = G;
    sources[nSources].B = B;
    nSources++;
}

/**
  * @description: This function will render the colour of the given single panel given
  * the positions of all the lights in the light source list.
  */
void renderPanel(Panel *panel, int *returnR, int *returnG, int *returnB)
{
    float R = 0.0;
    float G = 0.0;
    float B = 0.0;
    int i;

    for(i = 0; i < nSources; i++) {
        // Compute a factor that determines how much a light source contributes to this panel's colour.
        // This factor depends on how far the light source is from the panel and how diffuse it has become.
        int diffusion_age = sources[i].diffusion_age;
        float d = distance(panel->shape->getCentroid().x, panel->shape->getCentroid().y, sources[i].x, sources[i].y);
        d = d * 0.015;
        d = d - (float)(diffusion_age * 0.2);
        if(d < 0.0) {
            d = 0.0;
        }
        float factor = 1.0 / (d * 2.0 + 1.0);
        if(factor > 1.0) {
            factor = 1.0;
        }
        if(factor < 0.0) {
            factor = 0.0;
        }
        if(diffusion_age >= PROPOGATION_FRAMES) {
            factor = 0.0;
        }
        else {
            factor = factor * (1.0 - (float)diffusion_age / (float)PROPOGATION_FRAMES);
        }
        if(factor < FRACTION_COLOUR_TO_KEEP) {
            factor = FRACTION_COLOUR_TO_KEEP;
        }
        // mix this colour into the accumulator
        R = R * (1.0 - factor) + sources[i].R * factor;
        G = G * (1.0 - factor) + sources[i].G * factor;
        B = B * (1.0 - factor) + sources[i].B * factor;
    }

    // prepare the values we will be returning
    *returnR = (int)R;
    *returnG = (int)G;
    *returnB = (int)B;

    // let's not exceed 255 for any of the channels
    if(*returnR > 255) {
        *returnR = 255;
    }
    if(*returnG > 255) {
        *returnG = 255;
    }
    if(*returnB > 255) {
        *returnB = 255;
    }
}

/**
  * Increase the diffusion age number of each light source. In the rendering routine, this has the
  * effect of increasing the radious of the light source and decreasing its brightness. If any
  * light source gets too old, it will be removed from the list.
  */
void diffuseSources(void)
{
	int i;

    for(i = 0; i < nSources; i++) {
        sources[i].diffusion_age++;
    }
    if(nSources > MIN_SIMULTANEOUS_COLOURS) {
    	for(i = 0; i < nSources; i++) {
            if(sources[i].diffusion_age > PROPOGATION_FRAMES) {
                removeSource(i);
				i--;
            }
        }
    }
}

/**
  * A simple algorithm to detect beats. It finds a strong signal after a period of quietness.
  * Actually, it doesn't detect just beats. For example, classical music often doesn't have
  * strong beats but it has strong instrumental sections. Those would also get detected. 
  */
int16_t beat_detector(uint32_t soundPower)
{
	static uint32_t latest_minimum = 2000000000;
    int8_t beat_detected = 0;
    if(soundPower < latest_minimum) {
        latest_minimum = soundPower;
    }
    else if(latest_minimum > 0) {
        latest_minimum--;
    }
    if(soundPower > latest_minimum + 70) {
        latest_minimum = soundPower;
        beat_detected = 1;
    }
    return beat_detected;
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
void getPluginFrame(SoundFeature_t* soundFeature, Frame_t* frames, int* nFrames, int* sleepTime) {
//	uint32_t soundPower = 0;
	int R;
	int G;
	int B;
    int i;

//    printf("Energy: %hd   ", soundFeature->energy);
    
    uint8_t beat_detected = beat_detector(soundFeature->energy);

    if(beat_detected) {
        // add a new light source for each beat detected
        addSource();
    }

    // iterate through all the panels and render each one
    for(i = 0; i < layoutData->nPanels; i++) {
        renderPanel(&layoutData->panels[i], &R, &G, &B);
        frames[i].panelId = layoutData->panels[i].panelId;
        frames[i].r = R;
        frames[i].g = G;
        frames[i].b = B;
        frames[i].transTime = TRANSITION_TIME;
	}

    // diffuse all the light sources so they are ready for the next frame
    diffuseSources();

    // this algorithm renders every panel at every frame
    *nFrames = layoutData->nPanels;
}

/**
 * @description: called once when the plugin is being closed.
 * Do all deallocation for memory allocated in initplugin here
 */
void pluginCleanup() {
	// do deallocation here, but there is nothing to deallocate
}
