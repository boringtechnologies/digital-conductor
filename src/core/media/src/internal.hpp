#ifndef MEDIA_INTERNAL_H
#define MEDIA_INTERNAL_H

#include <iostream>
#include <gst/gst.h>

#define MAKE_ELEMENT(var, factory, name)                                    \
    do {                                                                    \
        var = gst_element_factory_make(factory, name);                      \
        if (!var) {                                                         \
            std::cerr << "Failed to find element: " << factory << std::endl;\
        }                                                                   \
    } while (0)

#if GST_USE_STATIC_PLUGINS

extern "C" {
    // base
    GST_PLUGIN_STATIC_DECLARE(coreelements);
    GST_PLUGIN_STATIC_DECLARE(app);
    // good
    GST_PLUGIN_STATIC_DECLARE(videoconvert);
    GST_PLUGIN_STATIC_DECLARE(videorate);
    GST_PLUGIN_STATIC_DECLARE(vpx);
    GST_PLUGIN_STATIC_DECLARE(rtp);
    GST_PLUGIN_STATIC_DECLARE(rtpmanager);
    // bad
    GST_PLUGIN_STATIC_DECLARE(applemedia);
}

#endif

static void gst_register_static_plugins() {
#if GST_USE_STATIC_PLUGINS
    GST_PLUGIN_STATIC_REGISTER(coreelements);
    GST_PLUGIN_STATIC_REGISTER(app);
    GST_PLUGIN_STATIC_REGISTER(videoconvert);
    GST_PLUGIN_STATIC_REGISTER(videorate);
    GST_PLUGIN_STATIC_REGISTER(vpx);
    GST_PLUGIN_STATIC_REGISTER(rtp);
    GST_PLUGIN_STATIC_REGISTER(rtpmanager);
    GST_PLUGIN_STATIC_REGISTER(applemedia);
#endif
}

#endif
