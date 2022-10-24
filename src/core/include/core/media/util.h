//******************************************************************************
//******************************************************************************

#ifndef UTIL_H
#define UTIL_H

#include <algorithm>
#include <string>
#include <sstream>
#include <vector>
#include <memory>

//******************************************************************************
//******************************************************************************
namespace dc
{
namespace media
{

//******************************************************************************
//******************************************************************************
struct DeviceDescr
{
    std::string deviceName;
    std::string deviceClass;
    // std::string deviceDescr;

    std::string internalName;
    std::vector<std::pair<std::string, std::string> > internalParams;

    std::string toStdString() const
    {
        std::stringstream paramsStr;
        std::for_each(internalParams.begin(), internalParams.end(), [&](const auto &p) {
            paramsStr << p.first;
            paramsStr << '=';
            paramsStr << p.second;
            paramsStr << ';';
        });

        return "\"" + deviceName + "\" (" + deviceClass + ") device with params: " + paramsStr.str();
    }
};

//******************************************************************************
//******************************************************************************
class Util
{
protected:
    Util();

public:
    static Util & instance();
    ~Util();

public:
    // TODO use argc/argv if needed
    // bool initializeMediaModule(int argc, char *argv[]);
    // TODO plugins path?
    bool initializeMediaModule(const std::string & pluginsPath);

    void startWatchDevices();
    std::vector<DeviceDescr> getDeviceList(const std::string & deviceClass);
    std::vector<DeviceDescr> getDeviceListDirect(const std::string & deviceClass);

private:
    class impl;
    std::shared_ptr<impl> m_p;
};

} // namespace media
} // namespace dc

#endif // UTIL_H
