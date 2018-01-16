/*
 * PluginOptionsManager.h
 *
 *  Created on: Jul 25, 2017
 *      Author: eski
 */

#ifndef INC_PLUGINOPTIONSMANAGER_H_
#define INC_PLUGINOPTIONSMANAGER_H_

#define PLUGIN_OPTIONS_ERROR_OPTION_NO_EXIST -10
#define PLUGIN_OPTIONS_ERROR_WRONG_OPTION_TYPE -11

#include <string>


int getOptionValue(const char* name, int& value);
int getOptionValue(const char* name, bool& value);
int getOptionValue(const char* name, double& value);
int getOptionValue(const char* name, std::string & value);


#endif /* INC_PLUGINOPTIONSMANAGER_H_ */
