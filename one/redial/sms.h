/*
 *define header file about sms
 */

#ifndef _SMS_H_
#define _SMS_H_

typedef enum SMS_STATE {
	SMS_STATE_UNREAD,
	SMS_STATE_READ,
	SMS_STATE_UNSENT,
	SMS_STATE_SENT,
	SMS_STATE_ALL
}SMS_STATE;

#define SMS_MODE_PDU		0
#define SMS_MODE_TEXT		1

#define SMS_DEFAULT_INTERVAL	30
#if 0
#define SMS_ACL_MAX	10

typedef enum {
	SMS_ACL_DENY,
	SMS_ACL_PERMIT
}SMS_ACL_ACTION;

typedef struct {
	int id;
	SMS_ACL_ACTION action;
	char phone[20];
	int io_msg_enable;
}SMS_ACL;

typedef struct {
	int enable;
	int mode;
	char sms_center[20];
	int interval;

	SMS_ACL acl[SMS_ACL_MAX];
}SMS_CONFIG;
#endif

int sms_init(char *dev, int mode);
int sms_handle_insms(char *dev, int mode, SMS_ACL *acl);
int sms_handle_outsms(char *dev, int mode, SMS *psms);
int pysms_handle_outsms(char *dev, SMS *psms);

#endif//_SMS_H_
