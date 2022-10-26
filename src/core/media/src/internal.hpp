#ifndef MEDIA_INTERNAL_H
#define MEDIA_INTERNAL_H

#include <gst/gst.h>

#include <iostream>

#define MAKE_ELEMENT(var, factory, name)                                     \
    do {                                                                     \
        var = gst_element_factory_make(factory, name);                       \
        if (!var) {                                                          \
            std::cerr << "Failed to find element: " << factory << std::endl; \
        }                                                                    \
    } while (0)

#if GST_USE_STATIC_PLUGINS

extern "C" {
// base
GST_PLUGIN_STATIC_DECLARE(app);
GST_PLUGIN_STATIC_DECLARE(audioconvert);
GST_PLUGIN_STATIC_DECLARE(audioresample);
GST_PLUGIN_STATIC_DECLARE(coreelements);
GST_PLUGIN_STATIC_DECLARE(vorbis);
// good
GST_PLUGIN_STATIC_DECLARE(autodetect);
#if GST_CHECK_VERSION(1, 21, 0)
GST_PLUGIN_STATIC_DECLARE(videoconvertscale);
#else
GST_PLUGIN_STATIC_DECLARE(videoconvert);
GST_PLUGIN_STATIC_DECLARE(videoscale);
#endif
GST_PLUGIN_STATIC_DECLARE(videorate);
GST_PLUGIN_STATIC_DECLARE(vpx);
GST_PLUGIN_STATIC_DECLARE(rtp);
GST_PLUGIN_STATIC_DECLARE(rtpmanager);
#if defined(__APPLE__)
GST_PLUGIN_STATIC_DECLARE(osxaudio);
GST_PLUGIN_STATIC_DECLARE(osxvideo);
// bad
GST_PLUGIN_STATIC_DECLARE(applemedia);
#elif defined(__linux__)
GST_PLUGIN_STATIC_DECLARE(alsa);
GST_PLUGIN_STATIC_DECLARE(pulseaudio);
GST_PLUGIN_STATIC_DECLARE(ximagesink);
GST_PLUGIN_STATIC_DECLARE(xvimagesink);
GST_PLUGIN_STATIC_DECLARE(video4linux2);
#endif
GST_PLUGIN_STATIC_DECLARE(libav);
#ifndef NDEBUG
GST_PLUGIN_STATIC_DECLARE(debug);
GST_PLUGIN_STATIC_DECLARE(debugutilsbad);
#endif
}

#endif

static void gst_register_static_plugins() {
#if GST_USE_STATIC_PLUGINS
    GST_PLUGIN_STATIC_REGISTER(app);
    GST_PLUGIN_STATIC_REGISTER(audioconvert);
    GST_PLUGIN_STATIC_REGISTER(audioresample);
    GST_PLUGIN_STATIC_REGISTER(coreelements);
    GST_PLUGIN_STATIC_REGISTER(vorbis);
    GST_PLUGIN_STATIC_REGISTER(autodetect);
#if GST_CHECK_VERSION(1, 21, 0)
    GST_PLUGIN_STATIC_REGISTER(videoconvertscale);
#else
    GST_PLUGIN_STATIC_REGISTER(videoconvert);
    GST_PLUGIN_STATIC_REGISTER(videoscale);
#endif
    GST_PLUGIN_STATIC_REGISTER(videorate);
    GST_PLUGIN_STATIC_REGISTER(vpx);
    GST_PLUGIN_STATIC_REGISTER(rtp);
    GST_PLUGIN_STATIC_REGISTER(rtpmanager);
#if defined(__APPLE__)
    GST_PLUGIN_STATIC_REGISTER(osxaudio);
    GST_PLUGIN_STATIC_REGISTER(osxvideo);
    GST_PLUGIN_STATIC_REGISTER(applemedia);
#elif defined(__linux__)
    GST_PLUGIN_STATIC_REGISTER(alsa);
    GST_PLUGIN_STATIC_REGISTER(pulseaudio);
    GST_PLUGIN_STATIC_REGISTER(video4linux2);
    GST_PLUGIN_STATIC_REGISTER(ximagesink);
    GST_PLUGIN_STATIC_REGISTER(xvimagesink);
#endif
    GST_PLUGIN_STATIC_REGISTER(libav);
#ifndef NDEBUG
    GST_PLUGIN_STATIC_REGISTER(debug);
    GST_PLUGIN_STATIC_REGISTER(debugutilsbad);
#endif
#endif
}

#endif
