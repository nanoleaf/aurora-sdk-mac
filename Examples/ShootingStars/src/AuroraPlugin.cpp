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
    At each beat, an illumination source will streak across the Aurora panels in a random direction.
    Several streaks may occur at the same time depending on the music and the colours are mixed.
    The colour depends on the most dominant frequecy of the sound.
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
#define ADJACENT_PANEL_DISTANCE 86.599995 // hard coded distance between adjacent panels; this ideally should be autodetected
#define TRANSITION_TIME 1  // the transition time to send to panels; set to 100ms currently

static RGB_t* paletteColours = NULL; // this is our saved pointer to the colour palette
static int nColours = 0;             // the number of colours in the palette
static LayoutData *layoutData; // this is our saved pointer to the panel layout information


// Here we store the information accociated with each light source like current
// position, velocity and colour. The information is stored in a list called sources.
typedef struct {
	float x;
	float y;
	float vx;
	float vy;
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
        printf("There are too many colours in the palette. using only the first %d\n", MAX_PALETTE_COLOURS);
        nColours = MAX_PALETTE_COLOURS;
    }

    for (int i = 0; i < nColours; i++) {
        printf("   %d %d %d\n", paletteColours[i].R, paletteColours[i].G, paletteColours[i].B);
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
    soundfeatureRequest->fft = true;
    soundfeatureRequest->nFftBins = nColours;
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
 * @description: Get a colour by interpolating in a lineaer way amongs the set of colours in the palette
 * @param colour A number between 0 and 255 that will be used to get a colour from the
 *               palette. For example, 0 will get the first colour; 255 will get the
 *               last colour and enverything in between will get an intermediate colour
 *               by interpolating within the palette.
 */
void getRGB(uint8_t colour, int *returnR, int *returnG, int *returnB)
{
	if(nColours == 0) {
	    *returnR = 0;
	    *returnG = 0;
	    *returnB = 0;
	}
	else if(nColours == 1) {
	    *returnR = paletteColours[0].R;
	    *returnG = paletteColours[0].G;
	    *returnB = paletteColours[0].B;
	}
	else {
	    float fraction = (float)colour / 256.0 * (float)(nColours - 1);
	    int idx = (int)fraction;
	    fraction = fraction - idx;
	    float R1 = paletteColours[idx].R;
	    float G1 = paletteColours[idx].G;
	    float B1 = paletteColours[idx].B;
	    float R2 = paletteColours[idx + 1].R;
	    float G2 = paletteColours[idx + 1].G;
	    float B2 = paletteColours[idx + 1].B;
	    float R = (1.0 - fraction) * R1 + fraction * R2;
	    float G = (1.0 - fraction) * G1 + fraction * G2;
	    float B = (1.0 - fraction) * B1 + fraction * B2;
	    *returnR = (int)R;
	    *returnG = (int)G;
	    *returnB = (int)B;
	}
}

/** 
  * @description: compute the distance from a point to a line
  * @param: x1, y1 and x2, y2 are two points that define the line
  *         x3, y3 is the point
  * @return: dist is the computed distance from the point to the closest point on the line
  *          u is a scalar representing how far along the line do we need to go to get near
  *          to the point
  */
void point2line(float x3, float y3, float x1, float y1, float x2, float y2, float *dist, float *u) // x3,y3 is the point
{
    float px = x2 - x1;
    float py = y2 - y1;
    float magnitude_squared = px * px + py * py;
    *u = ((x3 - x1) * px + (y3 - y1) * py) / magnitude_squared;
    float x = x1 + (*u) * px;
    float y = y1 + (*u) * py;
    float dx = x - x3;
    float dy = y - y3;
    *dist = sqrt(dx * dx + dy * dy);
}

/** 
  * @description: Adds a light source to the list of light sources. The light source will have a particular colour
  * and intensity and will move at a particular speed.
*/
void addSource(uint8_t colour, float intensity, float speed)
{
	float x;
	float y;
	float vx = 0;
	float vy = 0;
	int i;

    // we need at least two panels to do anything meaningful in here
    if(layoutData->nPanels < 2) {
        return;
    }

    // first, pick two panels at random and make sure they are not the same panel
    int n1;
    int n2;
    while(1) {
        n1 = drand48() * layoutData->nPanels;
        n2 = drand48() * layoutData->nPanels;
        if(n1 != n2) {
            break;
        }
    }
    
    // find a vector pointing from one of the panels to the other
    float x1 = layoutData->panels[n1].shape->getCentroid().x;
    float y1 = layoutData->panels[n1].shape->getCentroid().y;
    float x2 = layoutData->panels[n2].shape->getCentroid().x;
    float y2 = layoutData->panels[n2].shape->getCentroid().y;
    vx = x2 - x1;
    vy = y2 - y1;
    // normalize the vector to be length 1.0
    float normalization = 1.0 / sqrt(vx * vx + vy * vy);
    // compute a velocity vector based on the desired speed and normalize to the panel size
    vx *= normalization * speed * ADJACENT_PANEL_DISTANCE;
    vy *= normalization * speed * ADJACENT_PANEL_DISTANCE;
    
    // iterate through all panels and find the one that is closest to the edge of the
    // Aurora setup and near the line where the "shooting star" will traverse; this
    // will be the starting point of the light source
    float min_t = 1.0e20;
    int min_t_idx = -1;
    for(i = 0; i < layoutData->nPanels; i++) {
        x = layoutData->panels[i].shape->getCentroid().x;
        y = layoutData->panels[i].shape->getCentroid().y;
        float dist;
        float t;
        point2line(x, y, x1, y1, x2, y2, &dist, &t);
        dist *= ADJACENT_PANEL_DISTANCE;
        if(dist < 1.0) {
            if(t < min_t) {
                min_t = t;
                min_t_idx = i;
            }
        }
    }
    x = layoutData->panels[min_t_idx].shape->getCentroid().x;
    y = layoutData->panels[min_t_idx].shape->getCentroid().y;

    // decide in the colour of this light source and factor in the intensity to arrive at an RGB value
	int R;
	int G;
	int B;
    getRGB(colour, &R, &G, &B);
    R *= intensity;
    G *= intensity;
    B *= intensity;

    // if we have a lot of light sources already, let's bump off the oldest one
    if(nSources >= MAX_SOURCES) {
    	removeSource(0);
    }
    
    // add all the information to the list of light sources
    sources[nSources].x = x;
    sources[nSources].y = y;
    sources[nSources].vx = vx;
    sources[nSources].vy = vy;
    sources[nSources].R = (int)R;
    sources[nSources].G = (int)G;
    sources[nSources].B = (int)B;
    nSources++;
}

/**
  * @description: This function will render the colour of the given single panel given
  * the positions of all the lights in the light source list.
  */
void renderPanel(Panel *panel, int *returnR, int *returnG, int *returnB)
{
    float R = BASE_COLOUR_R;
    float G = BASE_COLOUR_G;
    float B = BASE_COLOUR_B;
    int i;

    // Iterate through all the sources
    // Depending how close the source is to the panel, we take some fraction of its colour and mix it into an
    // accumulator. Newest sources have the most weight. Old sources die away until they are gone.
    for(i = 0; i < nSources; i++) {
        float d = distance(panel->shape->getCentroid().x, panel->shape->getCentroid().y, sources[i].x, sources[i].y);
        d = d / ADJACENT_PANEL_DISTANCE;
        float d2 = d * d;
        float factor = 1.0 / (d2 * 1.5 + 1.0); // determines how much of the source's colour we mix in (depends on distance)
                                               // the formula is not based on physics, it is fudged to get a good effect
                                               // the formula yields a number between 0 and 1
        R = R * (1.0 - factor) + sources[i].R * factor;
        G = G * (1.0 - factor) + sources[i].G * factor;
        B = B * (1.0 - factor) + sources[i].B * factor;
    }
    *returnR = (int)R;
    *returnG = (int)G;
    *returnB = (int)B;
}

/**
  * Move the positions of all the light sources based on their velocities. If any particular
  * light source has moved far from the origin then it will be removed from the light source list.
  */
void propogateSources(void)
{
	int i;

    for(i = 0; i < nSources; i++) {
    	sources[i].x += sources[i].vx;
    	sources[i].y += sources[i].vy;
        float d = distance(0.0, 0.0, sources[i].x, sources[i].y);
        if(d > 20.0 * ADJACENT_PANEL_DISTANCE) {
            removeSource(i);
            i--;
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
    int16_t beat_detected = 0;
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
	uint32_t soundPower = 0;
	int R;
	int G;
	int B;
    int i;

//    printf("Energy: %hd   ", soundFeature->energy);
    
//    for(i = 0; i < soundFeature->nFFTBins; i++) {
//        printf("%hhu ", soundFeature->fftBins[i]);
//    }
//    printf("\n");


    // Compute the sound power (or volume)
	for(i = 0; i < soundFeature->nFFTBins; i++) {
		soundPower += soundFeature->fftBins[i];
	}
	soundPower /= soundFeature->nFFTBins;
    soundPower *= 10; // fudge factor to make the beat detection work well but not be too sensitive
//    printf("Sound power: %u\n", soundPower);

    uint8_t beat_detected = beat_detector(soundPower);

    if(beat_detected) {
        // now that a beat has been detected, figure out what frequency is strongest
    	int maxBin = 0;
    	int maxBinIndex = 0;
		for(i = 0; i < soundFeature->nFFTBins; i++) {
			if(soundFeature->fftBins[i] > maxBin) {
				maxBin = soundFeature->fftBins[i];
				maxBinIndex = i;
			}
		}

        // normalize to a number between 0 and 255
		int colour = maxBinIndex * 255 / (soundFeature->nFFTBins - 1);
        float speed = 0.5;
        float intensity = 1.0;
        // add a new light source for each beat detected
        addSource(colour, intensity, speed);
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

    // move all the light sources so they are ready for the next frame
    propogateSources();

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
