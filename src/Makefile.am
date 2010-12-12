bin_PROGRAMS = pulseaudio-mixer-applet

pulseaudio_mixer_applet_SOURCES = \
	main.c \
	pama-applet.c \
	pama-applet.h \
	pama-popup.c \
	pama-popup.h \
	pama-pulse-client.c \
	pama-pulse-client.h \
	pama-pulse-context.c \
	pama-pulse-context.h \
	pama-pulse-sink.c \
	pama-pulse-sink.h \
	pama-pulse-sink-input.c \
	pama-pulse-sink-input.h \
	pama-pulse-source.c \
	pama-pulse-source.h \
	pama-pulse-source-output.c \
	pama-pulse-source-output.h \
	pama-sink-input-widget.c \
	pama-sink-input-widget.h \
	pama-sink-popup.c \
	pama-sink-popup.h \
	pama-sink-widget.c \
	pama-sink-widget.h \
	pama-source-output-widget.c \
	pama-source-output-widget.h \
	pama-source-popup.c \
	pama-source-popup.h \
	pama-source-widget.c \
	pama-source-widget.h \
	widget-settings.h

pulseaudio_mixer_applet_CFLAGS = $(PULSEAUDIO_MIXER_APPLET_CFLAGS)
pulseaudio_mixer_applet_LDFLAGS = $(PULSEAUDIO_MIXER_APPLET_LDFLAGS)
pulseaudio_mixer_applet_LDADD = $(PULSEAUDIO_MIXER_APPLET_LIBS)

dist_man1_MANS = pulseaudio-mixer-applet.1

uidir   = $(datadir)/gnome-2.0/ui
ui_DATA = PulseAudioMixerApplet.xml

serverdir          = $(libdir)/bonobo/servers
server_in_files    = PulseAudioMixerApplet.server.in
server_in_in_files = $(server_in_files:.server.in=.server.in.in) 
server_DATA        = $(server_in_files:.server.in=.server)

$(server_in_files): $(server_in_in_files) 
	sed -e "s|\@BINDIR\@|$(bindir)|" $< > $@

CLEANFILES = $(server_in_files) $(server_DATA)

EXTRA_DIST = \
	$(server_in_in_files) \
	$(ui_DATA)

@INTLTOOL_SERVER_RULE@


