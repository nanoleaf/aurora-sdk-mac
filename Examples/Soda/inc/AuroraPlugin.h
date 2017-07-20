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
 * AuroraPlugin.h
 *
 *  Created on: Feb 12, 2017
 *      Author: eski
 */

#ifndef SRC_AURORAPLUGIN_H_
#define SRC_AURORAPLUGIN_H_

#include <stdint.h>

struct Frame_t {
	int panelId; 		/*the panelId that this frame element targets*/
	int r, g, b;		/*the rgb color that it must transition to*/
	int transTime;		/*time taken to transition to specified color - in multiples of 100ms*/
};

#endif /* SRC_AURORAPLUGIN_H_ */
