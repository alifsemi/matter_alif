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

#include <platform/CHIPDeviceLayer.h>
#if CONFIG_CHIP_FACTORY_DATA
#include <platform/alif/FactoryDataProvider.h>
#else
#include <platform/alif/DeviceInstanceInfoProviderImpl.h>
#endif

#include <cstdint>

struct k_timer;
struct Identify;

class AppTask : public MatterAppTask
{
public:

    enum class Timer : uint8_t
    {
        DimmerTrigger,
        Dimmer
    };

    static AppTask & Instance()
    {
        static AppTask sAppTask;
        return sAppTask;
    };

    static void IdentifyStartHandler(Identify *);
    static void IdentifyStopHandler(Identify *);

private:
    friend class MatterAppTask;

    CHIP_ERROR Init();

    static void StartBLEAdvertisementHandler(const AppEvent * event);

};
