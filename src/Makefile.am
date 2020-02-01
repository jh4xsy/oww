## Process this file with automake to produce Makefile.in

# Needs USB_CFLAGS here else things like musbllnk compile will fail if usb.h
# isn't in the compilers standard include path.
INCLUDES = \
	 $(USB_CFLAGS)\
	$(GTK_CFLAGS)

if DISABLEGUI
bin_PROGRAMS = owwnogui ini2dev

else
bin_PROGRAMS = owwnogui ini2dev oww

oww_SOURCES = support.h support.c ad26.c \
			ad26.h applctn.c applctn.h arne.c arne.h atod20.c atod20.h auxwin.c auxwin.h \
			auxwin_t.h auxwin_un.c barom.h choices.h choices_linux.c client.c client.h \
			convert.c convert.h cwop.c cwop.h devices.c devices.h events.h file.c file.h \
			filesel.c globaldef.h interactive.c interactive.h intl.h launchurl.c lnk.h log.c \
			log.h mainwin.c mainwin.h meteo.c meteo.h mypopt.c mypopt.h omem.c omem.h \
			ownet.h oww.c oww.h oww_trx.c oww_trx.h parseout.c parseout.h process.c \
			process.h progstate.c progstate.h rainint.c rainint.h sendwx.c sendwx.h server.c \
			server.h setup.c setup.h setupd.c setupd.h setupp.c setupp.h stats.c stats.h nstats.c nstats.h \
			swt12.c swt12.h swt1f.c swt1f.h tai8570.c tai8570.h txtserve.c txtserve.h \
			url_get.h utility.c utility.h weather.c weather.h werr.h 		werr_linux.c wstypes.h \
	ds2480ut.c ds2480.h mcrcutil.c mlinuxlnk.c mowllu.c 		mownetu.c mowsesu.c \
	mowtrnu.c multilnk.c multinet.c multises.c multitran.c 		owerr.c tai8590.c \
	thermometer.c barometer.c hygrochron.c url_get_curl.c ad30.c ad30.h \
	wsi603a.c wsi603a.h sllist.c sllist.h hobbyboards_uv.c hobbyboards_uv.h \
	thermocouple.c thermocouple.h hobbyboards_moist.c hobbyboards_moist.h

EXTRA_oww_SOURCES = \
	musbllnk.c libusbds2490.c libusbds2490.h musblnet.c musblses.c musbltran.c

oww_DEPENDENCIES = @USB_OBJ@

# need to figure out how to get -Wall for gcc and -w for other compilers
oww_CFLAGS =\
	 @LIBCURL_CPPFLAGS@ \
	 -DPACKAGE_STATE_DIR=\""$(PACKAGE_STATE_DIR)"\" \
	 -DSYSCONFDIR=\""$(owwsysconfdir)"\"

oww_LDFLAGS = -lm


oww_LDADD = $(USB_LIBS) @LIBCURL@ $(GTK_LIBS) @USB_OBJ@
endif

owwnogui_SOURCES = \
	ad26.c ad26.h applctn.c applctn.h arne.c arne.h atod20.c \
	atod20.h barom.h choices.h choices_linux.c client.c client.h convert.c convert.h \
	cwop.c cwop.h devices.c devices.h events.h file.c file.h globaldef.h \
	interactive.c interactive.h intl.h launchurl.c lnk.h log.c log.h mainwin.h meteo.c \
	meteo.h mypopt.c mypopt.h omem.c omem.h ownet.h oww.c oww.h oww_trx.c oww_trx.h \
	parseout.c parseout.h process.c process.h progstate.c progstate.h rainint.c \
	rainint.h sendwx.c sendwx.h server.c server.h setup.c setup.h setupp.c setupp.h \
	stats.c stats.h nstats.c nstats.h swt12.c swt12.h swt1f.c swt1f.h tai8570.c tai8570.h txtserve.c \
	txtserve.h url_get.h url_get_curl.c utility.c utility.h weather.c weather.h \
	werr.h werr_linux.c wstypes.h ds2480ut.c ds2480.h mcrcutil.c \
	mlinuxlnk.c mowllu.c mownetu.c mowsesu.c mowtrnu.c multilnk.c multinet.c multises.c \
	multitran.c owerr.c tai8590.c thermometer.c thermometer.h barometer.c \
	barometer.h hygrochron.c hygrochron.h ad30.c ad30.h \
	wsi603a.c wsi603a.h sllist.c sllist.h hobbyboards_uv.c hobbyboards_uv.h \
	thermocouple.c thermocouple.h hobbyboards_moist.c hobbyboards_moist.h

EXTRA_owwnogui_SOURCES = \
	musbllnk.c libusbds2490.c libusbds2490.h musblnet.c musblses.c musbltran.c

owwnogui_DEPENDENCIES = @USB_OBJ@

# need to figure out how to get -Wall for gcc and -w for other compilers
owwnogui_CFLAGS = \
	 -DNOGUI\
	 @LIBCURL_CPPFLAGS@ \
	 -DPACKAGE_STATE_DIR=\""$(PACKAGE_STATE_DIR)"\" \
	 -DSYSCONFDIR=\""$(owwsysconfdir)"\"
         
# added GTK_LIBS as a hack on solaris to get -lintl
owwnogui_LDADD = $(USB_LIBS) @LIBCURL@ $(GTK_LIBS) @USB_OBJ@

owwnogui_LDFLAGS = -lm 

ini2dev_SOURCES = \
	ini2dev.c

ini2dev_LDADD =
noinst_HEADERS = tai8590.h thermometer.h barometer.h hygrochron.h ad30.h
