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

#include "LightSwitch.h"
#include "AppEvent.h"
#include "BindingHandler.h"

#include <app/server/Server.h>
#include <app/util/binding-table.h>
#include <controller/InvokeInteraction.h>

#include <app/clusters/switch-server/switch-server.h>

#include <app-common/zap-generated/attributes/Accessors.h>

using namespace chip;
using namespace chip::app;
using namespace chip::app::Clusters;

void LightSwitch::Init(chip::EndpointId aLightDimmerSwitchEndpoint, chip::EndpointId aLightGenericSwitchEndpointId)
{
    BindingHandler::GetInstance().Init();
    mLightSwitchEndpoint          = aLightDimmerSwitchEndpoint;
    mLightGenericSwitchEndpointId = aLightGenericSwitchEndpointId;
}

void LightSwitch::LightControl(Action mAction)
{
    BindingHandler::BindingData * data = Platform::New<BindingHandler::BindingData>();
    if (data)
    {
        data->EndpointId = mLightSwitchEndpoint;
        data->ClusterId  = Clusters::OnOff::Id;
        switch (mAction)
        {
        case Action::Toggle:
            data->CommandId = Clusters::OnOff::Commands::Toggle::Id;
            break;
        case Action::On:
            data->CommandId = Clusters::OnOff::Commands::On::Id;
            break;
        case Action::Off:
            data->CommandId = Clusters::OnOff::Commands::Off::Id;
            break;
        default:
            Platform::Delete(data);
            return;
        }
        data->IsGroup = BindingHandler::GetInstance().IsGroupBound();
        DeviceLayer::PlatformMgr().ScheduleWork(BindingHandler::SwitchWorkerHandler, reinterpret_cast<intptr_t>(data));
    }
}

void LightSwitch::LightDimmControl(uint16_t dimmLevel)
{

    BindingHandler::BindingData * data = Platform::New<BindingHandler::BindingData>();
    if (data)
    {
        data->EndpointId = mLightSwitchEndpoint;
        data->CommandId  = Clusters::LevelControl::Commands::MoveToLevel::Id;
        data->ClusterId  = Clusters::LevelControl::Id;

        if (dimmLevel > kMaximumBrightness)
        {
            dimmLevel = kMaximumBrightness;
        }
        data->Value   = (uint8_t) dimmLevel;
        data->IsGroup = BindingHandler::GetInstance().IsGroupBound();
        DeviceLayer::PlatformMgr().ScheduleWork(BindingHandler::SwitchWorkerHandler, reinterpret_cast<intptr_t>(data));
    }
}

void LightSwitch::GenericSwitchEvent(SwitchEvent event)
{
    DeviceLayer::SystemLayer().ScheduleLambda([this, event] {
        uint8_t previousPosition;
        uint8_t newPosition;

        switch (event)
        {
        case SwitchEvent::Pressed:
            previousPosition = 1;
            newPosition      = 1;
            break;

        default:
            previousPosition = 1;
            newPosition      = 0;
            break;
        }

        Clusters::Switch::Attributes::CurrentPosition::Set(mLightGenericSwitchEndpointId, newPosition);
        Clusters::SwitchServer::Instance().OnInitialPress(mLightGenericSwitchEndpointId, previousPosition);
    });
}
