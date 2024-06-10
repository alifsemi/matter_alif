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

#include "ShellCommands.h"
#include "LightSwitch.h"
#include <platform/CHIPDeviceLayer.h>

#include "BindingHandler.h"

using namespace chip;
using namespace chip::app;

namespace SwitchCommands {
using Shell::Engine;
using Shell::shell_command_t;
using Shell::streamer_get;
using Shell::streamer_printf;

static CHIP_ERROR LightComandHandler(int argc, char ** argv)
{
    if (argc != 1)
    {
        streamer_printf(SwitchCommands::streamer_get(), "Usage: light [on|off|toggle]");
    }

    if (strcmp(argv[0], "on") == 0)
    {
        LightSwitch::GetInstance().LightControl(LightSwitch::Action::On);
    }
    else if (strcmp(argv[0], "off") == 0)
    {
        LightSwitch::GetInstance().LightControl(LightSwitch::Action::Off);
    }
    else if (strcmp(argv[0], "toggle") == 0)
    {
        LightSwitch::GetInstance().LightControl(LightSwitch::Action::Toggle);
    }
    else
    {
        streamer_printf(SwitchCommands::streamer_get(), "Usage: light [on|off|toggle]");
    }

    return CHIP_NO_ERROR;
}

static CHIP_ERROR SwitchComandHandler(int argc, char ** argv)
{
    if (argc != 1)
    {
        streamer_printf(SwitchCommands::streamer_get(), "Usage: switch [on|off|toggle]");
    }

    if (strcmp(argv[0], "on") == 0)
    {
        LightSwitch::GetInstance().GenericSwitchEvent(LightSwitch::SwitchEvent::Pressed);
    }
    else if (strcmp(argv[0], "off") == 0)
    {
        LightSwitch::GetInstance().GenericSwitchEvent(LightSwitch::SwitchEvent::Released);
    }
    else
    {
        streamer_printf(SwitchCommands::streamer_get(), "Usage: switch [on|off|toggle]");
    }

    return CHIP_NO_ERROR;
}

static CHIP_ERROR BindTablePrint(int argc, char ** argv)
{
    BindingHandler::GetInstance().PrintTable();
    return CHIP_NO_ERROR;
}

void RegisterSwitchCommands(void)
{
    static const shell_command_t sLightCommand = {LightComandHandler, "light", "Light test commands. Usage: light [on|off|toggle]" };
    static const shell_command_t sSwitchCommand = {SwitchComandHandler, "switch", "Light switch test commands. Usage: switch [on|off]" };
    static const shell_command_t sTableCommand = {BindTablePrint, "table", "Print light binding table. Usage: table" };

    Engine::Root().RegisterCommands(&sLightCommand, 1);
    Engine::Root().RegisterCommands(&sSwitchCommand, 1);
    Engine::Root().RegisterCommands(&sTableCommand, 1);
    return;
}

} // namespace SwitchCommands
