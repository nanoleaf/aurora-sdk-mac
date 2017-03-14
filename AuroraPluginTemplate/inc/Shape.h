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
 * Shape.h
 *
 *  Created on: Mar 6, 2017
 *      Author: eski
 */

#ifndef INC_SHAPE_H_
#define INC_SHAPE_H_

#include "Point.h"

#define SHAPE_TRIANGLE 0
#define SHAPE_RHYTHM 1
#define SHAPE_SQUARE 2

class Shape {
	Shape (const Shape&) = delete;
protected:
	Point centroid;				/*a point object representing the position of the centroid of the shape*/
	int orientation;			/*orientation represents the angle in degrees that the base of the shape makes with the x-axis, the base is taken as side 1, out of the n sides*/
public:
	Point* vertices;			/*vertices of the shape, presented as an array of Point objects*/
	int nVertices;				/*number of vertices*/
	double area;				/*area of the shape*/
	int shapeType;				/*type of shape, as indicated in the #defines above*/
	static int sideLength;		/*a static const for the sideLength of the shape*/
	Shape();
	virtual ~Shape();

	/**
	 * @description: returns whether a given point is inside the shape or not
	 * @params p : the point to be tested
	 * @return : true, if inside the shape, false otherwise
	 */
	virtual bool isPointInsideShape(Point p) = 0;

	/**
	 * @description: a fucntion to update the centroid and/or the orientation of a shape. The value of vertices, is automatically
	 * calculated whenever the updateShape fucntion is called
	 *
	 * @params centroid: a pointer to a point object which carries the value that the shape object's centroid
	 * must be updated with. If NULL is supplied, the centroid object in shape will not be updated
	 * @params orientation : a pointer to an int which carries the value that the shape object's orientation
	 * must be updated with. If NULL is supplied, the orientation value in shape will not be updated
	 *
	 */
	virtual void updateShape(Point* centroid, int* orientation) = 0;

	/**
	 * getters and setters for the centroid and orientation members
	 */
	const Point& getCentroid() const;
	int getOrientation() const;
};

#endif /* INC_SHAPE_H_ */
