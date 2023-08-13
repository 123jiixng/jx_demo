#ifndef _REDIAL_MBIM_H__
#define _REDIAL_MBIM_H__

#define MBIM_NETWORK_START_TIMEOUT		12  //s

int build_mbim_config_file(PPP_CONFIG *info, MODEM_INFO *modem_info);
int mbim_start_data_connection_by_cli(char *dev);
int modem_get_net_params_by_at(char *commdev, BOOL init, PPP_CONFIG *info, MODEM_INFO *modem_info, VIF_INFO *pinfo, BOOL verbose);
int clear_mbim_child_processes(void);

#endif
