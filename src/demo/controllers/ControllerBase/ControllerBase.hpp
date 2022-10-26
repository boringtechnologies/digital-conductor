#pragma once

#include <QObject>

#include <type_traits>

template <class Controller> class ControllerBase
{
  public:
    static Controller &instance()
    {
        static Controller inst;
        return inst;
    }

    ControllerBase(const ControllerBase &)            = delete;
    ControllerBase &operator=(const ControllerBase &) = delete;

  protected:
    ControllerBase()
    {
        static_assert(std::is_base_of_v<ControllerBase<Controller>, Controller>);
        static_assert(std::is_base_of_v<QObject, Controller>);
    }

    virtual ~ControllerBase() = default;
};
