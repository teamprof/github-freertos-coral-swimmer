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
#include <optional>
#include "libs/base/gpio.h"
#include "libs/base/utils.h"
#include "libs/camera/camera.h"
#include "libs/base/led.h"
#include "third_party/freertos_kernel/include/FreeRTOS.h"

#include "../AppContext.h"
#include "../AppEvent.h"
#include "./QueueMainM7.h"

using namespace coralmicro;

////////////////////////////////////////////////////////////////////////////////////////////
QueueMainM7 *QueueMainM7::_instance = nullptr;

QueueMainM7 *QueueMainM7::getInstance(void)
{
    static QueueMainM7 instance;
    _instance = &instance;
    return _instance;
}

////////////////////////////////////////////////////////////////////////////////////////////
QueueMainM7::QueueMainM7() : QueueMain(),
                             handlerMap(),
                             ipc(nullptr),
                             _isA2dpConnected(false),
                             _isLaneDetected(false)
{
    handlerMap = {
        __EVENT_MAP(QueueMainM7, EventIPC),
    };
}

///////////////////////////////////////////////////////////////////////
// event handler
///////////////////////////////////////////////////////////////////////
__EVENT_FUNC_DEFINITION(QueueMainM7, EventIPC, msg) // void QueueMainM7::handlerEventIPC(const Message &msg)
{
    // DBGLOG(Debug, "EventIPC(%hd), iParam=%hd, uParam=%hu, lParam=%lu", msg.event, msg.iParam, msg.uParam, msg.lParam);
    IpcCommand ipcCommand = static_cast<IpcCommand>(msg.iParam);
    switch (ipcCommand)
    {
    case EventLaneDetect:
    {
        bool isLaneDetected = static_cast<bool>(msg.uParam);
        if (_isLaneDetected != isLaneDetected)
        {
            _isLaneDetected = isLaneDetected;
            LedSet(Led::kUser, _isLaneDetected);
            DBGLOG(Debug, "EventLaneDetect: _isLaneDetected=%hd", _isLaneDetected);
        }
        break;
    }

    case EventA2dpConnectionChanged:
    {
        bool isConnected = static_cast<bool>(msg.uParam);
        if (_isA2dpConnected != isConnected)
        {
            _isA2dpConnected = isConnected;
            LedSet(Led::kStatus, _isA2dpConnected);
            DBGLOG(Debug, "EventA2dpConnectionChanged: _isA2dpConnected=%hd", _isA2dpConnected);
        }
        break;
    }

    default:
        DBGLOG(Debug, "Unsupported ipcCommand=%hd, uParam=%hu, lParam=%lu", ipcCommand, msg.uParam, msg.lParam);
        break;
    }
}

///////////////////////////////////////////////////////////////////////
void QueueMainM7::onMessage(const Message &msg)
{
    auto func = handlerMap[msg.event];
    if (func)
    {
        (this->*func)(msg);
    }
    else
    {
        QueueMain::onMessage(msg);
    }
}

///////////////////////////////////////////////////////////////////////
void QueueMainM7::start(void *context, void *ipc)
{
    this->ipc = static_cast<IpcCoreM7 *>(ipc);
    QueueMain::start(context);
}

void QueueMainM7::setup(void)
{
}
