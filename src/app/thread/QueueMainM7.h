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
#include <map>

#include "../../base/ipc/IpcCoreM7.h"
#include "../AppEvent.h"
#include "./QueueMain.h"

#define SINGLE_INSTANCE_QUEUE_MAIN_M7

class QueueMainM7 : public QueueMain
{
public:
    static QueueMainM7 *getInstance(void);

private:
    static QueueMainM7 *_instance;
    QueueMainM7();

public:
    void start(void *context, void *ipc);

protected:
    typedef void (QueueMainM7::*handlerFunc)(const Message &);
    std::map<int16_t, handlerFunc> handlerMap;

    void onMessage(const Message &msg);

private:
    IpcCoreM7 *ipc;
    bool _isA2dpConnected;
    bool _isLaneDetected;

    virtual void setup(void);

    ///////////////////////////////////////////////////////////////////////
    // event handler
    ///////////////////////////////////////////////////////////////////////
    __EVENT_FUNC_DECLARATION(EventIPC)
};
