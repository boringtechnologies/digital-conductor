//******************************************************************************
//******************************************************************************

#include "core/permissionmgr.hpp"

//******************************************************************************
//******************************************************************************
namespace dc
{

class PermissionManager::impl
{
    friend class PermissionManager;
};

//******************************************************************************
//******************************************************************************
PermissionManager::PermissionManager()
{
}

PermissionManager::~PermissionManager()
{
}

bool PermissionManager::isAllPermissinsGranted()
{
    return true;
}

void PermissionManager::requestMissingPermissions()
{
    // Nothing to do in default implementation
}

} // namespace dc