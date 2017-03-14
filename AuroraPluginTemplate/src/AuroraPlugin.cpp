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

}

/**
 * @description: called once when the plugin is being closed.
 * Do all deallocation for memory allocated in initplugin here
 */
void pluginCleanup(){
	//do deallocation here
}
