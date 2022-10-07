//******************************************************************************
//******************************************************************************

#include "core/media/util.h"

#include <gst/app/app.h>

#include <iostream>
#include <thread>
#include <map>
#include <mutex>
#include <chrono>
#include <set>

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

    std::mutex                             m_devicesDirectLocker;
    std::chrono::system_clock::time_point  m_lastDevicesRequested;
    std::map<std::string, DeviceDescr>     m_devicesDirect;

    impl();

    void watchThreadFn();

    void addDevice(GstDevice * dev);
    void updateDevice(GstDevice * dev);
    void removeDevice(GstDevice * dev);

    void getDeviceParams(GstDevice * dev,
                         std::string & deviceName,
                         std::vector<std::pair<std::string, std::string> > & params);

    static gboolean busMsgHandler(GstBus * bus, GstMessage * msg, gpointer userData);
};

//******************************************************************************
//******************************************************************************
Util::impl::impl()
    : m_lastDevicesRequested(std::chrono::system_clock::now() - std::chrono::seconds(60))
{
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

    DeviceDescr descr;
    descr.deviceName = deviceName;
    descr.deviceClass = deviceClass;

    getDeviceParams(dev, descr.internalName, descr.internalParams);

//    std::cout << "found " << deviceClass << " device " << deviceName << std::endl;
//    std::cout << "    " << descr.internalName;
    if (descr.internalParams.empty())
    {
        std::cout << std::endl;
    }
    else
    {
//        std::cout << " params" << std::endl;
//        for (uint32_t i = 0; i < descr.internalParams.size(); ++i)
//        {
//            std::cout << "        "
//                      << descr.internalParams[i].first
//                      << ":"
//                      << descr.internalParams[i].second
//                      << std::endl;
//        }
    }

    g_free(deviceClass);
    g_free(deviceName);

    std::lock_guard<std::mutex> l(m_devicesLocker);
    m_devices[descr.deviceClass + descr.deviceName] = descr;
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
void Util::impl::getDeviceParams(GstDevice * dev,
                                 std::string & deviceName,
                                 std::vector<std::pair<std::string, std::string> > & params)

{
    static std::vector<const char *> ignoredPropnames = { "name", "parent", "direction", "template", "caps"};

    GstElement * element = gst_device_create_element(dev, nullptr);
    if (!element)
    {
        return;
    }

    GstElementFactory * factory = gst_element_get_factory(element);
    if (!factory)
    {
        gst_object_unref(element);
        return;
    }

    gchar * objectName = gst_plugin_feature_get_name(factory);
    if (!objectName)
    {
        gst_object_unref(element);
        return;
    }

    deviceName = objectName;

    // TODO need to free?
    // g_string_free(objectName, true);

    GstElement * pureelement = gst_element_factory_create(factory, NULL);

    // get paramspecs and show non-default properties

    guint count = 0;
    GParamSpec ** properties = g_object_class_list_properties(G_OBJECT_GET_CLASS(element), &count);

    if (properties)
    {
        for (guint i = 0; i < count; ++i)
        {
            GParamSpec * property = properties[i];

            // skip some properties
            if ((property->flags & G_PARAM_READWRITE) != G_PARAM_READWRITE)
            {
                continue;
            }

            gboolean ignore = false;
            for (gint j = 0; j < ignoredPropnames.size(); ++j)
            {
                if (!g_strcmp0(ignoredPropnames[j], property->name))
                ignore = true;
                break;
            }

            if (ignore)
            {
                continue;
            }

            // Can't use _param_value_defaults () because sub-classes modify the
            // values already.
            GValue value = G_VALUE_INIT;
            g_value_init(&value, property->value_type);

            GValue pvalue = G_VALUE_INIT;
            g_value_init(&pvalue, property->value_type);

            g_object_get_property(G_OBJECT(element),     property->name, &value);
            g_object_get_property(G_OBJECT(pureelement), property->name, &pvalue);

            if (gst_value_compare (&value, &pvalue) != GST_VALUE_EQUAL)
            {
                gchar * valuestr = gst_value_serialize(&value);

                if (!valuestr)
                {
                    std::cout << "Could not serialize property " << GST_OBJECT_NAME(element) << ":" << property->name << std::endl;
                    g_free(valuestr);
                }
                else
                {
                    params.emplace_back(property->name, valuestr);
                    g_free(valuestr);
                }
            }

            g_value_unset (&value);
            g_value_unset (&pvalue);
        }
        g_free(properties);
    }

    gst_object_unref(element);
    gst_object_unref(pureelement);
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

    // TODO instead of GST_PLUGIN_PATH may be use this code?
    // GstRegistry * registry = gst_registry_get();
    // gst_registry_scan_path(registry, pluginsPath.c_str());

    return true;
}

//******************************************************************************
//******************************************************************************
void Util::startWatchDevices()
{
    m_p->m_watchThread = std::thread(&impl::watchThreadFn, m_p.get());
}

//******************************************************************************
//******************************************************************************
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

//******************************************************************************
//******************************************************************************
std::vector<DeviceDescr> Util::getDeviceListDirect(const std::string & deviceClass)
{
    std::lock_guard<std::mutex> l(m_p->m_devicesDirectLocker);

    while (m_p->m_lastDevicesRequested + std::chrono::milliseconds(5000) < std::chrono::system_clock::now())
    {
        GstDeviceMonitor * monitor = gst_device_monitor_new();
        if (!monitor)
        {
            std::cout << "no monitor";
            break;
        }

        gst_device_monitor_set_show_all_devices(monitor, true);

        gboolean ret = gst_device_monitor_start(monitor);
        if (!ret)
        {
            std::cout << "monitor not started " << ret << std::endl;
            break;
        }

        GList * devices = gst_device_monitor_get_devices(monitor);
        for (GList * it = devices; it; it = g_list_next(it))
        {
            GstDevice * dev = GST_DEVICE(it->data);

            gchar * devName  = gst_device_get_display_name(dev);
            gchar * devClass = gst_device_get_device_class(dev);

//            GstStructure * s = gst_device_get_properties(dev);
//            gchar * devDescr = nullptr;
//            gst_structure_get(s, "device.adapter.description", G_TYPE_STRING, &devDescr, nullptr);

            DeviceDescr descr;
            descr.deviceName  = devName;
            descr.deviceClass = devClass;
//            if (devDescr)
//            {
//                descr.deviceDescr = devDescr;
//            }

            m_p->getDeviceParams(dev, descr.internalName, descr.internalParams);

//            std::cout << "found " << devClass << " device " << devName << std::endl;
//            std::cout << "    " << descr.internalName;
            if (descr.internalParams.empty())
            {
                std::cout << std::endl;
            }
            else
            {
//                std::cout << " params" << std::endl;
//                for (uint32_t i = 0; i < descr.internalParams.size(); ++i)
//                {
//                    std::cout << "        "
//                              << descr.internalParams[i].first
//                              << ":"
//                              << descr.internalParams[i].second
//                              << std::endl;
//                }
            }

            m_p->m_devicesDirect[descr.deviceClass + descr.deviceName] = descr;

//            g_free(devDescr);
            g_free(devClass);
            g_free(devName);
        }

        g_list_free_full(devices, gst_object_unref);

        m_p->m_lastDevicesRequested = std::chrono::system_clock::now();

        break;
    }

    std::vector<DeviceDescr> result;
    for (const std::pair<std::string, DeviceDescr> & p : m_p->m_devicesDirect)
    {
        if (deviceClass.empty() || p.second.deviceClass == deviceClass)
        {
            result.emplace_back(p.second);
        }
    }
    return result;
}

} // namespace media
} // namespace dc
