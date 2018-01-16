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
 * SoundUtils.h
 *
 *  Created on: Feb 23, 2017
 *      Author: eski
 */

#ifndef INC_SOUNDUTILS_H_
#define INC_SOUNDUTILS_H_

#include <stdint.h>

/**
 * @description: Shows the fft on the screen vertically with the amplitude of each bin represented
 * as a horizontal row of '*'s
 *
 * @params fft: the fft to be visualized
 * @params nFftBins: number of bins in the ffts
 */
void visualizeFft(uint8_t* fft, int nFftBins);


#endif /* INC_SOUNDUTILS_H_ */
