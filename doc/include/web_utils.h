/* Copyright (C) 2025 Ricardo Guzman - CA2RXU
 * 
 * This file is part of LoRa APRS Tracker.
 * 
 * LoRa APRS Tracker is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or 
 * (at your option) any later version.
 * 
 * LoRa APRS Tracker is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with LoRa APRS Tracker. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef WEB_UTILS_H_
#define WEB_UTILS_H_

#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include <Arduino.h>
#include <SPIFFS.h>
#include <WiFi.h>


namespace WEB_Utils {

    void handleNotFound(AsyncWebServerRequest *request);
    void handleStatus(AsyncWebServerRequest *request);
    void handleHome(AsyncWebServerRequest *request);
    void handleStyle(AsyncWebServerRequest *request);
    void handleScript(AsyncWebServerRequest *request);
    void handleBootstrapStyle(AsyncWebServerRequest *request);
    void handleBootstrapScript(AsyncWebServerRequest *request);

    void setup();

}

#endif