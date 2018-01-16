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
 * LayoutProcessingUtilities.h
 *
 *  Created on: Feb 13, 2017
 *      Author: eski
 */

#ifndef UTILITIES_LAYOUTPROCESSINGUTILITIES_H_
#define UTILITIES_LAYOUTPROCESSINGUTILITIES_H_

#include "Point.h"
#include <vector>
#include "Shape.h"


/**
 * An Element of the layout Data Array
 */

struct Panel{
	int panelId;	 	/*the panelId of the panel*/
	Shape* shape;
	Panel (const Panel&) = delete;
	Panel(){
		panelId = -1;
		shape = NULL;
	}
	~Panel(){
		if (shape){
			delete shape;
		}
	}
};

struct LayoutData{
	int nPanels; 					/*number of panels in the layout*/
	Panel* panels; 					/*statically allocated buffer containing the layoutData of the panels*/
	int globalOrientation; 			/*orientation as set by the user*/
	Point layoutGeometricCenter;
	LayoutData(const LayoutData&) = delete;
	LayoutData(){
		nPanels = 0;
		panels = NULL;
		globalOrientation = 0;
	}
	~LayoutData(){
		if (panels){
			delete [] panels;
			panels = NULL;
		}
	}
};

struct FrameSlice_t {
	std::vector<int> panelIds;
};

/**
 * Helper function
 */
void parseLayoutData(int* layoutDataByteStream, int nPanels, LayoutData** layoutData);

/*
 * @description: Utility function to geometrically rotate the layout through a specified angle. the angle is snapped to the
 * closest multiple of 30 degrees
 * @params layoutData : the layout to rotate
 * @params angle_degrees: the angle to rotate through
 */
int rotateAuroraPanels(LayoutData* layoutData, int *angle_degrees);

/**
 * @description: Utility function that helps breakdown the layout into frame slices, which aligns the layout into a grid. This helps in creating effects
 * @params LayoutData: the layoutData to process
 * @params frameSlices: A buffer that is dynamically allocated internally and 'splits' the layout into 'FrameSlices' that is aligns the layout into a grid
 * The grid spacing is 0.5*sideLength if orientations are multiples of 60 degrees and 0.288*sideLength if its not a multiple of 60 degrees
 */
void getFrameSlicesFromLayoutForTriangle(LayoutData* layoutData, FrameSlice_t** frameSlices, int* nFrameSlicesint, int totalAuroraRotation);

/**
 * @description: test whether point p is inside Panel given by panel.
 * @params layoutDataElement: the centroid of the shape that the point is inside
 * @params p : the point to be tested
 * @return : true if inside, else false
 */
bool isPointInsidePanel(Panel* panel, Point p);

/**
 * @description: returns the panelId of the panel the point p is inside.
 * If not inside any panel, the value returned is -1
 * the function loops over all the panels, so excessive usage of this API might hit efficiency
 * @params layoutData : a pointer to the LayoutData object
 * @params p : the point to test and check if within any panel
 * @return : the panelId of the panel that the point is within, -1 if not inside any panel
 */
int pointInsideWhichPanel(LayoutData* layoutData, Point p);

/**
 * Internal Helper function
 */
void freeLayoutData(LayoutData* layoutData);

/**
 * @description: De-allocate frameslices allocated by getFramesFrom Layout
 */
void freeFrameSlices(FrameSlice_t* frameSlices);

#endif /* UTILITIES_LAYOUTPROCESSINGUTILITIES_H_ */
