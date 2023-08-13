#include <syslog.h>
#include <errno.h>
#include <stdio.h>
#include <string.h> 
#include "ih_logtrace.h"
#include "ih_sysrepo.h"


IH_SYSREPO_SRV  ih_sysrepo_srv;

struct lyd_node *ih_lyd_new_path(struct lyd_node *data_tree, const struct ly_ctx *ctx, const char *path, char *value,
               LYD_ANYDATA_VALUETYPE value_type, int options)
{
    struct lyd_node *ret_node = NULL;

    if (!data_tree) {
        syslog(LOG_INFO, "data_tree is null");
    }

    ret_node = lyd_new_path(data_tree, ctx, path, value, value_type, options);
    if (ret_node) {
        syslog(LOG_INFO, "add new path [%s=%s]",  path, (value && ((char *)value)[0])? (char *)value: "null");
        return ret_node;
    }

    syslog(LOG_INFO, "add new path [%s=%s] failed", path, (value && ((char *)value)[0])? (char *)value: "null");

    return NULL;
}

/*
 *get the root key node of one list xpath node.
 * eg: we want get the interface name of the following xpath node 
        /ietf-interfaces:interfaces/interface/ipv4/address/prefix-length
        the name xpath is /ietf-interfaces:interfaces/interface/name
    so, key_parent_name = interface, key_node_name = name
 * */
struct lyd_node *ih_sysrepo_get_root_key_node(const struct lyd_node *node, char *key_parent_name, char *key_node_name)
{
    int times = 0;
    struct lyd_node *node_parent = NULL;

    if (!node || !key_parent_name || !key_node_name) {
        syslog(LOG_INFO, "%s--%d, parameters error!", __func__, __LINE__);
        return NULL;
    }

    node_parent = node->parent;

    while (node_parent && times < 10) {    //Max 10 times
        if (!strcmp(key_parent_name, node_parent->schema->name) && !strcmp(key_node_name, node_parent->child->schema->name)) {
            return node_parent->child;
        }

        times++;
        node_parent = node_parent->parent;
    }

    syslog(LOG_INFO, "%s--%d get %s/%s failed!", __func__, __LINE__, key_parent_name, key_node_name);
    return NULL;
}

//eg getting: "/ietf-interfaces:interfaces/interface[name=\"<ifname>\"]/enabled"
static int _ih_sr_get_items_by_xpath(sr_session_ctx_t *session, const char *xpath, int database, sr_val_t **values, size_t *value_cnt)
{
	int ret = SR_ERR_NOMEM;
    sr_datastore_t current_ds;
	int i;

    *value_cnt = 0;

    current_ds = sr_session_get_ds(session);

	if (current_ds != database) {
		sr_session_switch_ds(session, database);
	}

	ret = sr_get_items(session, xpath, 0, 0, values, value_cnt);
    if (ret == SR_ERR_OK && (*value_cnt > 0) && values[0]) {
        for (i = 0; i < *value_cnt; i++) {
            syslog(LOG_DEBUG, "get [%s] value [%d]:[%s]", xpath, i, sr_val_to_str(values[0]));
		}
        syslog(LOG_DEBUG, "get [%s] value success", xpath);
		ret = SR_ERR_OK;
	} else {
		ret = SR_ERR_NOMEM;
	}

	if (current_ds != database) {
		sr_session_switch_ds(session, current_ds);
	}

    if (ret != SR_ERR_OK) {
        syslog(LOG_DEBUG, "get [%s] value failed, ret %d", xpath, ret);
    }

	return ret;
}

int ih_sr_get_config_value_by_xpath(sr_session_ctx_t *session, const char *xpath, sr_val_t **values, size_t *value_cnt)
{
	return _ih_sr_get_items_by_xpath(session, xpath, SR_DS_RUNNING, values, value_cnt);
}

static int _ih_sysrepo_subscribe(IH_SYSREPO_SUB *subscribe)
{
    int rc;

    /* subscribe for providing state data */
    if (!subscribe || !subscribe->mod_name || !subscribe->mod_name[0] 
        || !subscribe->xpath || !subscribe->xpath[0] || !subscribe->callback) {
        EINT;
        goto error;
    }

    switch (subscribe->type) {
        case IH_SYSREPO_SUB_CONFIG:
            //FIXME, maybe no need
            rc = sr_session_switch_ds(ih_sysrepo_srv.sr_sess, SR_DS_RUNNING);
            if (rc != SR_ERR_OK) {
                syslog(LOG_INFO, "%s -- switch sesion to running failed", __func__);
                return SR_ERR_INTERNAL;
            }

            IH_SYSREPO_CONFIG_SUB(subscribe->mod_name, subscribe->xpath, subscribe->callback);
            break;

        case IH_SYSREPO_SUB_OPER:
            //FIXME, maybe no need
            rc = sr_session_switch_ds(ih_sysrepo_srv.sr_sess, SR_DS_OPERATIONAL);
            if (rc != SR_ERR_OK) {
                syslog(LOG_INFO, "%s -- switch sesion to operational failed", __func__);
                return SR_ERR_INTERNAL;
            }

            IH_SYSREPO_OPER_SUB(subscribe->mod_name, subscribe->xpath, subscribe->callback);
            break;

        case IH_SYSREPO_SUB_RPC:
            IH_SYSREPO_RPC_SUB(subscribe->xpath, subscribe->callback);
            break;

        default:
            LOG_ER("invalide sysrepo register type %d", subscribe->type);
            goto error;
    }

    LOG_IN("Sysrepo subscribe [type : %d] success.", subscribe->type);
    return 0;

error:
    LOG_ER("Sysrepo subscribe [type : %d] failed.", subscribe->type);
    return -1;

}

int ih_sysrepo_subscribe(IH_SYSREPO_SUB *sr_subscribe, unsigned int num)
{
    IH_SYSREPO_SUB *p = NULL;
    int i = 0;

    if (ih_sysrepo_srv.sr_data_sub) {
        EINT;
        return -1;
    }
    
    p = sr_subscribe;
    for (i = 0; i < num; i++) {
        if((p + i)->mod_name && (p + i)->xpath 
           && (p + i)->type >= 0 && (p + i)->callback)
            _ih_sysrepo_subscribe(p + i);
    }

    return 0;

}

int ih_sysrepo_init(IH_SYSREPO_SUB *sr_subscribe, unsigned int num)
{
    int rc;

    /* init sysrepo context */
    memset(&ih_sysrepo_srv, 0, sizeof(ih_sysrepo_srv));

    /* connect to the sysrepo and set edit-config NACM diff check callback */
    rc = sr_connect(SR_CONN_CACHE_RUNNING, &ih_sysrepo_srv.sr_conn);
    if (rc != SR_ERR_OK) {
        LOG_ER("Connecting to sysrepo failed (%s).", sr_strerror(rc));
        goto error;
    }

    /* server session */
    rc = sr_session_start(ih_sysrepo_srv.sr_conn, SR_DS_RUNNING, &ih_sysrepo_srv.sr_sess);
    if (rc != SR_ERR_OK) {
        LOG_ER("Creating sysrepo session failed (%s).", sr_strerror(rc));
        goto error;
    }

    ih_sysrepo_subscribe(sr_subscribe, num);

    return 0;

error:
    LOG_ER("Server init failed.");
    if(ih_sysrepo_srv.sr_conn){
        sr_disconnect(ih_sysrepo_srv.sr_conn);
        ih_sysrepo_srv.sr_conn = NULL;
    }
    return -1;
}

int ih_sysrepo_cleanup()
{
    int ret;

    LOG_IN("Cleanup sysrepo resources!!!");
    
    if(ih_sysrepo_srv.sr_data_sub != NULL){
        sr_unsubscribe(ih_sysrepo_srv.sr_data_sub);
        ih_sysrepo_srv.sr_data_sub = NULL;
    }

    if(ih_sysrepo_srv.sr_rpc_sub != NULL){
        sr_unsubscribe(ih_sysrepo_srv.sr_rpc_sub);
        ih_sysrepo_srv.sr_rpc_sub = NULL;
    }
   
    if(ih_sysrepo_srv.sr_notif_sub != NULL){
        sr_unsubscribe(ih_sysrepo_srv.sr_notif_sub);
        ih_sysrepo_srv.sr_notif_sub = NULL;
    }
   
    if(ih_sysrepo_srv.sr_sess != NULL){
        ret = sr_session_stop(ih_sysrepo_srv.sr_sess);
        if (ret != SR_ERR_OK) {
            LOG_ER("Close sysrepo session failed (%s).", sr_strerror(ret));
            goto error;
        }
        ih_sysrepo_srv.sr_sess = NULL;
    }

    if(ih_sysrepo_srv.sr_conn != NULL){
        ret = sr_disconnect(ih_sysrepo_srv.sr_conn);
        if(ret != SR_ERR_OK){
            LOG_ER("Disconnecting to sysrepo failed (%s).", sr_strerror(ret));
            goto error;
        }
        ih_sysrepo_srv.sr_conn = NULL;
    }

    return 0;

error:
    LOG_ER("Sysrepo cleanup failed.");
    return -1;
}
