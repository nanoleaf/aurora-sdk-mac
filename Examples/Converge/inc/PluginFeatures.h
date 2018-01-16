/*
 * AdvancedFeatures.h
 *
 *  Created on: Jul 5, 2017
 *      Author: leizhang
 */

#ifndef INC_PLUGINFEATURES_H_
#define INC_PLUGINFEATURES_H_

#include <stdbool.h>
#include <stdint.h>

/* ----------------------------------
 * RHYTHM FEATURE FUNCTIONS
 * ----------------------------------
 */
void enableEnergy(void);
void enableFft(uint16_t nFftBins);
void enableDistance(void);
void enableSpeed(void);			// get motion speed in m/s
uint16_t getEnergy(void);
uint8_t *getFftBins(void);
uint8_t getDistance(void);
uint8_t getSpeed(void);

/* ----------------------------------
 * BEAT FEATURE FUNCTIONS
 * ----------------------------------
 */
void enableBeatFeatures(void);	// enable beat features
bool getIsBeat(void);			// get beat flag
bool getIsOnset(void);			// get onset flag
float getTempo(void);			// get tempo in beats-per-minute (bpm)

/* -----------------------------------
 * MORE ADVANCED FEATURES ...
 * -----------------------------------
 */

#endif /* INC_PLUGINFEATURES_H_ */
