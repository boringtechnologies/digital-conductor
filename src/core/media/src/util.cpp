//******************************************************************************
//******************************************************************************

#include "core/media/util.h"

#include <gst/app/app.h>

//******************************************************************************
//******************************************************************************
namespace dc
{
namespace media
{

//******************************************************************************
//******************************************************************************
Util::Util()
{

}

//******************************************************************************
//******************************************************************************
// static 
std::vector<std::string> Util::getDeviceList()
{
    std::vector<std::string> result;

    GstDeviceMonitor * monitor = gst_device_monitor_new();
    if (!monitor)
    {
        return result;
    }

    gst_device_monitor_set_show_all_devices(monitor, true);

//    if (!gst_device_monitor_start(monitor))
//    {
//        return result;
//    }

    GList * devices = gst_device_monitor_get_devices(monitor);
    if (!devices)
    {
//        gst_device_monitor_stop(monitor);
        gst_object_unref(monitor);
        return result;
    }

    for (GList * it = devices; it; it = g_list_next(it))
    {
        GstDevice * dev = GST_DEVICE (it->data);

        gchar * name = gst_device_get_display_name(dev);

        GstStructure * s = gst_device_get_properties(dev);

        gchar * descr = nullptr;
        gst_structure_get(s, "device.adapter.description", G_TYPE_STRING, &descr, nullptr);


        g_free(descr);
        g_free(name);
    }

    g_list_free_full(devices, gst_object_unref);

    gst_device_monitor_stop(monitor);
    gst_object_unref(monitor);

    return result;
}

} // namespace media
} // namespace dc
