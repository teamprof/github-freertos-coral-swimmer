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

#include "libs/base/mutex.h"
#include "libs/base/ipc_m4.h"

#include "../type/Message.h"

using namespace coralmicro;

class IpcCoreM4 
{
public:
    // static const uint8_t mutexGate = 0;

    static IpcCoreM4 *getSingleton()
    {
        static IpcCoreM4 ipc;
        return &ipc;
    }

    void sendAppMessage(const Message &msg)
    {
        IpcMessage ipcMsg{
            .type = IpcMessageType::kApp,
        };
        auto *appMsg = reinterpret_cast<Message *>(&ipcMsg.message.data);
        appMsg->event = msg.event;
        appMsg->iParam = msg.iParam;
        appMsg->uParam = msg.uParam;
        appMsg->lParam = msg.lParam;

        // MulticoreMutexLock lock(mutexGate);
        IpcM4::GetSingleton()->SendMessage(ipcMsg);
    }

    void registerMessageHandler(Ipc::AppMessageHandler handler)
    {
        IpcM4::GetSingleton()->RegisterAppMessageHandler(handler);
    }

protected:
private:
};

// class IpcCoreM4 : public IpcM4
// {
// public:
//     static const uint8_t mutexGate = 0;

//     static IpcCoreM4 *getSingleton()
//     {
//         static IpcCoreM4 ipc;
//         return &ipc;
//     }

//     void sendAppMessage(const Message &msg)
//     {
//         IpcMessage ipcMsg{
//             .type = IpcMessageType::kApp,
//         };
//         auto *appMsg = reinterpret_cast<Message *>(&ipcMsg.message.data);
//         appMsg->event = msg.event;
//         appMsg->iParam = msg.iParam;
//         appMsg->uParam = msg.uParam;
//         appMsg->lParam = msg.lParam;

//         MulticoreMutexLock lock(mutexGate);
//         SendMessage(ipcMsg);
//     }

//     void registerMessageHandler(AppMessageHandler handler)
//     {
//         RegisterAppMessageHandler(handler);
//     }

// protected:
// private:
//     IpcCoreM4()
//     {
//     }
// };
