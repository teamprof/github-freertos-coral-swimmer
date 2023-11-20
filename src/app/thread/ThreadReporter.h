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

#include "../../ArduProf.h"
#include "../../base/peripheral/PeriodicTimer.h"
#include "../peripheral/i2c/I2cDevice.h"
#include "../peripheral/i2c/I2cParam.h"
#include "../AppEvent.h"

#define SINGLE_INSTANCE_THREAD_REPORTER

///////////////////////////////////////////////////////////////////////////////
class ThreadReporter : public ThreadBase
{
public:
    static ThreadReporter *getInstance(void);

private:
    ThreadReporter();
    static ThreadReporter *_instance;

public:
    virtual void start(void *);

protected:
    typedef void (ThreadReporter::*handlerFunc)(const Message &);
    std::map<int16_t, handlerFunc> handlerMap;

    virtual void onMessage(const Message &msg);
    virtual void run(void);

private:
    ///////////////////////////////////////////////////////////////////////////
    TaskHandle_t _taskInitHandle;

    I2cDevice _i2cDevice;
    PeriodicTimer _timerReporter;
    bool _isA2dpConnected;

    uint8_t _volume;
    I2cParam::Sound _sound;

    ///////////////////////////////////////////////////////////////////////////
    virtual void setup(void);
    virtual void delayInit(void);

    void schedulerReporter(void);
    void setA2dpConntectionStatus(bool value);

    ///////////////////////////////////////////////////////////////////////////
    // event handler
    ///////////////////////////////////////////////////////////////////////////
    __EVENT_FUNC_DECLARATION(EventIPC)
    __EVENT_FUNC_DECLARATION(EventSystem)
    __EVENT_FUNC_DECLARATION(EventNull) // void handlerEventNull(const Message &msg);
};