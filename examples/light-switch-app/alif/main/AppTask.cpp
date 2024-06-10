/*
 *
 *    Copyright (c) 2022 Project CHIP Authors
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

#include "AppTask.h"
#include "AppConfig.h"
#include "LightSwitch.h"

#include <DeviceInfoProviderImpl.h>
#include <app/TestEventTriggerDelegate.h>
#include <app/clusters/identify-server/identify-server.h>
#include <app/server/OnboardingCodesUtil.h>
#include <app/server/Server.h>
#include <credentials/DeviceAttestationCredsProvider.h>
#include <credentials/examples/DeviceAttestationCredsExample.h>
#include <lib/support/CHIPMem.h>
#include <lib/support/CodeUtils.h>
#include <lib/support/SafeInt.h>
#include <platform/CHIPDeviceLayer.h>
#include <system/SystemError.h>


#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(app, CONFIG_CHIP_APP_LOG_LEVEL);

using namespace ::chip;
using namespace ::chip::app;
using namespace ::chip::Credentials;
using namespace ::chip::DeviceLayer;

namespace {
constexpr EndpointId kLightDimmerSwitchEndpointId  = 1;
constexpr EndpointId kLightGenericSwitchEndpointId = 2;
constexpr EndpointId kLightEndpointId              = 1;

constexpr uint32_t kDimmerTriggeredTimeout          = 500;
constexpr uint32_t kDimmerInterval                  = 300;

Identify sIdentify = { kLightEndpointId, AppTask::IdentifyStartHandler, AppTask::IdentifyStopHandler,
                       Clusters::Identify::IdentifyTypeEnum::kVisibleIndicator };

chip::DeviceLayer::DeviceInfoProviderImpl gExampleDeviceInfoProvider;
} // namespace


CHIP_ERROR AppTask::Init()
{
    // Initialize CHIP
    LOG_INF("Init CHIP stack");

    // Init Light switch endpoints
    LightSwitch::GetInstance().Init(kLightDimmerSwitchEndpointId, kLightGenericSwitchEndpointId);

    CHIP_ERROR err = InitCommonParts();
    if (err != CHIP_NO_ERROR) {
        LOG_ERR("Matter common device Init fail");
        return err;
    }

    gExampleDeviceInfoProvider.SetStorageDelegate(&Server::GetInstance().GetPersistentStorage());
    chip::DeviceLayer::SetDeviceInfoProvider(&gExampleDeviceInfoProvider);

    return err;
}

void AppTask::IdentifyStartHandler(Identify *)
{
    AppEvent event;
    event.Type    = AppEventType::IdentifyStart;
    event.Handler = [](const AppEvent *) {
        LOG_INF("Identify start");
    };
    Instance().PostEvent(&event);
}

void AppTask::IdentifyStopHandler(Identify *)
{
    AppEvent event;
    event.Type    = AppEventType::IdentifyStop;
    event.Handler = [](const AppEvent *) {
        LOG_INF("Identify stop");
    };
    Instance().PostEvent(&event);
}

void AppTask::StartBLEAdvertisementHandler(const AppEvent *)
{
    if (Server::GetInstance().GetFabricTable().FabricCount() != 0)
    {
        LOG_INF("Matter service BLE advertising not started - device is already commissioned");
        return;
    }

    if (ConnectivityMgr().IsBLEAdvertisingEnabled())
    {
        LOG_INF("BLE advertising is already enabled");
        return;
    }

    if (Server::GetInstance().GetCommissioningWindowManager().OpenBasicCommissioningWindow() != CHIP_NO_ERROR)
    {
        LOG_ERR("OpenBasicCommissioningWindow() failed");
    }
}
