//******************************************************************************
//******************************************************************************

#include "core/media/util.h"

#include <gst/app/app.h>

#include <iostream>
#include <thread>

//******************************************************************************
//******************************************************************************
namespace dc
{
namespace media
{

//******************************************************************************
//******************************************************************************
class Util::impl
{
    friend class Util;

    std::thread        m_watchThread;
    GMainLoop        * m_monitorLoop = nullptr;
    GstDeviceMonitor * m_monitor     = nullptr;
    uint32_t           m_busWatchId  = 0;

    impl();

    void watchThreadFn();

    static gboolean busMsgHandler(GstBus * bus, GstMessage * msg, gpointer userData);
};

//******************************************************************************
//******************************************************************************
Util::impl::impl()
{
    m_watchThread = std::thread(&impl::watchThreadFn, this);
}

//******************************************************************************
//******************************************************************************
void Util::impl::watchThreadFn()
{
    std::cout << "monitor thread started" << std::endl;

    m_monitorLoop = g_main_loop_new(nullptr, false);

    m_monitor = gst_device_monitor_new();
    if (!m_monitor)
    {
        std::cout << "no monitor";
        return;
    }

    gst_device_monitor_set_show_all_devices(m_monitor, true);

    GstBus * bus = gst_device_monitor_get_bus(m_monitor);
    m_busWatchId = gst_bus_add_watch (bus, &impl::busMsgHandler, this);
    gst_object_unref(bus);

    gboolean ret = gst_device_monitor_start(m_monitor);
    if (!ret)
    {
        std::cout << "monitor not started " << ret << std::endl;
        return;
    }

    g_main_loop_run(m_monitorLoop);

//    for (GList * it = devices; it; it = g_list_next(it))
//    {
//        GstDevice * dev = GST_DEVICE (it->data);

//        gchar * name = gst_device_get_display_name(dev);

//        GstStructure * s = gst_device_get_properties(dev);

//        gchar * descr = nullptr;
//        gst_structure_get(s, "device.adapter.description", G_TYPE_STRING, &descr, nullptr);


//        g_free(descr);
//        g_free(name);
//    }

//    g_list_free_full(devices, gst_object_unref);

    std::cout << "monitor thread stopped" << std::endl;
}

//******************************************************************************
//******************************************************************************
// static
gboolean Util::impl::busMsgHandler(GstBus * bus, GstMessage * msg, gpointer userData)
{
    std::cout << "gst message " << GST_MESSAGE_TYPE_NAME(msg) << std::endl;

    GstDevice * device = nullptr;

    switch (GST_MESSAGE_TYPE (msg))
    {
        case GST_MESSAGE_DEVICE_ADDED:
            gst_message_parse_device_added(msg, &device);
            // TODO get data
            gst_object_unref (device);
            break;
        case GST_MESSAGE_DEVICE_REMOVED:
            gst_message_parse_device_removed(msg, &device);
            // TODO get data
            gst_object_unref (device);
            break;
        case GST_MESSAGE_DEVICE_CHANGED:
            gst_message_parse_device_changed(msg, &device, NULL);
            // TODO get data
            gst_object_unref (device);
            break;
        default:
            std::cout << "gst message " << GST_MESSAGE_TYPE_NAME(msg) << std::endl;
            break;
    }

    return true;
}

//******************************************************************************
//******************************************************************************
Util::Util()
    : m_p(new impl)
{

}

//******************************************************************************
//******************************************************************************
// static
Util & Util::instance()
{
    static Util u;
    return u;
}

//******************************************************************************
//******************************************************************************
Util::~Util()
{
    if (m_p->m_watchThread.joinable())
    {
        g_main_loop_quit(m_p->m_monitorLoop);
        m_p->m_watchThread.join();
        gst_device_monitor_stop(m_p->m_monitor);
        gst_object_unref(m_p->m_monitor);
        g_source_remove (m_p->m_busWatchId);
        g_main_loop_unref(m_p->m_monitorLoop);
    }
}

//******************************************************************************
//******************************************************************************
// static 
std::vector<std::string> Util::getDeviceList()
{
    std::vector<std::string> result;


    return result;
}

} // namespace media
} // namespace dc
