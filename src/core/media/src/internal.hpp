#ifndef MEDIA_INTERNAL_H
#define MEDIA_INTERNAL_H

#include <iostream>

#define MAKE_ELEMENT(var, factory, name)                                    \
    do {                                                                    \
        var = gst_element_factory_make(factory, name);                      \
        if (!var) {                                                         \
            std::cerr << "Failed to find element: " << factory << std::endl;\
        }                                                                   \
    } while (0)

#endif
