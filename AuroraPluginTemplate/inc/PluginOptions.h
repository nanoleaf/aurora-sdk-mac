#ifndef PLUGIN_OPTIONS_H
#define PLUGIN_OPTIONS_H

const char* pluginOptionsJsonString = "{\"options\": [{\"defaultValue\": 15, \"minValue\": 1, \"type\": \"int\", \"name\": \"transTime\", \"maxValue\": 600}]}";

#ifdef __cplusplus
extern "C" {
#endif

const char* getPluginOptionsJsonString(){
	return pluginOptionsJsonString;
}

#ifdef __cplusplus
}
#endif

#endif
