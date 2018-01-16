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
 * DataManger.h
 *
 *  Created on: Feb 13, 2017
 *      Author: eski
 */

#ifndef INC_DATAMANAGER_H_
#define INC_DATAMANAGER_H_

#include "ColorUtils.h"
#include "LayoutProcessingUtils.h"

/*
 * @description: get the color palette
 * @params palette: a pointer that will point to a statically allocated buffer holding the colorPalette in it
 * Do NOT free this buffer. Data Manager will handle this for you
 * @params nColors: a pointer that will be filled with the number of colors in the palette
 */
void getColorPalette(RGB_t** palette, int* nColors);

/**
 * @description: get the layoutData
 * @return: a pointer to a statically allocated object of LayoutData
 * Do NOT free this object. Data Manager will handle this for you
 */
LayoutData* getLayoutData();


#endif /* INC_DATAMANAGER_H_ */
