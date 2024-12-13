/*
 *
 *    Copyright (c) 2024 Project CHIP Authors
 *    All rights reserved.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

// Generic AppTask definition from application
#include "AppTask.h"
#include <lib/support/CHIPMem.h>
#include <platform/CHIPDeviceLayer.h>


LOG_MODULE_REGISTER(app, CONFIG_CHIP_APP_LOG_LEVEL);

using namespace ::chip;
using namespace ::chip::Inet;
using namespace ::chip::DeviceLayer;

int main(void)
{
    // Initialize CHIP stack and Open Thread stack
    LOG_INF("Init CHIP stack");

    CHIP_ERROR err = CHIP_NO_ERROR;

    err = chip::Platform::MemoryInit();
    if (err != CHIP_NO_ERROR) {
        LOG_ERR("MemoryInit fail");
        goto exit;
    }

    err = PlatformMgr().InitChipStack();
    if (err != CHIP_NO_ERROR) {
        LOG_ERR("InitChipStack fail");
        goto exit;
    }

    err = ThreadStackMgr().InitThreadStack();
    if (err != CHIP_NO_ERROR) {
        LOG_ERR("InitThreadStack fail");
        goto exit;
    }

#if CONFIG_CHIP_THREAD_SSED
    err = ConnectivityMgr().SetThreadDeviceType(ConnectivityManager::kThreadDeviceType_SynchronizedSleepyEndDevice);
#elif CONFIG_OPENTHREAD_MTD_SED
    err = ConnectivityMgr().SetThreadDeviceType(ConnectivityManager::kThreadDeviceType_SleepyEndDevice);
#else
    err = ConnectivityMgr().SetThreadDeviceType(ConnectivityManager::kThreadDeviceType_Router);
#endif
    if (err != CHIP_NO_ERROR) {
        LOG_ERR("SetThreadDeviceType fail");
        goto exit;
    }

    // Start Application
    err = AppTask::Instance().StartApp();

exit:

    LOG_ERR("Exited with code %" CHIP_ERROR_FORMAT, err.Format());
    return err == CHIP_NO_ERROR ? EXIT_SUCCESS : EXIT_FAILURE;
}
