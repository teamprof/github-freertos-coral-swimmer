/* Copyright 2023 teamprof.net@gmail.com
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this
 * software and associated documentation files (the "Software"), to deal in the Software
 * without restriction, including without limitation the rights to use, copy, modify,
 * merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#pragma once
#include <stdint.h>

enum AppEvent : int16_t
{
    /////////////////////////////////////////////////////////////////////////////
    EventNull = 0,
    EventSystem, // iParam=SystemTriggerSource
    EventIPC,    // iParam=IpcCommand

    /////////////////////////////////////////////////////////////////////////////
};

enum SystemTriggerSource : int16_t
{
    SysSoftwareTimer = 1, // lParam=xTimer:uint32_t
};

enum IpcCommand : int16_t
{
    EventInterCore = 1,
    EventA2dpConnectionChanged, // uParam = 1 means earphone connected, 0 means disconnected
    EventLaneDetect,            // uParam = 1 means detected lane, 0 means not detected
    EventInference,             // uParam = (volume << 8) | (sound & 0x0FF)
};