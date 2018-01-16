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
 * RGBUtils.h
 *
 *  Created on: Feb 12, 2017
 *      Author: eski
 */

#ifndef UTILITIES_RGBUTILS_H_
#define UTILITIES_RGBUTILS_H_

struct RGB_t{
	int R, G, B;
};

struct HSV_t {
	int H, S, V;
};

/**
 * @description: Helper Function
 */
void parseColor(int* colorByteStream, int nColors, RGB_t** rgb);

/**
 * @description: Convert Color from HSV colorspace to RGB colorspace
 * @params HSV: color to convert from ...
 * @params RGB: ... color to convert to
 */
void HSVtoRGB(HSV_t hsv, RGB_t* rgb);

/**
 * @description: Convert Color from RGB colorspace to HSV colorspace
 * @params RGB: color to convert from ...
 * @params HSV: ... color to convert to
 */
void RGBtoHSV(RGB_t rgb, HSV_t* hsv);

/**
 * helper function
 */
void freeColor(RGB_t* rgb);

/**
 * Operator overloads to help with RGB manipulation
 */
RGB_t operator+ (const RGB_t& l, const RGB_t& r);
RGB_t operator- (const RGB_t& l, const RGB_t& r);
RGB_t operator* (const RGB_t& l, int m);
RGB_t operator* (int m, const RGB_t& l);
RGB_t operator/ (const RGB_t& l, float d);
RGB_t limitRGB(const RGB_t& c, int max, int min);


#endif /* UTILITIES_RGBUTILS_H_ */
