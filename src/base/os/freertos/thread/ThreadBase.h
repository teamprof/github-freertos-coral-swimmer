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
#include "third_party/freertos_kernel/include/FreeRTOS.h"
#include "third_party/freertos_kernel/include/task.h"
#include "third_party/freertos_kernel/include/queue.h"

#include "../MessageQueue.h"

class ThreadBase : public MessageQueue
{
public:
    ThreadBase(uint16_t queueLength,
               uint8_t *pucQueueStorageBuffer = nullptr,
               StaticQueue_t *pxQueueBuffer = nullptr) : MessageQueue(queueLength, pucQueueStorageBuffer, pxQueueBuffer),
                                                         _context(nullptr),
                                                         _taskHandle(nullptr),
                                                         _taskDone(false)
    {
    }

    virtual void start(void *) = 0;

    virtual void onMessage(const Message &msg) = 0;

    virtual void messageLoop(TickType_t xTicksToWait = portMAX_DELAY)
    {
        Message msg;
        if (xQueueReceive(_queue, (void *)&msg, xTicksToWait) == pdTRUE)
        {
            onMessage(msg);
        }
        else
        {
            // LOG_DEBUG("xQueueReceive() timeout");
        }
    }

    virtual void messageLoopForever(void)
    {
        while (!_taskDone)
        {
            messageLoop();
        }
    }

    virtual void run(void)
    {
        setup();

        messageLoopForever();

        vTaskDelay(pdMS_TO_TICKS(100)); // delay 100ms
        vTaskDelete(_taskHandle);
    }

    void *context(void)
    {
        return _context;
    }

protected:
    virtual void setup(void)
    {
        // BaseType_t result = xTimerPendFunctionCall(
        //     [](void *param1, uint32_t param2)
        //     {
        //         // LOG_TRACE("xTimerPendFunctionCall()");
        //         static_cast<ThreadBase *>(param1)->delayInit();
        //     },
        //     this,        // param1
        //     (uint32_t)0, // param2
        //     pdMS_TO_TICKS(200));
        // (void)result;
        // // LOG_TRACE("xTimerPendFunctionCall() returns ", result);
        delayInit();
    }
    // virtual void setup(void) = 0;

    virtual void delayInit(void) = 0;

    void *_context;
    TaskHandle_t _taskHandle;

    bool _taskDone;
};