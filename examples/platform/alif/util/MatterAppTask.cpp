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

#include "MatterAppTask.h"
#include "AppTask.h"
#include "BLEManagerImpl.h"

#include <DeviceInfoProviderImpl.h>
#include <app/TestEventTriggerDelegate.h>
#include <app/clusters/identify-server/identify-server.h>
#include <app/clusters/ota-requestor/OTATestEventTriggerHandler.h>
#include <app/server/OnboardingCodesUtil.h>
#include <app/server/Server.h>

#include <zephyr/fs/nvs.h>
#include <zephyr/settings/settings.h>

using namespace ::chip;
using namespace chip::app;
using namespace chip::Credentials;
using namespace ::chip::DeviceLayer;

LOG_MODULE_DECLARE(app, CONFIG_CHIP_APP_LOG_LEVEL);

namespace {
constexpr int kAppEventQueueSize       = 10;

K_MSGQ_DEFINE(sAppEventQueue, sizeof(AppEvent), kAppEventQueueSize, alignof(AppEvent));

bool sIsThreadProvisioned   = false;
bool sIsThreadEnabled       = false;
bool sIsThreadAttached      = false;
bool sHaveBLEConnections    = false;

//Test network key
uint8_t sTestEventTriggerEnableKey[TestEventTriggerDelegate::kEnableKeyLength] = { 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
                                                                                   0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff };
} // namespace


class AppFabricTableDelegate : public chip::FabricTable::Delegate
{
public:
    ~AppFabricTableDelegate() { chip::Server::GetInstance().GetFabricTable().RemoveFabricDelegate(this); }

    /**
     * @brief Initialize module and add a delegation to the Fabric Table.
     *
     * To use the OnFabricRemoved method defined within this class and allow to react on the last fabric removal
     * this method should be called in the application code.
     */
    static void Init()
    {
        static AppFabricTableDelegate sAppFabricDelegate;
        chip::Server::GetInstance().GetFabricTable().AddFabricDelegate(&sAppFabricDelegate);
        k_timer_init(&sFabricRemovedTimer, &OnFabricRemovedTimerCallback, nullptr);
    }

private:
    void OnFabricRemoved(const chip::FabricTable & fabricTable, chip::FabricIndex fabricIndex)
    {
        k_timer_start(&sFabricRemovedTimer, K_MSEC(1000), K_NO_WAIT);
    }

    static void OnFabricRemovedTimerCallback(k_timer * timer)
    {
        if (chip::Server::GetInstance().GetFabricTable().FabricCount() == 0) {
            chip::DeviceLayer::PlatformMgr().ScheduleWork([](intptr_t) {

                // Erase PersistedStorage data and Network credentials
                chip::DeviceLayer::PersistedStorage::KeyValueStoreMgrImpl().DoFactoryReset();
                chip::DeviceLayer::ConnectivityMgr().ErasePersistentInfo();
                // Activate BLE advertisment
                if (!chip::DeviceLayer::ConnectivityMgr().IsBLEAdvertisingEnabled()) {
                    if (CHIP_NO_ERROR == chip::Server::GetInstance().GetCommissioningWindowManager().OpenBasicCommissioningWindow()) {
                        return;
                    }
                }
                ChipLogError(FabricProvisioning, "Could not start BLE advertising");
            });
        }
    }

    inline static k_timer sFabricRemovedTimer;
};

CHIP_ERROR MatterAppTask::StartApp(void)
{
    CHIP_ERROR err = AppTask::Instance().Init();

    if (err != CHIP_NO_ERROR) {
        LOG_ERR("AppTask Init fail");
        return err;
    }

    AppEvent event = {};

    while (true) {
        GetEvent(&event);
        DispatchEvent(&event);
    }
}

CHIP_ERROR MatterAppTask::InitCommonParts()
{
    CHIP_ERROR err;

    // Initialize CHIP server
    LOG_INF("Init Chip Server");
#if CONFIG_CHIP_FACTORY_DATA
    ReturnErrorOnFailure(mFactoryDataProvider.Init());
    SetDeviceInstanceInfoProvider(&mFactoryDataProvider);
    SetDeviceAttestationCredentialsProvider(&mFactoryDataProvider);
    SetCommissionableDataProvider(&mFactoryDataProvider);
    // Read EnableKey from the factory data.
    MutableByteSpan enableKey(sTestEventTriggerEnableKey);
    err = mFactoryDataProvider.GetEnableKey(enableKey);
    if (err != CHIP_NO_ERROR)
    {
        LOG_ERR("mFactoryDataProvider.GetEnableKey() failed. Could not delegate a test event trigger");
        memset(sTestEventTriggerEnableKey, 0, sizeof(sTestEventTriggerEnableKey));
    }
#else
    SetDeviceInstanceInfoProvider(&DeviceInstanceInfoProviderMgrImpl());
    SetDeviceAttestationCredentialsProvider(Examples::GetExampleDACProvider());
#endif
    
    // Init ZCL Data Model and start server
    static CommonCaseDeviceServerInitParams initParams;
    static SimpleTestEventTriggerDelegate sTestEventTriggerDelegate{};
    static OTATestEventTriggerHandler sOtaTestEventTriggerHandler{};
    VerifyOrDie(sTestEventTriggerDelegate.Init(ByteSpan(sTestEventTriggerEnableKey)) == CHIP_NO_ERROR);
    VerifyOrDie(sTestEventTriggerDelegate.AddHandler(&sOtaTestEventTriggerHandler) == CHIP_NO_ERROR);
    (void) initParams.InitializeStaticResourcesBeforeServerInit();
    initParams.testEventTriggerDelegate = &sTestEventTriggerDelegate;
    ReturnErrorOnFailure(chip::Server::GetInstance().Init(initParams));
    AppFabricTableDelegate::Init();

    ConfigurationMgr().LogDeviceConfig();
    PrintOnboardingCodes(chip::RendezvousInformationFlags(chip::RendezvousInformationFlag::kBLE));

    PlatformMgr().AddEventHandler(ChipEventHandler, 0);

    LOG_INF("Start Matter event Loop");
    err = PlatformMgr().StartEventLoopTask();
    if (err != CHIP_NO_ERROR) {
        LOG_ERR("PlatformMgr().StartEventLoopTask() failed");
        return err;
    }

    return CHIP_NO_ERROR;
}

void MatterAppTask::ChipEventHandler(const ChipDeviceEvent * event, intptr_t /* arg */)
{
    switch (event->Type)
    {
    case DeviceEventType::kCHIPoBLEAdvertisingChange:
        sHaveBLEConnections = ConnectivityMgr().NumBLEConnections() != 0;
        LOG_INF("BLE connection state %d", sHaveBLEConnections);
        break;

    case DeviceEventType::kDnssdInitialized:
        LOG_INF("DNS init done");
        break;

    case DeviceEventType::kDnssdRestartNeeded:
        LOG_INF("DNS Restart needed");
        break;

    case DeviceEventType::kThreadConnectivityChange:
        LOG_INF("Thread connectivity change: %d", event->ThreadConnectivityChange.Result);
        if (event->ThreadConnectivityChange.Result == kConnectivity_Established) {
            LOG_INF("Thread connectivity established");
        } else if (event->ThreadConnectivityChange.Result == kConnectivity_Lost) {
            LOG_INF("Thread connectivity lost");
        }
        break;

    case DeviceEventType::kThreadStateChange:
        sIsThreadProvisioned = ConnectivityMgr().IsThreadProvisioned();
        sIsThreadEnabled     = ConnectivityMgr().IsThreadEnabled();
        sIsThreadAttached    = ConnectivityMgr().IsThreadAttached();
        LOG_INF("Thread State Provisioned %d, enabled %d, Atteched %d", sIsThreadProvisioned, sIsThreadEnabled, sIsThreadAttached);
        break;

    case DeviceEventType::kCommissioningComplete:
        LOG_INF("Commission complet node ide %lld, fabid %d", event->CommissioningComplete.nodeId,
                event->CommissioningComplete.fabricIndex);
        break;

    case DeviceEventType::kServiceProvisioningChange:
        LOG_INF("Service Provisioned %d, conf update %d", event->ServiceProvisioningChange.IsServiceProvisioned,
                event->ServiceProvisioningChange.ServiceConfigUpdated);
        break;
    case DeviceEventType::kFailSafeTimerExpired:
        LOG_INF("Commission fail safe timer expiration: fab id %d, NoCinvoked %d, updNoCCmd %d",
                event->FailSafeTimerExpired.fabricIndex, event->FailSafeTimerExpired.addNocCommandHasBeenInvoked,
                event->FailSafeTimerExpired.updateNocCommandHasBeenInvoked);
        break;

    default:
        LOG_INF("Unhandled event types: %d", event->Type);
        break;
    }
}

void MatterAppTask::PostEvent(AppEvent * aEvent)
{
    if (!aEvent) {
        return;
    }
        
    if (k_msgq_put(&sAppEventQueue, aEvent, K_NO_WAIT) != 0) {
        LOG_INF("PostEvent fail");
    }
}

void MatterAppTask::DispatchEvent(const AppEvent * aEvent)
{
    if (!aEvent) {
        return;
    }
    if (aEvent->Handler) {
        aEvent->Handler(aEvent);
    } else {
        LOG_INF("Dropping event without handler");
    }
}

void MatterAppTask::GetEvent(AppEvent * aEvent)
{
    k_msgq_get(&sAppEventQueue, aEvent, K_FOREVER);
}
