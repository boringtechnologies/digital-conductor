//******************************************************************************
//******************************************************************************

#include "core/media/util.h"

#include <gst/app/app.h>

#include <iostream>
#include <thread>
#include <map>
#include <mutex>

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

    std::mutex                         m_devicesLocker;
    std::map<std::string, DeviceDescr> m_devices;

    impl();

    void watchThreadFn();

    void addDevice(GstDevice * dev);
    void updateDevice(GstDevice * dev);
    void removeDevice(GstDevice * dev);
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
void Util::impl::addDevice(GstDevice * dev)
{
    gchar * deviceName  = gst_device_get_display_name(dev);
    gchar * deviceClass = gst_device_get_device_class(dev);

    std::cout << "found " << deviceClass << " device " << deviceName << std::endl;

    DeviceDescr descr;
    descr.deviceName = deviceName;
    descr.deviceClass = deviceClass;

    g_free(deviceClass);
    g_free(deviceName);

    std::lock_guard<std::mutex> l(m_devicesLocker);
    m_devices[descr.deviceName] = descr;
}

//******************************************************************************
//******************************************************************************
void Util::impl::updateDevice(GstDevice * dev)
{
    addDevice(dev);
}

//******************************************************************************
//******************************************************************************
void Util::impl::removeDevice(GstDevice * dev)
{
    gchar * deviceName  = gst_device_get_display_name(dev);

    {
        std::lock_guard<std::mutex> l(m_devicesLocker);
        m_devices.erase(deviceName);
    }

    g_free(deviceName);
}

//******************************************************************************
//******************************************************************************
// static
gboolean Util::impl::busMsgHandler(GstBus * bus, GstMessage * msg, gpointer userData)
{
    std::cout << "gst message " << GST_MESSAGE_TYPE_NAME(msg) << std::endl;

    Util::impl * impl = reinterpret_cast<Util::impl *>(userData);

    GstDevice * device = nullptr;
    switch (GST_MESSAGE_TYPE (msg))
    {
        case GST_MESSAGE_DEVICE_ADDED:
            gst_message_parse_device_added(msg, &device);
            impl->addDevice(device);
            gst_object_unref (device);
            break;
        case GST_MESSAGE_DEVICE_REMOVED:
            gst_message_parse_device_removed(msg, &device);
            impl->updateDevice(device);
            gst_object_unref (device);
            break;
        case GST_MESSAGE_DEVICE_CHANGED:
            gst_message_parse_device_changed(msg, &device, NULL);
            impl->removeDevice(device);
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
bool Util::initializeMediaModule(const std::string & pluginsPath)
{
    // gst_init(&argc, &argv);
    gst_init(nullptr, nullptr);

    // TODO instead of GST_PLUGIN_PATH may be use that?
    // GstRegistry * registry = gst_registry_get();
    // gst_registry_scan_path(registry, pluginsPath.c_str());

    return true;
}

//******************************************************************************
//******************************************************************************
// static 
std::vector<DeviceDescr> Util::getDeviceList(const std::string & deviceClass)
{
    std::vector<DeviceDescr> result;

    {
        std::lock_guard<std::mutex> l(m_p->m_devicesLocker);
        for (const std::pair<std::string, DeviceDescr> & p : m_p->m_devices)
        {
            if (deviceClass.empty() || p.second.deviceClass == deviceClass)
            {
                result.emplace_back(p.second);
            }
        }
    }

    return result;
}

} // namespace media
} // namespace dc
