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
class Util
{
protected:
    Util();

public:
    static Util & instance();
    ~Util();

public:
    std::vector<std::string> getDeviceList();

private:
    class impl;
    std::shared_ptr<impl> m_p;
};

} // namespace media
} // namespace dc

#endif // UTIL_H
