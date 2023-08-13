EXE = redial

OBJS = ih_redial.o modem.o ppp.o operator.o sms.o pdu.o mbim.o redial_factory.o

ifeq ($(NETCONF), y)
OBJS += redial_sysrepo.o
endif

ifeq ($(CLOUD_WEB), y)
OBJS += redial_cloud_web.o
else ifeq ($(NEW_WEBUI), y)
OBJS += redial_web.o
endif

LDLIBS += $(LIB_IPC) $(LIB_SHARE) $(LIB_EVENT) -lpthread -ldl

ifeq ($(NEW_WEBUI), y)
LDLIBS += -L$(SYSTEM_PATH)/mosquitto/lib -lmosquitto -lmosq -lssl -lcares -lcrypto -luuid -ljansson -lm
INCLUDES_PATH += -I$(SYSTEM_PATH)/mosquitto/lib/ -I$(SYSTEM_PATH)/jansson/src -I$(SYSTEM_PATH)/mosquitto/include
endif

include ../Makefile.rule
