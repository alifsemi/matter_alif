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

#include "AppConfig.h"
#include "AppEvent.h"

#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <platform/CHIPDeviceLayer.h>

#include <credentials/examples/DeviceAttestationCredsExample.h>

#if CONFIG_CHIP_FACTORY_DATA
#include <platform/alif/FactoryDataProvider.h>
#else
#include <platform/alif/DeviceInstanceInfoProviderImpl.h>
#endif

#include <cstdint>

using namespace ::chip;
using namespace ::chip::app;
using namespace ::chip::Credentials;
using namespace ::chip::DeviceLayer;

class MatterAppTask
{
public:
    CHIP_ERROR StartApp();
    void PostEvent(AppEvent * event);

protected:
    CHIP_ERROR InitCommonParts(void);

    void DispatchEvent(const AppEvent * event);
    void GetEvent(AppEvent * aEvent);
    static void ChipEventHandler(const chip::DeviceLayer::ChipDeviceEvent * event, intptr_t arg);
#if CONFIG_CHIP_FACTORY_DATA
    chip::DeviceLayer::FactoryDataProvider<chip::DeviceLayer::ExternalFlashFactoryData> mFactoryDataProvider;
#endif
};
