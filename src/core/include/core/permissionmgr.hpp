//******************************************************************************
//******************************************************************************

#ifndef PERMISSINGMGR_H
#define PERMISSINGMGR_H

#include <memory>

//******************************************************************************
//******************************************************************************
namespace dc
{

class PermissionManager;
typedef std::shared_ptr<PermissionManager> PermissionMgrPtr;

//******************************************************************************
//******************************************************************************
class PermissionManager final
{
  public:
    PermissionManager();
    ~PermissionManager();

    bool isAllPermissinsGranted();
    void requestMissingPermissions();

  private:
    class impl;
    std::shared_ptr<impl> m_p;
};

} // namespace dc

#endif // PERMISSINGMGR_H
