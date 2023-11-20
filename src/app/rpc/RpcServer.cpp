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
#include <string>
#include "libs/rpc/rpc_http_server.h"
#include "libs/rpc/rpc_utils.h"
#include "libs/camera/camera.h"
#include "third_party/mjson/src/mjson.h"

#include "../../LibLog.h"
#include "../ml/InferenceFactory.h"
#include "./RpcServer.h"
#include "./StringUtil.h"

////////////////////////////////////////////////////////////////////////////////////////////
static const char RPC_SERVER_NAME[] = "get_image_from_camera";

RpcServer *RpcServer::_instance = nullptr;

////////////////////////////////////////////////////////////////////////////////////////////
RpcServer *RpcServer::getInstance(void)
{
    if (!_instance)
    {
        static RpcServer rpcServer;
        _instance = &rpcServer;
    }
    return _instance;
}

void RpcServer::start(mjson_print_fn_t response_cb, void *userdata)
{
    _userdata = userdata;

    jsonrpc_init(response_cb, userdata);
    jsonrpc_export(RPC_SERVER_NAME, makeJsonResponse);
    coralmicro::UseHttpServer(new coralmicro::JsonRpcHttpServer);
}

void RpcServer::makeJsonResponse(struct jsonrpc_request *request)
{
    auto inferenceFactory = InferenceFactory::getInstance();
    if (!inferenceFactory->lock())
    {
        DBGLOG(Debug, "inferenceFactory->lock() failed");
        return;
    }

    auto result = inferenceFactory->getResult();
    if (result->isValid())
    {
        auto tfObjects = result->tfObjects;
        auto size = tfObjects.size();

        std::string sObjects = "\"tfObjects\":[";
        if (size > 0)
        {
            StringUtil::appendTfObject(sObjects, &tfObjects[0]);
        }
        for (size_t i = 1; i < size; i++)
        {
            sObjects.append(",");
            StringUtil::appendTfObject(sObjects, &tfObjects[i]);
        }
        sObjects.append("]");
        // DBGLOG(Debug, "sObjects=%s", sObjects.c_str());

        std::string sLanes = "\"tfLanes\":[";
        if (result->lane.isValid())
        {
            StringUtil::appendTfObject(sLanes, &result->lane);
        }
        sLanes.append("]");
        // DBGLOG(Debug, sLanes.c_str());

        std::string sSwimmers = "\"tfSwimmers\":[";
        if (result->swimmer.isValid())
        {
            StringUtil::appendTfObject(sSwimmers, &result->swimmer);
        }
        sSwimmers.append("]");
        // DBGLOG(Debug, sSwimmers.c_str());

        auto cameraImage = &result->cameraImage;
        // DBGLOG(Debug, "cameraImage->width=%d, height=%d, size=%d", cameraImage->width, cameraImage->height, cameraImage->image.size());
        jsonrpc_return_success(request, "{%Q: %d, %Q: %d, %Q: %V, %s, %s, %s}",
                               "width", cameraImage->width,
                               "height", cameraImage->height,
                               "base64_data", cameraImage->image.size(), cameraImage->image.data(),
                               sLanes.c_str(),
                               sSwimmers.c_str(),
                               sObjects.c_str());

        result->invalid();
    }
    else
    {
        jsonrpc_return_error(request, -1, "Failed to get image from camera.", nullptr);
    }

    if (!inferenceFactory->unlock())
    {
        DBGLOG(Debug, "inferenceFactory->unlock() failed");
    }
}
