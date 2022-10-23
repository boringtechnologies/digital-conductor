//******************************************************************************
//******************************************************************************

#include "core/permissionmgr.hpp"

#import <AVFoundation/AVCaptureDevice.h>

#include <iostream>

//******************************************************************************
//******************************************************************************
namespace dc
{

class PermissionManager::impl
{
    friend class PermissionManager;

    bool isMediaTypeAuthorized(AVMediaType mediaType)
    {
        return [AVCaptureDevice authorizationStatusForMediaType:mediaType] == AVAuthorizationStatusAuthorized;
    }
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
    bool cameraGranted = m_p->isMediaTypeAuthorized(AVMediaTypeVideo);
    bool microGranted = m_p->isMediaTypeAuthorized(AVMediaTypeAudio);

    return cameraGranted && microGranted;
}

void PermissionManager::requestMissingPermissions()
{
    if (!m_p->isMediaTypeAuthorized(AVMediaTypeVideo))
    {
        [AVCaptureDevice requestAccessForMediaType:AVMediaTypeVideo
                                 completionHandler: ^(BOOL granted) {
            std::cout << "AVMediaTypeVideo permissions granted = " << granted << '\n';
        }];
    }

    if (!m_p->isMediaTypeAuthorized(AVMediaTypeAudio))
    {
        [AVCaptureDevice requestAccessForMediaType:AVMediaTypeAudio
                                 completionHandler: ^(BOOL granted) {
            std::cout << "AVMediaTypeAudio permissions granted = " << granted << '\n';
        }];
    }
}

} // namespace dc
