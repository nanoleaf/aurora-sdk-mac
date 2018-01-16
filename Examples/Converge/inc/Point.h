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
 * Point.h
 *
 *  Created on: Feb 13, 2017
 *      Author: eski
 */

#ifndef INC_POINT_H_
#define INC_POINT_H_


#include <string>

typedef double degrees;
typedef double radians;

class Point{
public:
	double x, y;

	Point();
	Point(double _x, double _y);
	Point operator+(Point p2);
	Point operator-(Point p2);
	void ToInt(int* _x, int* _y);
	Point rotate(degrees angle);
	std::string ToString();
	static double distance(Point P1, Point P2);
};

double degs2rads(double degs);


#endif /* INC_POINT_H_ */
