#ifndef LINUXPORTALHOSTREGISTRY_H
#define LINUXPORTALHOSTREGISTRY_H

#include <QString>

enum class LinuxPortalHostRegistrationState {
    NotAttempted,
    Registered,
    Unsupported,
    Failed
};

namespace LinuxPortalHostRegistry {

QString applicationId();
LinuxPortalHostRegistrationState registerApplication();
LinuxPortalHostRegistrationState state();
LinuxPortalHostRegistrationState classifyReply(bool success, const QString &errorName,
                                                const QString &errorMessage = QString());
bool portalMayIdentifyApp(LinuxPortalHostRegistrationState registrationState);

}

#endif
