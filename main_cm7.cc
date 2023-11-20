/*
make -C out -j4
python3 coralmicro/scripts/flashtool.py --build_dir out --elf_path out/coralmicro-app --nodata


cd prj/freertos-coral-swimmer
python3 py/coral_swimmer.py



*/

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

#include <cstdio>
#include "libs/base/mutex.h"
#include "libs/base/watchdog.h"
#include "libs/base/led.h"
#include "libs/base/utils.h"
#include "third_party/freertos_kernel/include/FreeRTOS.h"
#include "third_party/freertos_kernel/include/task.h"

#include "./src/ArduProf.h"
#include "./src/base/ipc/IpcCoreM7.h"
#include "./src/app/AppContext.h"
#include "./src/app/thread/ThreadInference.h"
#include "./src/app/thread/ThreadReporter.h"
#include "./src/app/thread/QueueMainM7.h"
#include "./src/app/rpc/RpcServer.h"

using namespace coralmicro;

///////////////////////////////////////////////////////////////////////////////

static AppContext appContext = {0};

///////////////////////////////////////////////////////////////////////////////
static void createTasks(void)
{
  appContext.queueMain = QueueMainM7::getInstance();
  appContext.threadInference = ThreadInference::getInstance();
  appContext.threadReporter = ThreadReporter::getInstance();

  if (appContext.queueMain)
  {
    static_cast<QueueMainM7 *>(appContext.queueMain)->start(&appContext, IpcCoreM7::getSingleton());
  }

  if (appContext.threadInference)
  {
    appContext.threadInference->start(&appContext);
  }

  if (appContext.threadReporter)
  {
    appContext.threadReporter->start(&appContext);
  }

  // start RPC server
  auto rpcServer = RpcServer::getInstance();
  rpcServer->start(nullptr, nullptr);
}

static void printAppInfo(void)
{
  std::string usb_ip;
  if (!GetUsbIpAddress(&usb_ip))
  {
    usb_ip = "null";
  }

  PRINTLN("===============================================================================");
  PRINTLN("GetUsbIpAddress()=%s", usb_ip.c_str());
  PRINTLN("===============================================================================");
}

extern "C" void app_main(void *param)
{
  (void)param;

  vTaskDelay(pdMS_TO_TICKS(1000)); // let USB becomes ready
  printAppInfo();

  constexpr WatchdogConfig wdt_config = {
      .timeout_s = 8,
      .pet_rate_s = 3,
      .enable_irq = false,
  };
  WatchdogStart(wdt_config);

  createTasks();
  (reinterpret_cast<QueueMain *>(appContext.queueMain))->messageLoopForever();

  // should not be here

  vTaskSuspend(nullptr);
}
