//******************************************************************************
//******************************************************************************

#ifndef UTIL_H
#define UTIL_H

#include <string>
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
