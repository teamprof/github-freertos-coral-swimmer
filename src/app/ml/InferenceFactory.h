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
#include "third_party/tflite-micro/tensorflow/lite/micro/micro_interpreter.h"
#include "../type/InferenceResult.h"

using namespace coralmicro;

class InferenceFactory
{
public:
    static InferenceFactory *getInstance(void)
    {
        if (!_instance)
        {
            static InferenceFactory factory;
            _instance = &factory;
        }
        return _instance;
    }

    bool inference(tflite::MicroInterpreter *interpreter);

    InferenceResult *getResult(void)
    {
        return _ptrRd;
    }

    bool lock(void)
    {
        return (xSemaphoreTake(_mutex, portMAX_DELAY) == pdTRUE);
    }

    bool unlock(void)
    {
        return (xSemaphoreGive(_mutex) == pdTRUE);
    }

private:
    static InferenceFactory *_instance;

    InferenceResult *_ptrWr;
    InferenceResult *_ptrRd;
    InferenceResult _results[2]; // for write and read

    SemaphoreHandle_t _mutex;

    InferenceFactory() : _results({}), _mutex(xSemaphoreCreateMutex())
    {
        _ptrWr = &_results[0];
        _ptrRd = &_results[1];
    }

    void swapPtrWrRd(void)
    {
        InferenceResult *tmp = _ptrRd;
        _ptrRd = _ptrWr;
        _ptrWr = tmp;
    }

    bool findLane(std::vector<coralmicro::tensorflow::Object> &tfObjects, tensorflow::Object &lane);
    bool findSwimmer(std::vector<coralmicro::tensorflow::Object> &tfObjects, tensorflow::Object &swimmer);
};
