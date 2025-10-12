/**
 * @file ESP32DesktopInfoBoard.h
 */

#ifndef ESP32DESKTOPINFOBOARD_H
#define ESP32DESKTOPINFOBOARD_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/

#include "ESP32DesktopInfoBoard_gen.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL VARIABLES
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**
 * Initialize the component library
 */
void ESP32DesktopInfoBoard_init(const char * asset_path);

/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*ESP32DESKTOPINFOBOARD_H*/