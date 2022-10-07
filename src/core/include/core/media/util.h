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

    std::vector<DeviceDescr> getDeviceList(const std::string & deviceClass);

private:
    class impl;
    std::shared_ptr<impl> m_p;
};

} // namespace media
} // namespace dc

#endif // UTIL_H
