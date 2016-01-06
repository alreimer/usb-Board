
# DESTDIR is for non root installs (eg packages, NFS) only!
DESTDIR =

binprefix = 
manprefix = /usr
exec_prefix = $(binprefix)/
sbindir = $(exec_prefix)sbin
mandir = $(manprefix)/share/man
oldmandir = $(manprefix)/man

STRIP ?= strip

#for i386 mashine
TYPE_SYSTEM = .longshine

#HTTPD_DOCUMENT_ROOT = /htdocs
CFLAGS = -I../usr/include

#ifeq ($(TYPE_SYSTEM),.longshine)
CFLAGS = -DNH221 -DNH240 -DLONGSHINE
#endif

ifeq ($(TYPE_SYSTEM),.micronet)
	CFLAGS = -DNH221 -DNH240 -DMICRONET
endif

ifeq ($(TYPE_SYSTEM),.nh240)
	CFLAGS = -DNH221 -DNH240
endif
ifdef M_LARGEFILE
	CFLAGS += -DEXT3
endif
ifdef M_NH220
    CFLAGS += -DNH220
endif

#CFLAGS := -O2 -W -Wall -Wbad-function-cast -Wcast-align -Wpointer-arith -Wcast-qual -Wshadow -Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations -fkeep-inline-functions -Wwrite-strings -Waggregate-return -Wnested-externs -Wtrigraphs $(CFLAGS)
CFLAGS += -static

#LDFLAGS += 

#this is for strip
LDFLAGS = -s
#LDFLAGS = -s -static

OBJS = httpd_scripts.o copy.o copy_CGI.o copy_tbl.o parse_CGI.o parser.o httpd.o
OBJS1 = parser.o pnmd.o
OBJS2 = ide_ioctl.o
OBJS3 = httpd_cfg.o
OBJS4 = httpd_tty.o
OBJS5 = terminal.o term_add.o term_cgi.o term_tbl.o term_parser.o

#CFLAGS := $(CFLAGS) -DHTTPD_DOCUMENT_ROOT=\"$(HTTPD_DOCUMENT_ROOT)\"
#CFLAGS := $(CFLAGS)
#CFLAGS := $(CFLAGS) -DHTTPD_MAX_ACCEPT

INSTALL = install
INSTALL_DATA = $(INSTALL) -m 644
INSTALL_DIR = $(INSTALL) -m 755 -d
INSTALL_PROGRAM = $(INSTALL)

#all: httpd pnmd config_sync ide_ioctl
all: terminal

httpd: $(OBJS)
	${CC} ${LDFLAGS} -o httpd ${LIBS} ${OBJS} ${LDADD}
#	${STRIP} -x --strip-unneeded -R .comment -R .pdr -R .note -R .mdebug.abi32 httpd
#	$(MAKE) CC=$(TARGET_CC) CC_FOR_BUILD="$(HOSTCC)" -C $(HTTPD_DIR)
#	$(STRIP) httpd


pnmd: $(OBJS1)
	${LD} $(LDFLAGS) -o pnmd ${LIBS} ${OBJS1} ${LDADD}

httpd_cfg: $(OBJS3)
	${CC} $(LDFLAGS) -o httpd_cfg ${LIBS} ${OBJS3} ${LDADD}

httpd_tty: $(OBJS4)
	${CC} $(LDFLAGS) -o httpd_tty ${LIBS} ${OBJS4} ${LDADD}

terminal: $(OBJS5)
	${CC} $(LDFLAGS) -o terminal ${LIBS} ${OBJS5} ${LDADD}

ide_ioctl: $(OBJS2)
	${LD} $(LDFLAGS) -o ide_ioctl ${LIBS} ${OBJS2} ${LDADD}

config_sync: checkupgrade.o
	$(LD) $(LDFLAGS) -o $@ ${LIBS} checkupgrade.o ${LDADD}

clean:
	rm -f *.[oa] *.elf *~ core httpd httpd_cfg httpd_tty terminal ide_ioctl config_sync pnmd *.gdb

install: all
#	$(CP) httpd httpd_cfg $(DESTSBIN)
#	$(CP) httpd pnmd config_sync ide_ioctl $(DESTSBIN)
	if [ ! -z $(DESTDIR) ]; then $(INSTALL_DIR) $(DESTDIR) ; fi
	if [ ! -z $(DESTDIR)$(sbindir) ]; then $(INSTALL_DIR) $(DESTDIR)$(sbindir) ; fi
	$(INSTALL_PROGRAM) -D httpd $(DESTDIR)$(sbindir)/httpd
	$(INSTALL_PROGRAM) -D httpd_cfg $(DESTDIR)$(sbindir)/httpd_cfg
	$(INSTALL_PROGRAM) -D httpd_tty $(DESTDIR)$(sbindir)/httpd_tty
	$(INSTALL_PROGRAM) -D terminal $(DESTDIR)$(sbindir)/terminal

