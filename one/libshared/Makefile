#include ../Makefile.inc

EXE_LIB = 1

EXE = $(LIBSHARED_NAME)
OBJS = \
	if_info.o\
	if_common.o \
	aes.o \
        aes_cbc.o \
        base64.o \
        i18n.o \
        md5_share_dgst.o \
        md5_share_one.o \
	hash.o \
        misc.o \
        serialport.o \
        sock_fd.o \
        sysv_ipc.o \
        validate.o \
        files.o \
        ledcon.o \
        lock.o \
        process.o \
        shutils.o \
        strings.o \
        crypto.o \
        watchdog.o \
        rtcapi.o \
        ih_sysinfo.o \
	sysmisc.o \
	ipsuite.o \
	mtdutils.o \
	bootenv.o \
	is_config.o \
	rstp_shared.o \
	lldp_shared.o \
	vif_shared.o \
	ipv6_common.o \
	icmp6.o \
	others.o\
	ih_interface.o\
	g8032d_shared.o\
	hash_md5_sha.o \
	license.o \
	inotify.o \
	serialconf.o \
	mobius_pysdk_check.o \
	ih_events.o \
	nm_events.o \
	ih_factory.o \
	ip_passth.o \
	ih_firmware.o

ifeq ($(PRODUCT), IG5)
OBJS += mobius_pysdk_img.o
endif

ifeq ($(NETCONF), y)
OBJS += ih_sysrepo.o
endif

LDFLAGS+=-shared
ifeq ($(LIBC), MUSL)
LDLIBS = -L$(LIBDIR) -L$(USRLIBDIR) -L$(BASE_LIBDIR) -L$(BASE_USRLIBDIR) -lm -lrt -lresolv -lpthread -ldl -ldl -lpthread -lm
else
LDLIBS = -L$(LIBDIR) -L$(USRLIBDIR) -L$(BASE_LIBDIR) -L$(BASE_USRLIBDIR) -lm -lrt -latomic -lresolv -lpthread -ldl -ldl -lpthread -lm
endif

ifeq ($(LIBC), GLIBC)
	LDLIBS += -lnsl
endif

INCLUDES_PATH += -I$(SYSTEM_PATH)/jansson/src  -I$(SYSTEM_PATH)/sqlite
#INCLUDES_PATH_EX = -I$(TOP_INCLUDE)/ae \
#		   -I$(TOP_SRCDIR)/libdsdt/Include/h/inhand \
#		   -I$(TOP_INCLUDE)/ipc \
#		   -I$(TOP_INCLUDE)

INCLUDES_PATH += -I$(INHAND_PATH)/includes 
INCLUDES_PATH += -I$(SYSTEM_PATH)/sqlite
include ../Makefile.rule
