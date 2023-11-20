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
#include <vector>
#include "libs/base/filesystem.h"
#include "libs/tensorflow/utils.h"
#include "third_party/tflite-micro/tensorflow/lite/micro/micro_error_reporter.h"
#include "third_party/tflite-micro/tensorflow/lite/micro/micro_interpreter.h"
#include "third_party/tflite-micro/tensorflow/lite/micro/micro_mutable_op_resolver.h"

#include "../type/SwimmingPool.h"
#include "../ml/InferenceFactory.h"
#include "../peripheral/i2c/I2cParam.h"
#include "../AppContext.h"
#include "./ThreadInference.h"

// volume for Bluetooth speaker
#define DefaultVolume 80

////////////////////////////////////////////////////////////////////////////////////////////
static constexpr char kModelPath[] = "/coralmicro/models/tf2_ssd_mobilenet_v2_coco17_ptq_edgetpu.tflite";

////////////////////////////////////////////////////////////////////////////////////////////
ThreadInference *ThreadInference::_instance = nullptr;

ThreadInference *ThreadInference::getInstance(void)
{
    static ThreadInference instance;
    _instance = &instance;
    return _instance;
}

////////////////////////////////////////////////////////////////////////////////////////////
// Thread
////////////////////////////////////////////////////////////////////////////////////////////
#define TASK_NAME "ThreadInference"
#define TASK_STACK_SIZE 4096
#define TASK_PRIORITY 3
#define TASK_QUEUE_SIZE 128 // message queue size for app task

static uint8_t ucQueueStorageArea[TASK_QUEUE_SIZE * sizeof(Message)];
static StaticQueue_t xStaticQueue;

static StackType_t xStack[TASK_STACK_SIZE];
static StaticTask_t xTaskBuffer;

////////////////////////////////////////////////////////////////////////////////////////////
ThreadInference::ThreadInference() : ThreadBase(TASK_QUEUE_SIZE, ucQueueStorageArea, &xStaticQueue),
                                     handlerMap(),
                                     _taskInitHandle(nullptr),
                                     _interpreter(nullptr),
                                     _timerInference("TimerImageRecorder",
                                                     pdMS_TO_TICKS(50),
                                                     pdTRUE, // auto-reload when expire.
                                                     [](TimerHandle_t xTimer)
                                                     {
                                                         if (_instance)
                                                         {
                                                             _instance->postEvent(EventSystem, SysSoftwareTimer, 0, (uint32_t)xTimer);
                                                         }
                                                     })
{
    _instance = this;

    handlerMap = {
        __EVENT_MAP(ThreadInference, EventSystem),
        __EVENT_MAP(ThreadInference, EventNull), // {EventNull, &ThreadInference::handlerEventNull},
    };
}

///////////////////////////////////////////////////////////////////////
__EVENT_FUNC_DEFINITION(ThreadInference, EventSystem, msg) // void ThreadInference::handlerEventSystem(const Message &msg)
{
    // DBGLOG(Debug, "EventSystem(%hd), iParam=%hd, uParam=%hu, lParam=%lu", msg.event, msg.iParam, msg.uParam, msg.lParam);

    enum SystemTriggerSource src = static_cast<SystemTriggerSource>(msg.iParam);
    switch (src)
    {
    case SysSoftwareTimer:
        if ((TimerHandle_t)(msg.lParam) == _timerInference.timer())
        {
            schedulerInference();
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
__EVENT_FUNC_DEFINITION(ThreadInference, EventNull, msg) // void ThreadInference::handlerEventNull(const Message &msg)
{
    DBGLOG(Debug, "EventNull(%hd), iParam=%hd, uParam=%hu, lParam=%lu", msg.event, msg.iParam, msg.uParam, msg.lParam);
}

///////////////////////////////////////////////////////////////////////
void ThreadInference::onMessage(const Message &msg)
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

void ThreadInference::start(void *ctx)
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

void ThreadInference::setup(void)
{
    auto interpreter = getInterpreter(kModelPath);
    configASSERT(interpreter);

    // turn on camera
    CameraTask::GetSingleton()->SetPower(true);
    CameraTask::GetSingleton()->Enable(CameraMode::kStreaming);

    // trigger inference to provide data for RPC server
    bool inference = InferenceFactory::getInstance()->inference(interpreter);
    if (!inference)
    {
        DBGLOG(Debug, "InferenceFactory::getInstance()->inference(interpreter) returns %d", inference);
    }

    // start schedular for inference
    _timerInference.start();

    // ThreadBase::setup() invokes delayInit()
    // do not call ThreadBase::setup() if delayInit() requires large stack size,
    ThreadBase::setup();

    // vTaskDelay(pdMS_TO_TICKS(500));
}

void ThreadInference::run(void)
{
    ThreadBase::run();
}

// note: this function is invoked in ThreadBase::setup()
void ThreadInference::delayInit(void)
{
}

///////////////////////////////////////////////////////////////////////
void ThreadInference::schedulerInference(void)
{
    auto factory = InferenceFactory::getInstance();
    if (!factory->inference(_interpreter))
    {
        return;
    }

    auto result = factory->getResult();
    if (!result->isValid())
    {
        return;
    }

    uint8_t volume = DefaultVolume;
    I2cParam::Sound sound = {0};
    if (result->lane.isValid() && result->swimmer.isValid())
    {
        // determine swimmer location within lane
        Swimmer &swimmer = result->swimmer;
        float x = (swimmer.bbox.xmin + swimmer.bbox.xmax) / 2;
        // DBGLOG(Debug, "swimmer.id=%d, x=%f, xmin=%f, xmax=%f", swimmer.id, x, swimmer.bbox.xmin, swimmer.bbox.xmax);

        Lane lane = result->lane;
        // DBGLOG(Debug, "lane.id=%d, xmin=%f, xmax=%f", lane.id, lane.bbox.xmin, lane.bbox.xmax);

        if (x < (lane.bbox.xmin + SwimmingPool::LaneRopeRegionX))
        {
            // DBGLOG(Debug, "swimmer near left rope");
            sound.bit.laneLeft = 1;
        }
        else if (x > (lane.bbox.xmax - SwimmingPool::LaneRopeRegionX))
        {
            // DBGLOG(Debug, "swimmer near right rope");
            sound.bit.laneRight = 1;
        }
        else
        {
            // DBGLOG(Debug, "swimmer is in middle");
            sound.bit.laneMiddle = 1;
        }
    }
    else
    {
    }

    AppContext *appContext = static_cast<AppContext *>(context());
    if (appContext && appContext->threadReporter)
    {
        uint16_t uParam = (((uint16_t)volume) << 8) | (uint16_t)(sound.byte.data);
        appContext->threadReporter->postEvent(EventIPC, EventInference, uParam);
    }

    if (appContext && appContext->queueMain)
    {
        appContext->queueMain->postEvent(EventIPC, EventLaneDetect, result->lane.isValid());
    }
}

tflite::MicroInterpreter *ThreadInference::getInterpreter(const char *kModelPath)
{
    if (!_interpreter)
    {
        static std::vector<uint8_t> model;
        DBGLOG(Debug, "Loading: %s ...", kModelPath);
        if (!LfsReadFile(kModelPath, &model))
        {
            DBGLOG(Error, "Failed to load %s", kModelPath);
            return nullptr;
        }

        DBGLOG(Debug, "EdgeTpuManager::GetSingleton()->OpenDevice() ...");
        static auto tpu_context = EdgeTpuManager::GetSingleton()->OpenDevice();
        if (!tpu_context)
        {
            DBGLOG(Error, "Failed to get EdgeTpu context");
            return nullptr;
        }

        DBGLOG(Debug, "Initializing tflite::MicroMutableOpResolver ...");
        static tflite::MicroErrorReporter error_reporter;
        static tflite::MicroMutableOpResolver<3> resolver;
        resolver.AddDequantize();
        resolver.AddDetectionPostprocess();
        resolver.AddCustom(kCustomOp, RegisterCustomOp());

        // An area of memory to use for input, output, and intermediate arrays.
        static constexpr int kTensorArenaSize = 8 * 1024 * 1024;
        STATIC_TENSOR_ARENA_IN_SDRAM(tensor_arena, kTensorArenaSize);

        DBGLOG(Debug, "Initializing tflite::MicroInterpreter ...");
        static tflite::MicroInterpreter interpreter(tflite::GetModel(model.data()), resolver,
                                                    tensor_arena, kTensorArenaSize,
                                                    &error_reporter);
        if (interpreter.AllocateTensors() != kTfLiteOk)
        {
            DBGLOG(Error, "AllocateTensors() failed");
            return nullptr;
        }

        if (interpreter.inputs().size() != 1)
        {
            DBGLOG(Error, "Model must have only one input tensor");
            return nullptr;
        }

        auto input_tensor = interpreter.input_tensor(0);
        int model_height = input_tensor->dims->data[1];
        int model_width = input_tensor->dims->data[2];
        DBGLOG(Debug, "Initializing tflite::MicroInterpreter done: model_height=%d, model_width=%d", model_height, model_width);

        _interpreter = &interpreter;
    }
    return _interpreter;
}