#include <stdio.h>
#include <string.h>

#include "build_info.h" 
#include "ih_logtrace.h"
#include "license.h"
#include "ih_model_support.h"

MODEL_SUPPORT	model_support_list[] = {
#ifdef INHAND_ER6
    // EN00	
	{ER605_EN00,			"V2.0.1"},
	{ER605_EN00_WLAN,		"V2.0.1"},

	// FQ38
	{ER605_FQ38,			"V2.0.1"},
	{ER605_FQ38_WLAN,		"V2.0.1"},

	// FQ39
	{ER605_FQ39,			"V2.0.1"},
	{ER605_FQ39_WLAN,		"V2.0.1"},

	// FQ58
	{ER605_FQ58, 			"V2.0.1"},
	{ER605_FQ58_WLAN, 		"V2.0.1"},

	// FQ78
	{ER605_FQ78, 			"V2.0.1"},
	{ER605_FQ78_WLAN,		"V2.0.1"},

	// NRQ2
	{ER605_NRQ2,			"V2.0.1"},
	{ER605_NRQ2_WLAN,		"V2.0.1"},
	
	// LF20
	{ER605_LF20,			"V2.0.1"},
	{ER605_LF20_WLAN,		"V2.0.1"},

	// NRQ3
	{ER605_NRQ3,			"V2.0.5"},
	{ER605_NRQ3_WLAN,		"V2.0.5"},

#elif defined INHAND_ER3
	{ODU2002_NAVA_WLAN,		"V2.0.1"},

#endif
};

int upgrading_version_support_current_model(const char *upgrade_version)
{
	int index = 0;
	IH_MODEL model;
	int major_version = 0, medium_version = 0, minor_version = 0;
	int up_major_version = 0, up_medium_version = 0, up_minor_version = 0;
	uint32_t support_fw_version = 0, up_fw_version = 0; 

	model = ih_license_get_model_id();

	for(index = 0; index < sizeof(model_support_list) / sizeof(model_support_list[0]); index++){
		if(model == model_support_list[index].model){
			sscanf(model_support_list[index].fw_version, "%*c%d.%d.%d%*s", &major_version, &medium_version, &minor_version);
			sscanf(upgrade_version, "%*c%d.%d.%d%*s", &up_major_version, &up_medium_version, &up_minor_version);

			support_fw_version = ((major_version & 0xff) << 24) | ((medium_version & 0xff) << 16) | (minor_version & 0xffff);
			up_fw_version = ((up_major_version & 0xff) << 24) | ((up_medium_version & 0xff) << 16) | (up_minor_version & 0xffff);

			if(up_fw_version < support_fw_version){
				goto CHECK_FAILED;
			}

			LOG_IN("valid firmware [%s]", upgrade_version);
			return 0;
		}
	}

	LOG_IN("get model support info failed, upgrade it.");
	return 0;
	
CHECK_FAILED:
	LOG_ER("upgrading version %s don't support current model", upgrade_version);

	return -1;
}
