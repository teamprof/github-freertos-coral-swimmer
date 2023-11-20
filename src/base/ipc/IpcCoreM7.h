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
#include "libs/base/ipc_m7.h"
#include "../type/Message.h"

using namespace coralmicro;

class IpcCoreM7 
{
public:
    // static const uint8_t mutexGate = 0;

    static bool hasM4Application(void)
    {
        return IpcM7::HasM4Application();
    }

    static IpcCoreM7 *getSingleton()
    {
        static IpcCoreM7 ipcM7;
        return &ipcM7;
    }

    void sendAppMessage(const Message &msg)
    {
        IpcMessage ipcMsg{
            .type = IpcMessageType::kApp};
        auto *appMsg = reinterpret_cast<Message *>(&ipcMsg.message.data);
        appMsg->event = msg.event;
        appMsg->iParam = msg.iParam;
        appMsg->uParam = msg.uParam;
        appMsg->lParam = msg.lParam;

        // MulticoreMutexLock lock(mutexGate);
        IpcM7::GetSingleton()->SendMessage(ipcMsg);
    }

    void registerMessageHandler(Ipc::AppMessageHandler handler)
    {
        IpcM7::GetSingleton()->RegisterAppMessageHandler(handler);
    }

    void startM4(void)
    {
        IpcM7::GetSingleton()->StartM4();
    }

    bool m4IsAlive(uint32_t millis)
    {
        return IpcM7::GetSingleton()->M4IsAlive(millis);
    }

protected:
private:
};


// class IpcCoreM7 : public IpcM7
// {
// public:
//     static const uint8_t mutexGate = 0;

//     static bool HasM4Application(void)
//     {
//         return IpcM7::HasM4Application();
//     }

//     static IpcCoreM7 *getSingleton()
//     {
//         IpcM7::GetSingleton();

//         static IpcCoreM7 ipc;
//         return &ipc;
//     }

//     void sendAppMessage(const Message &msg)
//     {
//         IpcMessage ipcMsg{
//             .type = IpcMessageType::kApp};
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

//     // void startM4(void)
//     // {
//     //     StartM4();
//     // }

//     // bool isAliveM4(uint32_t millis)
//     // {
//     //     return M4IsAlive(millis);
//     // }

//     // void Init() override {
//     //     IpcM7::Init();
//     // }

// protected:
// private:
//     // IpcCoreM7() : IpcM7()
//     // {
//     // }
// };
