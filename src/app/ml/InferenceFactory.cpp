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
#include "libs/tensorflow/detection.h"

#include "./InferenceFactory.h"
#include "../rpc/StringUtil.h"
#include "../../LibLog.h"

using namespace coralmicro;

///////////////////////////////////////////////////////////////////////
#define PredictionThreshold 0.51
#define PredictionSize 2

static const int ObjectIdSwimmer = 0;   // people
static const int ObjectIdLaneRope = 71; // tv

///////////////////////////////////////////////////////////////////////
InferenceFactory *InferenceFactory::_instance = nullptr;

///////////////////////////////////////////////////////////////////////
bool InferenceFactory::inference(tflite::MicroInterpreter *interpreter)
{
    configASSERT(interpreter);
    if (!interpreter)
    {
        return false;
    }

    auto input_tensor = interpreter->input_tensor(0);
    int model_height = input_tensor->dims->data[1];
    int model_width = input_tensor->dims->data[2];

    CameraFrameFormat fmt{
        CameraFormat::kRgb,
        CameraFilterMethod::kBilinear,
        CameraRotation::k270,
        model_width,
        model_height,
        false, // preserve_ratio
        _ptrWr->cameraImage.image.data(),
        true // white_balance
    };
    if (!CameraTask::GetSingleton()->GetFrame({fmt}))
    {
        return false;
    }

    // auto input_tensor = interpreter->input_tensor(0);
    std::memcpy(tflite::GetTensorData<uint8_t>(input_tensor),
                fmt.buffer, _ptrWr->cameraImage.getSize(fmt));
    if (interpreter->Invoke() != kTfLiteOk)
    {
        return false;
    }

    auto tfObjects = tensorflow::GetDetectionResults(interpreter, PredictionThreshold, PredictionSize);
    // DBGLOG(Debug, tensorflow::FormatDetectionOutput(tfObjects).c_str());

    if (findLane(tfObjects, _ptrWr->lane) && findSwimmer(tfObjects, _ptrWr->swimmer))
    {
        // coralmicro::tensorflow::Object &lane = _ptrWr->lane;
        // DBGLOG(Debug, "lane.id=%d, xmin=%f, xmax=%f", lane.id, lane.bbox.xmin, lane.bbox.xmax);

        // coralmicro::tensorflow::Object &swimmer = _ptrWr->swimmer;
        // DBGLOG(Debug, "swimmer.id=%d, xmin=%f, xmax=%f", swimmer.id, swimmer.bbox.xmin, swimmer.bbox.xmax);
    }

    _ptrWr->cameraImage.width = model_width;
    _ptrWr->cameraImage.height = model_height;

    _ptrWr->tfObjects = tfObjects;
    _ptrWr->setValid(true);

    ///////////////////////////////////////////////////////////////////////////
    // safely swap read/write pointers
    if (!lock())
    {
        DBGLOG(Debug, "lock() failed");
        return false;
    }
    swapPtrWrRd();
    if (!unlock())
    {
        DBGLOG(Debug, "unlock() failed");
    }
    ///////////////////////////////////////////////////////////////////////////

    return true;
}

// filter lane ropes by comparing tfObject.id with ObjectIdLaneRope
// return true if lane is found
bool InferenceFactory::findLane(std::vector<coralmicro::tensorflow::Object> &tfObjects,
                                tensorflow::Object &lane)
{
    for (size_t i = 0; i < tfObjects.size(); i++)
    {
        if (ObjectIdLaneRope == tfObjects[i].id)
        {
            lane.id = 1; // dummy for single lane
            lane.score = 0.0;
            lane.bbox = tfObjects[i].bbox;
            return true;
        }
    }

    lane.id = Lane::InvalidId; // no lane found
    return false;
}
bool InferenceFactory::findSwimmer(std::vector<coralmicro::tensorflow::Object> &tfObjects,
                                   tensorflow::Object &swimmer)
{
    for (size_t i = 0; i < tfObjects.size(); i++)
    {
        if (ObjectIdSwimmer == tfObjects[i].id)
        {
            swimmer.id = 0; // dummy for single swimmer
            // swimmer.id = tfObjects[i].id;
            swimmer.score = 0.0;
            swimmer.bbox = tfObjects[i].bbox;
            // std::memcpy(&swimmer.bbox, &tfObjects[i].bbox, sizeof(swimmer.bbox));

            return true;
        }
    }

    swimmer.id = Swimmer::InvalidId;
    return false;
}