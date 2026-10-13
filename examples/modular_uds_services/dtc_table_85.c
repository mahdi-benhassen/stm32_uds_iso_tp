/**
 * @file dtc_table_85.c
 * @brief 85 Standard Diagnostic Trouble Codes (DTC) from Specification
 */

#include "dtc_table_85.h"
#include "dtc_store.h"

const dtc_config_entry_t g_dtc_config_85[DTC_CONFIGURED_COUNT] = {
    { 0xF00614U, "ASW_DTC_BatteryVoltHighWarn", "U300614" },
    { 0xF00615U, "ASW_DTC_BatteryVoltLowWarn", "U300615" },
    { 0xD00616U, "ASW_DTC_BatteryVoltHigh", "C100616" },
    { 0xD00617U, "ASW_DTC_BatteryVoltLow", "C100617" },
    { 0xD00618U, "ASW_DTC_CanBusOff", "C100618" },
    { 0xD00619U, "ASW_DTC_SysPrs_SigLost", "C100619" },
    { 0xD00620U, "ASW_DTC_SysPrs_OverLimit", "C100620" },
    { 0xD00621U, "ASW_DTC_SysPrs_JumpErr", "C100621" },
    { 0xD00622U, "ASW_DTC_AccuPrs_SigLost", "C100622" },
    { 0xD00623U, "ASW_DTC_AccuPrs_OverLimit", "C100623" },
    { 0xD00624U, "ASW_DTC_AccuPrs_JumpErr", "C100624" },
    { 0xD00625U, "ASW_DTC_PfsPrs_SigLost", "C100625" },
    { 0xD00626U, "ASW_DTC_PfsPrs_OverLimit", "C100626" },
    { 0xD00627U, "ASW_DTC_PfsPrs_JumpErr", "C100627" },
    { 0xD00628U, "ASW_DTC_PipeLineErrFL", "C100628" },
    { 0xD00629U, "ASW_DTC_PipeLineErrFR", "C100629" },
    { 0xD00630U, "ASW_DTC_PipeLineErrRL", "C100630" },
    { 0xD00631U, "ASW_DTC_PipeLineErrRR", "C100631" },
    { 0xD00632U, "ASW_DTC_WssFL_Err", "C100632" },
    { 0xD00633U, "ASW_DTC_WssFR_Err", "C100633" },
    { 0xD00634U, "ASW_DTC_WssRL_Err", "C100634" },
    { 0xD00635U, "ASW_DTC_WssRR_Err", "C100635" },
    { 0xD00636U, "ASW_DTC_AccuLowPrs_Err", "C100636" },
    { 0xD00637U, "ASW_DTC_AccuPrsHoldErr", "C100637" },
    { 0xD00638U, "ASW_DTC_PconErr", "C100638" },
    { 0xD00639U, "ASW_DTC_AccuPump_DG", "C100639" },
    { 0xD00640U, "ASW_DTC_PumpMotLockedRotFault", "C100640" },
    { 0xD00641U, "ASW_DTC_AccuPump_Err", "C100641" },
    { 0xD00642U, "ASW_DTC_PTS_Err", "C100642" },
    { 0xD00643U, "ASW_DTC_PTS_Chanl_1Err", "C100643" },
    { 0xD00644U, "ASW_DTC_PTS_Chanl_2Err", "C100644" },
    { 0xD00645U, "ASW_DTC_MC_OutleakFault", "C100645" },
    { 0xD00646U, "ASW_DTC_MC_InleakFault", "C100646" },
    { 0xD00647U, "ASW_DTC_IMUyawrateSignalErrLevel", "C100647" },
    { 0xD00648U, "ASW_DTC_IMUaySignalErrLevel", "C100648" },
    { 0xD00649U, "ASW_DTC_IMUaxSignalErrLevel", "C100649" },
    { 0xD00650U, "ASW_DTC_SASSignalErrLevel", "C100650" },
    { 0xD00651U, "ASW_DTC_BrkConValveDriverFault", "C100651" },
    { 0xD00652U, "ASW_DTC_WhelPrsConValveDriverFault", "C100652" },
    { 0xD00653U, "ASW_DTC_ISV1_Fault", "C100653" },
    { 0xD00654U, "ASW_DTC_ISV1_OverTempWarn", "C100654" },
    { 0xD00655U, "ASW_DTC_ISV2_Fault", "C100655" },
    { 0xD00656U, "ASW_DTC_ISV2_OverTempWarn", "C100656" },
    { 0xD00657U, "ASW_DTC_ISV3_Fault", "C100657" },
    { 0xD00658U, "ASW_DTC_ISV3_OverTempWarn", "C100658" },
    { 0xD00659U, "ASW_DTC_ISV4_Fault", "C100659" },
    { 0xD00660U, "ASW_DTC_ISV4_OverTempWarn", "C100660" },
    { 0xD00661U, "ASW_DTC_PAV_Fault", "C100661" },
    { 0xD00662U, "ASW_DTC_PAV_OverTempWarn", "C100662" },
    { 0xD00663U, "ASW_DTC_PRV_Fault", "C100663" },
    { 0xD00664U, "ASW_DTC_PRV_OverTempWarn", "C100664" },
    { 0xD00665U, "ASW_DTC_BAV_Fault", "C100665" },
    { 0xD00666U, "ASW_DTC_CSV_Fault", "C100666" },
    { 0xD00667U, "ASW_DTC_SSV_Fault", "C100667" },
    { 0xD00668U, "ASW_DTC_USV_Fault", "C100668" },
    { 0xD00669U, "ASW_DTC_OSV1_Fault", "C100669" },
    { 0xD00670U, "ASW_DTC_OSV2_Fault", "C100670" },
    { 0xD00671U, "ASW_DTC_OSV3_Fault", "C100671" },
    { 0xD00672U, "ASW_DTC_OSV4_Fault", "C100672" },
    { 0xD00673U, "ASW_DTC_SASangErr", "C100673" },
    { 0xD00674U, "ASW_DTC_BrkSwitchErr", "C100674" },
    { 0xD00675U, "ASW_DTC_PowerSysErr", "C100675" },
    { 0xD00676U, "ASW_DTC_AccPosErr", "C100676" },
    { 0xD00677U, "ASW_DTC_GearPosErr", "C100677" },
    { 0xD00678U, "ASW_DTC_BtrErr", "C100678" },
    { 0xD00679U, "ASW_DTC_BrkFluidLevel_Low", "C100679" },
    { 0xD00680U, "ASW_DTC_SeatBltErr", "C100680" },
    { 0xD00681U, "ASW_DTC_DoorsErr", "C100681" },
    { 0xD00682U, "ASW_DTC_AdasConnectErr", "C100682" },
    { 0xD00683U, "ASW_DTC_FrontCameraErr", "C100683" },
    { 0xD00684U, "ASW_DTC_ExternalEPBErr", "C100684" },
    { 0xD00685U, "ASW_DTC_PumpMot_DiverShortCircuit", "C100685" },
    { 0xD00686U, "ASW_DTC_PumpMot_ShortCircuit", "C100686" },
    { 0xD00687U, "ASW_DTC_Motor_OpenL", "C100687" },
    { 0xD00688U, "ASW_DTC_Motor_OpenR", "C100688" },
    { 0xD00689U, "ASW_DTC_Motor_ShortL", "C100689" },
    { 0xD00690U, "ASW_DTC_Motor_OverCurrentL", "C100690" },
    { 0xD00691U, "ASW_DTC_Motor_ShortR", "C100691" },
    { 0xD00692U, "ASW_DTC_Motor_OverCurrentR", "C100692" },
    { 0xD00693U, "ASW_DTC_Motor_TimeOverL", "C100693" },
    { 0xD00694U, "ASW_DTC_Motor_TimeOverR", "C100694" },
    { 0xD00695U, "ASW_DTC_Motor_BrokenL", "C100695" },
    { 0xD00696U, "ASW_DTC_Motor_BrokenR", "C100696" },
    { 0xD00697U, "ASW_DTC_Motor_PistonOverstrokeL", "C100697" },
    { 0xD00698U, "ASW_DTC_Motor_PistonOverstrokeR", "C100698" },
};

void dtc_store_init_85_dtcs(void) {
    dtc_store_init();
    for (uint32_t i = 0U; i < DTC_CONFIGURED_COUNT; i++) {
        /* Initial status: 0x50 (testNotCompletedSinceLastClear | testNotCompletedThisOperationCycle) */
        dtc_store_add(g_dtc_config_85[i].dtc, 0x50U, NULL, 0U, NULL, 0U);
    }
}

void dtc_table_85_init(void) {
    dtc_store_init_85_dtcs();
}
