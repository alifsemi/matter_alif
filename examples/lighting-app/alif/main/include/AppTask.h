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

#pragma once

#include "AppEvent.h"
#include "MatterAppTask.h"
#include "PWMDevice.h"

#include <platform/CHIPDeviceLayer.h>
#include <platform/alif/DeviceInstanceInfoProviderImpl.h>

#include <cstdint>

struct Identify;

class AppTask : public MatterAppTask
{
public:
    static AppTask & Instance()
    {
        static AppTask sAppTask;
        return sAppTask;
    };

    void UpdateClusterState();
    PWMDevice & GetPWMDevice() { return mPWMDevice; }

    static void IdentifyStartHandler(Identify *);
    static void IdentifyStopHandler(Identify *);
#ifdef CONFIG_CHIP_LIB_SHELL
    static CHIP_ERROR LightCommandHandler(int argc, char ** argv);
    static CHIP_ERROR DevFunctionCommandHandler(int argc, char ** argv);
#endif

private:
    friend class MatterAppTask;

    CHIP_ERROR Init();

    static void LightingActionEventHandler(const AppEvent * event);
    static void StartBLEAdvertisementHandler(const AppEvent * event);
    static void ActionInitiated(PWMDevice::Action_t action, int32_t actor);
    static void ActionCompleted(PWMDevice::Action_t action, int32_t actor);

    PWMDevice mPWMDevice;
};
