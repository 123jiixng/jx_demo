/*
 * $Id$ --
 *
 *   SYSINFO routines
 *
 * Copyright (c) 2001-2010 InHand Networks, Inc.
 *
 * PROPRIETARY RIGHTS of InHand Networks are involved in the
 * subject matter of this material.  All manufacturing, reproduction,
 * use, and sales rights pertaining to this subject matter are governed
 * by the license agreement.  The recipient of this software implicitly
 * accepts the terms of the license.
 *
 * Creation Date: 06/16/2010
 * Author: Jianliang Zhang
 *
 */

#include <syslog.h>

#include "shared.h"
#include "ih_errno.h"
#include "ih_ipc.h"
#include "if_common.h"

const char *g_cmd_types[] = {"", "no", "default"};

IH_SYSINFO g_sysinfo = {
	DEFAULT_LANG_NAME,
	DEFAULT_HOST_NAME,
	DEFAULT_TIMEZONE,
	DEFAULT_FAMILY_NAME,
	DEFAULT_MODEL_NAME,
	DEFAULT_OEM_NAME,
	DEFAULT_BOOTLOADER,
	DEFAULT_PRODUCT_NUMBER,
	DEFAULT_SERIAL_NUMBER,
	DEFAULT_DESCRIPTION,
	DEFAULT_DOMAIN_NAME,
	DEFAULT_BANNER_LOGIN_NAME,
#ifdef INHAND_IDTU9
	DEFAULT_SECURITY,
	DEFAULT_CONSOLE_INHAND,
#endif
#ifdef NEW_WEBUI
	DEFAULT_SAVE,
#endif
	DEFAULT_ENCRYPT,
	DEFAULT_CLOUD_ENABLED,
    DEFAULT_SFE_ENABLED
};

char g_auto_lang[MAX_LANG_NAME] = DEFAULT_LANG_NAME;
char gl_env_tz[32];
IH_SYS_STATE g_sys_state = IH_SYS_STATE_LOADING;

int sys_state_change(IH_SYS_STATE sys_state)
{
	g_sys_state = sys_state;
	return sys_state_publish();
}

int sys_state_update(IH_SYS_STATE sys_state)
{
	g_sys_state = sys_state;
	return ERR_OK;
}

int sys_state_publish(void)
{
	return ih_ipc_publish(IPC_MSG_SYSTEM_STATE,&g_sys_state,sizeof(g_sys_state));
}

IH_SYS_STATE sys_state(void)
{
	return g_sys_state;
}

void sys_sfe(void)
{
	int ret = 0;

#ifdef INHAND_ER3
	if (g_sysinfo.sfe_enabled) {
		ret = system("hwnat -N 30 &");
	} else {
		ret = system("hwnat -N 65535 &");
	}
#elif defined INHAND_ER8
	ret = system("/etc/init.d/qca-nss-ecm stop");
	if (g_sysinfo.sfe_enabled) {
		ret += system("/etc/init.d/qca-nss-ecm start");
	}
#endif

	LOG_IN("%s sfe state %d", g_sysinfo.sfe_enabled? "start": "stop", ret);
}

int update_timezone(IH_SYSINFO *info)
{
        char *tz = NULL;
        if(NULL==info)  return ERR_INVAL;

        tz = getenv("TZ");
        if (tz && !strcmp(info->timezone, tz))  return 1;

        LOG_DB("Change time zone from %s to %s", g_sysinfo.timezone, info->timezone);
        strlcpy(g_sysinfo.timezone, info->timezone,sizeof(g_sysinfo.timezone));

        gl_env_tz[0] = 'T';
        gl_env_tz[1] = 'Z';
        gl_env_tz[2] = '=';
        gl_env_tz[3] = '\0';
        strcat(gl_env_tz,g_sysinfo.timezone);
        putenv(gl_env_tz);

        return 0;
}

/*
 * update sysinfo from input
 *
 * @param info		new system info
 *
 * @return 0 for not changed, >0 for changed, <0 for error
 */
int update_sysinfo(IH_SYSINFO *info)
{
	int changed = 0;

	if(NULL==info)	return ERR_INVAL;

	if (strcmp(info->lang, g_sysinfo.lang)) {
		LOG_DB("Change lang from %s to %s", g_sysinfo.lang, info->lang);
		strlcpy(g_sysinfo.lang, info->lang,sizeof(g_sysinfo.lang));
		changed++;
	}

	if (strcmp(info->hostname, g_sysinfo.hostname)) {
		LOG_DB("Change hostname from %s to %s", g_sysinfo.hostname, info->hostname);
		strlcpy(g_sysinfo.hostname, info->hostname,sizeof(g_sysinfo.hostname));
		changed++;
	}

	if(!update_timezone(info)) changed++;

	if (strcmp(info->family_name, g_sysinfo.family_name)) {
		LOG_DB("Change family name from %s to %s", g_sysinfo.family_name, info->family_name);
		strlcpy(g_sysinfo.family_name, info->family_name,sizeof(g_sysinfo.family_name));
		changed++;
	}

	if (strcmp(info->model_name, g_sysinfo.model_name)) {
		LOG_DB("Change model name from %s to %s", g_sysinfo.model_name, info->model_name);
		strlcpy(g_sysinfo.model_name, info->model_name,sizeof(g_sysinfo.model_name));
		changed++;
	}

	if (strcmp(info->oem_name, g_sysinfo.oem_name)) {
		LOG_DB("Change oem name from %s to %s", g_sysinfo.oem_name, info->oem_name);
		strlcpy(g_sysinfo.oem_name, info->oem_name,sizeof(g_sysinfo.oem_name));
		changed++;
	}

	if (strcmp(info->bootloader, g_sysinfo.bootloader)) {
		LOG_DB("Change bootloader from %s to %s", g_sysinfo.bootloader, info->bootloader);
		strlcpy(g_sysinfo.bootloader, info->bootloader,sizeof(g_sysinfo.bootloader));
		changed++;
	}

	if (strcmp(info->product_number, g_sysinfo.product_number)) {
		LOG_DB("Change product number from %s to %s", g_sysinfo.product_number, info->product_number);
		strlcpy(g_sysinfo.product_number, info->product_number,sizeof(g_sysinfo.product_number));
		changed++;
	}

	if (strcmp(info->serial_number, g_sysinfo.serial_number)) {
		LOG_DB("Change serial number from %s to %s", g_sysinfo.serial_number, info->serial_number);
		strlcpy(g_sysinfo.serial_number, info->serial_number,sizeof(g_sysinfo.serial_number));
		changed++;
	}

	if (strcmp(info->description, g_sysinfo.description)) {
		LOG_DB( "Change description from %s to %s", g_sysinfo.description, info->description);
		strlcpy(g_sysinfo.description, info->description,sizeof(g_sysinfo.description));
		changed++;
	}

	if (strcmp(info->banner_login, g_sysinfo.banner_login)) {
		LOG_DB( "Change banner from %s to %s", g_sysinfo.banner_login, info->banner_login);
		strlcpy(g_sysinfo.banner_login, info->banner_login,sizeof(g_sysinfo.banner_login));
		changed++;
	}

	if (info->encrypt_passwd != g_sysinfo.encrypt_passwd) {
		LOG_DB( "Change encrypt_passwd from %d to %d", g_sysinfo.encrypt_passwd, info->encrypt_passwd);
		g_sysinfo.encrypt_passwd = info->encrypt_passwd;
		changed++;
	}

	if (info->ipv6_enable != g_sysinfo.ipv6_enable) {
		LOG_DB( "Change IPv6 unicast-routing from %s to %s", ENABLED_STR(g_sysinfo.ipv6_enable), ENABLED_STR(info->ipv6_enable));
		g_sysinfo.ipv6_enable = info->ipv6_enable;
		changed++;
	}

    if (info->sfe_enabled != g_sysinfo.sfe_enabled) {
		LOG_DB( "Change SFE enable from %s to %s", ENABLED_STR(g_sysinfo.sfe_enabled), ENABLED_STR(info->sfe_enabled));
		g_sysinfo.sfe_enabled = info->sfe_enabled;
		changed++;
	}

#ifdef INHAND_IDTU9
	if (info->console_inhand_tool != g_sysinfo.console_inhand_tool) {
		LOG_DB( "Change console_inhand_tool from %d to %d", g_sysinfo.console_inhand_tool, info->console_inhand_tool);
		g_sysinfo.console_inhand_tool = info->console_inhand_tool;
		changed++;
	}
#endif

	return changed;
}
