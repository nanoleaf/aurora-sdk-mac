/*
 * logger.h
 *
 *  Created on: May 10, 2017
 *      Author: leizhang
 */

#ifndef INC_LOGGER_H_
#define INC_LOGGER_H_

#define LOGGING_ENABLED

#ifdef LOGGING_ENABLED
#define PRINTLOG(format, ...) printf(format,  ##__VA_ARGS__)
#else
#define PRINTLOG(format, ...) {}
#endif

#endif /* INC_LOGGER_H_ */
