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
#include <string.h>

#include "../peripheral/i2c/I2cCommand.h"
#include "../peripheral/i2c/I2cParam.h"
#include "../peripheral/i2c/I2cResponse.h"
#include "../AppContext.h"
#include "../AppEvent.h"
#include "./ThreadReporter.h"

/*
    coral                               esp32
      |                                     |
      |   I2cCommand::IsA2dpConnected       |
      | ----------------------------------> |
      |                                     |
      |   I2cResponse::A2dpConnected  or    |
      |   I2cResponse::A2dpDisconnected     |
      | <---------------------------------- |
      |                                     |
      |                                     |
      |       loop IsA2dpConnected          |
      |       until A2dpConnected           |
      |                                     |
      |                                     |
      |   I2cCommand::PlaySound             |
      |   param: <volumne> <sound>          |
      | ----------------------------------> |
      |                                     |
      |   I2cResponse::Success or           |
      |   I2cResponse::ErrorInvalidParam or |
      |   I2cResponse::ErrorDisconnected    |
      | <---------------------------------- |

    note:
    1. coral sends I2cCommand::PlaySound command every 0.5s
    2. coral re-start I2cCommand::IsA2dpConnected flow once "I2cResponse::ErrorDisconnected" is received
*/

////////////////////////////////////////////////////////////////////////////////////////////
#define I2C_DEV_ADDR ((uint8_t)0x55) // I2C device address

////////////////////////////////////////////////////////////////////////////////////////////
ThreadReporter *ThreadReporter::_instance = nullptr;

ThreadReporter *ThreadReporter::getInstance(void)
{
    static ThreadReporter instance;
    _instance = &instance;
    return _instance;
}

////////////////////////////////////////////////////////////////////////////////////////////
// Thread
////////////////////////////////////////////////////////////////////////////////////////////
#define TASK_NAME "ThreadReporter"
#define TASK_STACK_SIZE 2048
#define TASK_PRIORITY 3
#define TASK_QUEUE_SIZE 128 // message queue size for app task

static uint8_t ucQueueStorageArea[TASK_QUEUE_SIZE * sizeof(Message)];
static StaticQueue_t xStaticQueue;

static StackType_t xStack[TASK_STACK_SIZE];
static StaticTask_t xTaskBuffer;

////////////////////////////////////////////////////////////////////////////////////////////
ThreadReporter::ThreadReporter() : ThreadBase(TASK_QUEUE_SIZE, ucQueueStorageArea, &xStaticQueue),
                                   handlerMap(),
                                   _taskInitHandle(nullptr),
                                   _i2cDevice(coralmicro::I2c::kI2c1),
                                   _timerReporter("TimerReporter",
                                                  pdMS_TO_TICKS(500),
                                                  pdTRUE, // auto-reload when expire.
                                                  [](TimerHandle_t xTimer)
                                                  {
                                                      if (_instance)
                                                      {
                                                          _instance->postEvent(EventSystem, SysSoftwareTimer, 0, (uint32_t)xTimer);
                                                      }
                                                  }),
                                   _isA2dpConnected(false),
                                   _volume(0),
                                   _sound({0})
{
#ifdef SINGLE_INSTANCE_THREAD_REPORTER
    _instance = this;
#endif

    handlerMap = {
        __EVENT_MAP(ThreadReporter, EventIPC),
        __EVENT_MAP(ThreadReporter, EventSystem),
        __EVENT_MAP(ThreadReporter, EventNull), // {EventNull, &ThreadReporter::handlerEventNull},
    };
}

///////////////////////////////////////////////////////////////////////
__EVENT_FUNC_DEFINITION(ThreadReporter, EventSystem, msg) // void ThreadReporter::handlerEventSystem(const Message &msg)
{
    // DBGLOG(Debug, "EventSystem(%hd), iParam=%hd, uParam=%hu, lParam=%lu", msg.event, msg.iParam, msg.uParam, msg.lParam);

    enum SystemTriggerSource src = static_cast<SystemTriggerSource>(msg.iParam);
    switch (src)
    {
    case SysSoftwareTimer:
        if ((TimerHandle_t)(msg.lParam) == _timerReporter.timer())
        {
            schedulerReporter();
        }
        else
        {
            DBGLOG(Debug, "unsupported timer handle=0x%08lx", msg.lParam);
        }
        break;

    default:
        DBGLOG(Debug, "Unsupported SystemTriggerSource=%hd", src);
        break;
    }
}
__EVENT_FUNC_DEFINITION(ThreadReporter, EventIPC, msg) // void ThreadReporter::handlerEventIPC(const Message &msg)
{
    // DBGLOG(Debug, "EventIPC(%hd), iParam=%hd, uParam=%hu, lParam=%lu", msg.event, msg.iParam, msg.uParam, msg.lParam);

    _volume = (uint8_t)(msg.uParam >> 8);
    _sound.byte.data = (uint8_t)(msg.uParam & 0xFF);
    // DBGLOG(Debug, "_volume=%hu, _sound.byte.data=0x%02x", _volume, _sound.byte.data);
}
__EVENT_FUNC_DEFINITION(ThreadReporter, EventNull, msg) // void ThreadReporter::handlerEventNull(const Message &msg)
{
    DBGLOG(Debug, "EventNull(%hd), iParam=%hd, uParam=%hu, lParam=%lu", msg.event, msg.iParam, msg.uParam, msg.lParam);
}

///////////////////////////////////////////////////////////////////////
void ThreadReporter::onMessage(const Message &msg)
{
    // DBGLOG(Debug, "event=%hd, iParam=%hd, uParam=%hu, lParam=%lu", msg.event, msg.iParam, msg.uParam, msg.lParam);
    auto func = handlerMap[msg.event];
    if (func)
    {
        (this->*func)(msg);
    }
    else
    {
        DBGLOG(Debug, "Unsupported event=%hd, iParam=%hd, uParam=%hu, lParam=%lu", msg.event, msg.iParam, msg.uParam, msg.lParam);
    }
}

void ThreadReporter::start(void *ctx)
{
    configASSERT(ctx);
    _context = ctx;

    _taskHandle = xTaskCreateStatic(
        [](void *instance)
        { static_cast<ThreadBase *>(instance)->run(); },
        TASK_NAME,
        TASK_STACK_SIZE, // This stack size can be checked & adjusted by reading the Stack Highwater
        this,
        TASK_PRIORITY, // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
        xStack,
        &xTaskBuffer);
}

void ThreadReporter::setup(void)
{
    // DBGLOG(Debug, "xPortGetFreeHeapSize()=%u", xPortGetFreeHeapSize());

    _i2cDevice.init();
    _timerReporter.start();

    // ThreadBase::setup() invokes delayInit()
    // do not call ThreadBase::setup() if delayInit() requires large stack size,
    ThreadBase::setup();
}

void ThreadReporter::run(void)
{
    // DBGLOG(Debug, "xPortGetFreeHeapSize()=%u", xPortGetFreeHeapSize());
    ThreadBase::run();
}

// note: this function is invoked in ThreadBase::setup()
void ThreadReporter::delayInit(void)
{
    // DBGLOG(Debug, "xPortGetFreeHeapSize()=%u", xPortGetFreeHeapSize());
}

///////////////////////////////////////////////////////////////////////
void ThreadReporter::schedulerReporter(void)
{
    if (_isA2dpConnected)
    {
        uint8_t command[3]; // 3 bytes buffer for i2c PlaySound command

        // volume and sound data comes from threadInference
        command[0] = I2cCommand::PlaySound;
        command[1] = _volume;
        command[2] = _sound.byte.data;

        // // for testing only
        // uint8_t volume = 50;
        // I2cParam::Sound sound{
        //     .bit{
        //         .laneMiddle = 1,
        //         // .edgeTop = 1
        //     }};
        // command[0] = I2cCommand::PlaySound;
        // command[1] = volume;
        // command[2] = sound.byte.data;

        // DBGLOG(Debug, "i2cCommand=0x%02x 0x%02x 0x%02x", command[0], command[1], command[2]);

        uint8_t result = 0;
        bool ret = _i2cDevice.writeRead(I2C_DEV_ADDR, command, sizeof(command), &result, sizeof(result));
        // DBGLOG(Debug, "_i2cDevice.writeRead(): returns %d, result=%d", ret, result);
        if (!ret || result == I2cResponse::ErrorDisconnected)
        {
            setA2dpConntectionStatus(false);
        }

        // clear volume and sound after sent
        _volume = 0;
        _sound.byte.data = 0;
    }
    else
    {
        static const uint8_t command = I2cCommand::IsA2dpConnected;
        uint8_t result = 0;
        bool ret = _i2cDevice.writeRead(I2C_DEV_ADDR, &command, sizeof(command), &result, sizeof(result));
        // DBGLOG(Debug, "_i2cDevice.writeRead(): returns %d, result=%d", ret, result);
        if (ret)
        {
            setA2dpConntectionStatus(result == I2cResponse::A2dpConnected);
        }
    }
}

void ThreadReporter::setA2dpConntectionStatus(bool value)
{
    if (_isA2dpConnected != value)
    {
        _isA2dpConnected = value;

        AppContext *appContext = static_cast<AppContext *>(context());
        configASSERT(appContext && appContext->queueMain);
        if (appContext && appContext->queueMain)
        {
            appContext->queueMain->postEvent(EventIPC, EventA2dpConnectionChanged, (uint16_t)value);
        }
    }
}