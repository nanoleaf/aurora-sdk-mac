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


#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include "AuroraPlugin.h"
#include "LayoutProcessingUtils.h"
#include "ColorUtils.h"
#include "DataManager.h"
#include "PluginFeatures.h"
#include "Logger.h"

#ifdef __cplusplus
extern "C" {
#endif

	void initPlugin();
	void getPluginFrame(Frame_t* frames, int* nFrames, int* sleepTime);
	void pluginCleanup();

#ifdef __cplusplus
}
#endif

#define MAX_SOURCES 10           // this is the maximum number of sources that can propogate at the same time
#define BASE_COLOUR_R 0         // these three settings defined the background colour; set to black
#define BASE_COLOUR_G 0
#define BASE_COLOUR_B 0
#define ADJACENT_PANEL_DISTANCE 86.599995 // hard coded distance between adjacent panels; this ideally should be autodetected
#define TRANSITION_TIME 1       // the transition time to send to panels; set to 100ms currently
#define N_FFT_BINS 32     // number of fft bins to request in the sound feature and beat detector
#define BUBBLE_RADIUS 0.2       // the radius of the bubbles the flow across the Aurora

static RGB_t* paletteColours = NULL; // this is our saved pointer to the colour palette
static int nColours = 0;             // the number of colours in the palette
static LayoutData *layoutData;       // this is our saved pointer to the panel layout information

#define MAX_START_POINTS 30
static float startX[MAX_START_POINTS];
static float startY[MAX_START_POINTS];
static int nStartPoints = 0;

// Here we store the information accociated with each light source like current
// position, velocity and colour. The information is stored in a list called sources.
typedef struct {
    float x;
    float y;
    float vx;
    float vy;
    float radius;
    int R;
    int G;
    int B;
} source_t;
static source_t sources[MAX_SOURCES];
static int nSources = 0;

/** Compute cartesian distance between two points */
float distance(float x1, float y1, float x2, float y2)
{
    float dx = x2 - x1;
    float dy = y2 - y1;
    return sqrt(dx * dx + dy * dy);
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


/** This function will analyse the layout and determine some points on the layout where the
  * bubbles will start when first placed on tha canvas
  */
void defineStartPoints(void)
{
  float x;
  float y;
  int i;
    bool found;
    // we need at least one panel to do anything meaningful in here
    if(layoutData->nPanels < 1) {
        return;
    }

    // iterate through all panels
    for (int n1 = 0; n1 < layoutData->nPanels; n1++) {
        // find a vector pointing from the chosen panel upward
        float x1 = layoutData->panels[n1].shape->getCentroid().x;
        float y1 = layoutData->panels[n1].shape->getCentroid().y;
        float x2 = x1;
        float y2 = 1.0;
        PRINTLOG("Panel coords: %f %f\n", x1, y1);

        // iterate through all panels and find the one that is closest to the bottom of the
        // Aurora setup and near the line where the "bubble" will float up; this
        // will help define the starting point of the light source
        float min_t = -1.0e20;
        int min_t_idx = -1;
        for(i = 0; i < layoutData->nPanels; i++) {
            x = layoutData->panels[i].shape->getCentroid().x;
            y = layoutData->panels[i].shape->getCentroid().y;
            float dist;
            float t;
            point2line(x, y, x1, y1, x2, y2, &dist, &t);
            dist *= ADJACENT_PANEL_DISTANCE;
            if(dist < 1.0) {
                if(t > min_t) {
                    min_t = t;
                    min_t_idx = i;
                }
            }
        }
        x = layoutData->panels[min_t_idx].shape->getCentroid().x;
        y = layoutData->panels[min_t_idx].shape->getCentroid().y;

        PRINTLOG("Candidate start point at: %f %f\n", x, y);

        // search to see if we have this start point in our list yet
        found = false;
        for(i = 0; i < nStartPoints; i++)
        {
            if((x == startX[i]) && (y == startY[i])) {
                found = true;
                break;
            }
        }
        if(!found) {
            PRINTLOG("Adding this unique start point\n");
            startX[nStartPoints] = x;
            startY[nStartPoints] = y;
            nStartPoints++;
            if(nStartPoints >= MAX_START_POINTS) {
                break;
            }
        }
        else {
            PRINTLOG("This start point has been previously found. Not adding it.\n");
        }
    }
}
/**
 * @description: Initialize the plugin. Called once, when the plugin is loaded.
 * This function can be used to enable rhythm or advanced features,
 * e.g., to enable energy feature, simply call enableEnergy()
 * It can also be used to load the LayoutData and the colorPalette from the DataManager.
 * Any allocation, if done here, should be deallocated in the plugin cleanup function
 *
 */
void initPlugin() {
    enableEnergy();
    enableFft(N_FFT_BINS);
    enableBeatFeatures();
    getColorPalette(&paletteColours, &nColours);
    PRINTLOG("The palette has %d colours:\n", nColours);

    for (int i = 0; i < nColours; i++) {
        PRINTLOG("   %d %d %d\n", paletteColours[i].R, paletteColours[i].G, paletteColours[i].B);
    }
    
    layoutData = getLayoutData(); // grab the layour data and store a pointer to it for later use
    
    PRINTLOG("The layout has %d panels:\n", layoutData->nPanels);
    for (int i = 0; i < layoutData->nPanels; i++) {
        PRINTLOG("   Id: %d   X, Y: %lf, %lf\n", layoutData->panels[i].panelId,
        layoutData->panels[i].shape->getCentroid().x, layoutData->panels[i].shape->getCentroid().y);
    }
    defineStartPoints();
}

/** Removes a light source from the list of light sources */
void removeSource(int idx)
{
  memmove(sources + idx, sources + idx + 1, sizeof(source_t) * (nSources - idx - 1));
  nSources--;
}

/**
 * @description: Get a colour by interpolating in a linear way amongs the set of colours in the palette
 * @param colour The colour we want between 0 and nColours - 1. We interpolate in the palette to come up
 *               with a final colour somewhere in the palettes spectrum.
 */
void getRGB(float colour, int *returnR, int *returnG, int *returnB)
{
    float R;
    float G;
    float B;
    
  if(nColours == 0) {
      *returnR = 128; // in the case of no palette, use half white as default
      *returnG = 128;
      *returnB = 128;
  }
  else if(nColours == 1) {
      *returnR = paletteColours[0].R;
      *returnG = paletteColours[0].G;
      *returnB = paletteColours[0].B;
  }
  else {
      int idx = (int)colour;
      float fraction = colour - (float)idx;

        if(colour <= 0) {
            R = paletteColours[0].R;
            G = paletteColours[0].G;
            B = paletteColours[0].B;
        }
        else if(idx < nColours - 1) {
            float R1 = paletteColours[idx].R;
            float G1 = paletteColours[idx].G;
            float B1 = paletteColours[idx].B;
            float R2 = paletteColours[idx + 1].R;
            float G2 = paletteColours[idx + 1].G;
            float B2 = paletteColours[idx + 1].B;
            R = (1.0 - fraction) * R1 + fraction * R2;
            G = (1.0 - fraction) * G1 + fraction * G2;
            B = (1.0 - fraction) * B1 + fraction * B2;
        }
        else {
            R = paletteColours[nColours - 1].R;
            G = paletteColours[nColours - 1].G;
            B = paletteColours[nColours - 1].B;
        }
      *returnR = (int)R;
      *returnG = (int)G;
      *returnB = (int)B;
  }
}

/** 
  * @description: Adds a light source to the list of light sources. The light source will have a particular colour
  * and intensity and will move at a particular speed.
*/
void addSource(float colour, float intensity, float speed, float radius)
{
  float x;
  float y;
  float vx = 0;
  float vy = 0;
  int i;

    // we need at least one start point to do anything meaningful in here
    if(nStartPoints < 1) {
        return;
    }

    // pick a random start point
    i = drand48() * nStartPoints;
    x = startX[i];
    y = startY[i] - radius * 2 * ADJACENT_PANEL_DISTANCE; // we want to start a bit lower because it will scroll onto the canvas
    
    vx = 0.0;
    vy = speed * ADJACENT_PANEL_DISTANCE;
    
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
    sources[nSources].radius = radius;
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
        d = d - sources[i].radius;
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
    int R;
    int G;
    int B;
    int i;
    static int maxBinIndexSum = 0;
    static int n = 0;

    // figure out what frequency is strongest
    int maxBin = 0;
    int maxBinIndex = 0;
    uint8_t * fftBins = getFftBins();
    for(i = 0; i < N_FFT_BINS; i++) {
        if(fftBins[i] > maxBin) {
            maxBin = fftBins[i];
            maxBinIndex = i;
        }
    }
    maxBinIndexSum += maxBinIndex;
    n++;

    // When a beat is detected, we will choose a colour and display it
    // The colour depends on the strongest frequencies that have been measured since the last beat
    // The speed dependes on the current tempo (or bpm)
    // The first palette colour is reserved for onsets and the rest are used for beats
    if(getIsBeat()) {
        maxBinIndex = maxBinIndexSum / n;
        maxBinIndexSum = 0;
        n = 0;
        int colour = maxBinIndex * nColours / (N_FFT_BINS / 4) + 1;
//        float speed = (float)bd.getTempo() / 250;
        float speed = 0.4; // Disable dependence on tempo for now until beat detector is improved
        if(speed < 0.2) {
            speed = 0.2;
        }
        float intensity = 1.0;
        // add a new light source for each beat detected
        addSource(colour, intensity, speed, BUBBLE_RADIUS);
    }
    else if(getIsOnset()) {   // We will also display something for onsets but only at 30% intensity
        addSource(0.0, 0.3, 0.3, BUBBLE_RADIUS);
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
void pluginCleanup(){
	//do deallocation here
}
