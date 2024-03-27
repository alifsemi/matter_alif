/*
 *
 *    Copyright (c) 2025 Project CHIP Authors
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

#include <platform/internal/CHIPDeviceLayerInternal.h>

#include <inet/UDPEndPointImplOpenThread.h>

#include <lib/support/CodeUtils.h>
#include <lib/support/logging/CHIPLogging.h>
#include <platform/ConnectivityManager.h>
#include <platform/Zephyr/InetUtils.h>
#include <platform/internal/BLEManager.h>

#if CHIP_SYSTEM_CONFIG_USE_PLATFORM_MULTICAST_API
#include <inet/UDPEndPointImplSockets.h>
#ifndef CONFIG_ARCH_POSIX
#include <zephyr/net/net_if.h>
#endif
#endif

#include <platform/internal/GenericConnectivityManagerImpl_UDP.ipp>

#if INET_CONFIG_ENABLE_TCP_ENDPOINT
#include <platform/internal/GenericConnectivityManagerImpl_TCP.ipp>
#endif

#if CHIP_DEVICE_CONFIG_ENABLE_CHIPOBLE
#include <platform/internal/GenericConnectivityManagerImpl_BLE.ipp>
#endif

#include <platform/OpenThread/OpenThreadUtils.h>
#include <platform/internal/GenericConnectivityManagerImpl_Thread.ipp>


using namespace ::chip::Inet;
using namespace ::chip::DeviceLayer::Internal;

namespace chip {
namespace DeviceLayer {

namespace {
#if CHIP_SYSTEM_CONFIG_USE_PLATFORM_MULTICAST_API
CHIP_ERROR JoinLeaveMulticastGroup(net_if * iface, const Inet::IPAddress & address,
                                   UDPEndPointImplSockets::MulticastOperation operation)
{
    if (net_if_l2(iface) == &NET_L2_GET_NAME(OPENTHREAD))
    {
        const otIp6Address otAddress = ToOpenThreadIP6Address(address);
        const auto handler = operation == UDPEndPointImplSockets::MulticastOperation::kJoin ? otIp6SubscribeMulticastAddress
                                                                                            : otIp6UnsubscribeMulticastAddress;
        otError error;

        ThreadStackMgr().LockThreadStack();
        error = handler(openthread_get_default_instance(), &otAddress);
        ThreadStackMgr().UnlockThreadStack();

        return MapOpenThreadError(error);
    }

    return CHIP_NO_ERROR;
}
#endif // CHIP_SYSTEM_CONFIG_USE_PLATFORM_MULTICAST_API
} // namespace

ConnectivityManagerImpl ConnectivityManagerImpl::sInstance;

CHIP_ERROR ConnectivityManagerImpl::_Init()
{
    GenericConnectivityManagerImpl_Thread<ConnectivityManagerImpl>::_Init();

#if CHIP_SYSTEM_CONFIG_USE_PLATFORM_MULTICAST_API
    UDPEndPointImplSockets::SetMulticastGroupHandler(
        [](InterfaceId interfaceId, const IPAddress & address, UDPEndPointImplSockets::MulticastOperation operation) {
            if (interfaceId.IsPresent())
            {
                net_if * iface = InetUtils::GetInterface(interfaceId);
                VerifyOrReturnError(iface != nullptr, INET_ERROR_UNKNOWN_INTERFACE);

                return JoinLeaveMulticastGroup(iface, address, operation);
            }

            // If the interface is not specified, join or leave the multicast group on all interfaces.
            for (int i = 1; net_if * iface = net_if_get_by_index(i); i++)
            {
                ReturnErrorOnFailure(JoinLeaveMulticastGroup(iface, address, operation));
            }

            return CHIP_NO_ERROR;
        });
#endif // CHIP_SYSTEM_CONFIG_USE_PLATFORM_MULTICAST_API

    return CHIP_NO_ERROR;
}

void ConnectivityManagerImpl::_OnPlatformEvent(const ChipDeviceEvent * event)
{
    // Forward the event to the generic base classes as needed.
    GenericConnectivityManagerImpl_Thread<ConnectivityManagerImpl>::_OnPlatformEvent(event);
}

} // namespace DeviceLayer
} // namespace chip
