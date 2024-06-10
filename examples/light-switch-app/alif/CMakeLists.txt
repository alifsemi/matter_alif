#
#    Copyright (c) 2024 Project CHIP Authors
#
#    Licensed under the Apache License, Version 2.0 (the "License");
#    you may not use this file except in compliance with the License.
#    You may obtain a copy of the License at
#
#        http://www.apache.org/licenses/LICENSE-2.0
#
#    Unless required by applicable law or agreed to in writing, software
#    distributed under the License is distributed on an "AS IS" BASIS,
#    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#    See the License for the specific language governing permissions and
#    limitations under the License.
#
cmake_minimum_required(VERSION 3.13.1)

get_filename_component(CHIP_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/third_party/connectedhomeip REALPATH)
get_filename_component(ALIF_COMMON ${CHIP_ROOT}/examples/platform/alif REALPATH)
get_filename_component(LIGHTING_COMMON ${CHIP_ROOT}/examples/lighting-switch-app/lighting-switch-common REALPATH)
get_filename_component(GEN_DIR ${CHIP_ROOT}/zzz_generated/ REALPATH)

list(APPEND ZEPHYR_EXTRA_MODULES ${CHIP_ROOT}/config/alif/chip-module)

if(CONFIG_CHIP_FACTORY_DATA)
set(EXTRA_DTC_OVERLAY_FILE "${CHIP_ROOT}/src/platform/alif/b1_factorydata_partition.overlay")
set(OVERLAY_CONFIG "factory_data.conf")
endif()

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(chip-alif-lighting-switch-example)

include(${CHIP_ROOT}/config/alif/app/enable-gnu-std.cmake)
include(${CHIP_ROOT}/src/app/chip_data_model.cmake)

# if (CONFIG_CHIP_FACTORY_DATA)

# endif()

# -Wmaybe-uninitialized has too many false positives, including on std::optional
# and chip::Optional.  Make it nonfatal.
#
# See https://gcc.gnu.org/bugzilla/show_bug.cgi?id=80635
target_compile_options(app PRIVATE -Werror -Wno-error=maybe-uninitialized)

target_include_directories(app PRIVATE
                           main/include
                           ${LIGHTING_COMMON}
                           ${GEN_DIR}
                           ${GEN_DIR}/app-common
                           ${GEN_DIR}/light-switch-app
                           ${ALIF_COMMON}/util/include
                           ${ALIF_COMMON}/app/include)

target_sources(app PRIVATE
               main/AppTask.cpp
               main/LightSwitch.cpp
               main/ShellCommands.cpp
               main/BindingHandler.cpp
               ${ALIF_COMMON}/util/MatterMain.cpp
               ${ALIF_COMMON}/util/MatterAppTask.cpp)
              

chip_configure_data_model(app
    INCLUDE_SERVER
    ZAP_FILE ${CMAKE_CURRENT_SOURCE_DIR}/../light-switch-common/light-switch-app.zap
)
