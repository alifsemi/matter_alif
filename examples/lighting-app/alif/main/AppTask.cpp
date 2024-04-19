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

#include "AppTask.h"

#include "AppConfig.h"
#include "AppEvent.h"
#include "PWMDevice.h"

#include <DeviceInfoProviderImpl.h>
#include <app-common/zap-generated/attributes/Accessors.h>
#include <app/DeferredAttributePersistenceProvider.h>
#include <app/clusters/identify-server/identify-server.h>
#include <app/server/Dnssd.h>
#include <app/server/OnboardingCodesUtil.h>
#include <app/server/Server.h>
#include <credentials/DeviceAttestationCredsProvider.h>
#include <credentials/examples/DeviceAttestationCredsExample.h>
#include <lib/core/ErrorStr.h>
#include <lib/support/CHIPMem.h>
#include <lib/support/CodeUtils.h>
#include <system/SystemClock.h>

#ifdef CONFIG_CHIP_LIB_SHELL
#include <lib/core/CHIPError.h>
#include <lib/shell/Engine.h>
#include <lib/shell/commands/Help.h>
#endif

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(app, CONFIG_CHIP_APP_LOG_LEVEL);

using namespace ::chip;
using namespace ::chip::app;
using namespace ::chip::Credentials;
using namespace ::chip::DeviceLayer;

namespace {

constexpr EndpointId kLightEndpointId          = 1;
constexpr uint8_t kDefaultMinLevel             = 0;
constexpr uint8_t kDefaultMaxLevel             = 254;

// Create Identify server
Identify sIdentify = { kLightEndpointId, AppTask::IdentifyStartHandler, AppTask::IdentifyStopHandler,
                       Clusters::Identify::IdentifyTypeEnum::kVisibleIndicator };

chip::DeviceLayer::DeviceInfoProviderImpl gExampleDeviceInfoProvider;

// Define a custom attribute persister which makes actual write of the PWM dimm CurrentLevel attribute value
// to the non-volatile storage only when it has remained constant for 5 seconds.
DeferredAttribute gCurrentLevelPersister(ConcreteAttributePath(kLightEndpointId, Clusters::LevelControl::Id,
                                                               Clusters::LevelControl::Attributes::CurrentLevel::Id));
DeferredAttributePersistenceProvider gDeferredAttributePersister(Server::GetInstance().GetDefaultAttributePersister(),
                                                                 Span<DeferredAttribute>(&gCurrentLevelPersister, 1),
                                                                 System::Clock::Milliseconds32(5000));
} // namespace

#ifdef CONFIG_CHIP_LIB_SHELL
namespace ShellCommands
{
using Shell::Engine;
using Shell::shell_command_t;
using Shell::streamer_get;
using Shell::streamer_printf;

void RegisterShellCommands(void)
{
    static const shell_command_t sLightCommand = { AppTask::LightCommandHandler, "light", "Light test commands. Usage: light [on|off]" };
    static const shell_command_t sDevFunctCommand = { AppTask::DevFunctionCommandHandler, "devfunc", "Light test commands. Usage: devfunc [blecom]" };

    Engine::Root().RegisterCommands(&sLightCommand, 1);
    Engine::Root().RegisterCommands(&sDevFunctCommand, 1);
    return;
}
} // Namespace

CHIP_ERROR AppTask::LightCommandHandler(int argc, char ** argv)
{
    AppEvent Lighting_Event;

    Lighting_Event.Type = AppEventType::Lighting;
    Lighting_Event.Handler = AppTask::Instance().LightingActionEventHandler;
    Lighting_Event.LightingEvent.Actor = static_cast<int32_t>(AppEventType::ShellButton);
    Lighting_Event.LightingEvent.Action = PWMDevice::INVALID_ACTION;

    if (argc == 1 && strcmp(argv[0], "on") == 0) {
        Lighting_Event.LightingEvent.Action = PWMDevice::ON_ACTION;
    }
    if (argc == 1 && strcmp(argv[0], "off") == 0) {
        Lighting_Event.LightingEvent.Action = PWMDevice::OFF_ACTION;
    }

    if ( Lighting_Event.LightingEvent.Action != PWMDevice::INVALID_ACTION) {
        AppTask::Instance().PostEvent(&Lighting_Event);
    } else {
        streamer_printf(ShellCommands::streamer_get(), "Usage: switch [on|off]");
    }
    
    return CHIP_NO_ERROR;
}

CHIP_ERROR AppTask::DevFunctionCommandHandler(int argc, char ** argv)
{
    AppEvent Event;

    Event.Type    = AppEventType::ShellButton;
    Event.Handler = NULL;

    if (argc == 1 && strcmp(argv[0], "blecom") == 0) {

        Event.Handler = AppTask::Instance().StartBLEAdvertisementHandler;
    } else {
        streamer_printf(ShellCommands::streamer_get(), "Usage: devfunc [blecom]");
    }

    if (Event.Handler) {
        AppTask::Instance().PostEvent(&Event);
    }

    return CHIP_NO_ERROR;
}
#endif

CHIP_ERROR AppTask::Init()
{
    LOG_INF("Init Lighting-app cluster");

    // Initialize lighting device (PWM)
    uint8_t minLightLevel = kDefaultMinLevel;
    Clusters::LevelControl::Attributes::MinLevel::Get(kLightEndpointId, &minLightLevel);

    uint8_t maxLightLevel = kDefaultMaxLevel;
    Clusters::LevelControl::Attributes::MaxLevel::Get(kLightEndpointId, &maxLightLevel);

    int ret = mPWMDevice.Init(NULL, minLightLevel, maxLightLevel, maxLightLevel);
    if (ret != 0) {
        return chip::System::MapErrorZephyr(ret);
    }
    // Register PWM device init and activate callback's
    mPWMDevice.SetCallbacks(ActionInitiated, ActionCompleted);

    CHIP_ERROR err = InitCommonParts();
    if (err != CHIP_NO_ERROR) {
        LOG_ERR("Matter common device Init fail");
        return err;
    }

    // Init modified persistent storage setup
    gExampleDeviceInfoProvider.SetStorageDelegate(&Server::GetInstance().GetPersistentStorage());
    chip::DeviceLayer::SetDeviceInfoProvider(&gExampleDeviceInfoProvider);
    app::SetAttributePersistenceProvider(&gDeferredAttributePersister);

#ifdef CONFIG_CHIP_LIB_SHELL
    ShellCommands::RegisterShellCommands();
#endif // CONFIG_CHIP_LIB_SHELL

    return CHIP_NO_ERROR;
}

void AppTask::IdentifyStartHandler(Identify *)
{
    AppEvent event;
    event.Type    = AppEventType::IdentifyStart;
    event.Handler = [](const AppEvent *) {
        Instance().mPWMDevice.SuppressOutput();
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
        Instance().mPWMDevice.ApplyLevel();
    };
    Instance().PostEvent(&event);
}

void AppTask::LightingActionEventHandler(const AppEvent * event)
{
    if (event->Type != AppEventType::Lighting) {
        return;
    }

    PWMDevice::Action_t action = static_cast<PWMDevice::Action_t>(event->LightingEvent.Action);
    int32_t actor              = event->LightingEvent.Actor;

    LOG_INF("Light state to %d by %d", action, actor);

    if (Instance().mPWMDevice.InitiateAction(action, actor, NULL)) {
        LOG_INF("Action is already in progress or active.");
    }
}

void AppTask::StartBLEAdvertisementHandler(const AppEvent *)
{
    if (Server::GetInstance().GetFabricTable().FabricCount() != 0) {
        LOG_INF("Matter service BLE advertising not started - device is already commissioned");
        return;
    }

    if (ConnectivityMgr().IsBLEAdvertisingEnabled()) {
        LOG_INF("BLE advertising is already enabled");
        return;
    }

    if (Server::GetInstance().GetCommissioningWindowManager().OpenBasicCommissioningWindow() != CHIP_NO_ERROR) {
        LOG_ERR("OpenBasicCommissioningWindow() failed");
    }
}

void AppTask::ActionInitiated(PWMDevice::Action_t action, int32_t actor)
{
    if (action == PWMDevice::ON_ACTION) {
        LOG_INF("Turn On Action has been initiated");
    } else if (action == PWMDevice::OFF_ACTION) {
        LOG_INF("Turn Off Action has been initiated");
    } else if (action == PWMDevice::LEVEL_ACTION) {
        LOG_INF("Level Action has been initiated");
    }
}

void AppTask::ActionCompleted(PWMDevice::Action_t action, int32_t actor)
{
    if (action == PWMDevice::ON_ACTION) {
        LOG_INF("Turn On Action has been completed");
    } else if (action == PWMDevice::OFF_ACTION) {
        LOG_INF("Turn Off Action has been completed");
    } else if (action == PWMDevice::LEVEL_ACTION) {
        LOG_INF("Level Action has been completed");
    }

    if (actor == static_cast<int32_t>(AppEventType::ShellButton)) {
        Instance().UpdateClusterState();
    }
}

void AppTask::UpdateClusterState()
{
    SystemLayer().ScheduleLambda([this] {
        // write the new on/off value
        Protocols::InteractionModel::Status status =
            Clusters::OnOff::Attributes::OnOff::Set(kLightEndpointId, mPWMDevice.IsTurnedOn());

        if (status != Protocols::InteractionModel::Status::Success) {
            LOG_ERR("Updating on/off cluster failed: %x", to_underlying(status));
        }

        // write the current level
        status = Clusters::LevelControl::Attributes::CurrentLevel::Set(kLightEndpointId, mPWMDevice.GetLevel());

        if (status != Protocols::InteractionModel::Status::Success) {
            LOG_ERR("Updating level cluster failed: %x", to_underlying(status));
        }
    });
}
