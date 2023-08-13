/*
 * For factory test mode
 *
 */

#ifndef _REDIAL_FACTORY_H_
#define _REDIAL_FACTORY_H_

#include "ih_cmd.h"
#include "redial.h"

void redial_handle_enter(REDIAL_STATE *state, MODEM_INFO *modem_info, VIF_INFO *vif_info);
void redial_handle_leave(REDIAL_STATE *state, MODEM_INFO *modem_info);
int my_factory_cmd_handle(IH_CMD *cmd);
void my_factory_modem_timer(int fd, short event, void *arg);

extern struct event g_ev_factory_modem_tmr;

#endif
