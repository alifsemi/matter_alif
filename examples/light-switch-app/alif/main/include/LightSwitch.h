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

#pragma once

#include <app/util/basic-types.h>
#include <lib/core/CHIPError.h>

class LightSwitch
{
public:
    enum class Action : uint8_t
    {
        Toggle, /// Switch state on lighting-app device
        On,     /// Turn on light on lighting-app device
        Off     /// Turn off light on lighting-app device
    };

    enum class SwitchEvent : uint8_t
    {
        Pressed,     /// Switch pressed
        Released     /// Switch released
    };

    void Init(chip::EndpointId aLightDimmerSwitchEndpoint, chip::EndpointId aLightGenericSwitchEndpointId);
    void LightControl(Action);
    void LightDimmControl(uint16_t brigtness);
    void GenericSwitchEvent(SwitchEvent);

    static LightSwitch & GetInstance()
    {
        static LightSwitch sLightSwitch;
        return sLightSwitch;
    }

private:
    constexpr static auto kMaximumBrightness                 = 254;

    chip::EndpointId mLightSwitchEndpoint;
    chip::EndpointId mLightGenericSwitchEndpointId;
};
