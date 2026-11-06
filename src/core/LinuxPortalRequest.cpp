#include "LinuxPortalRequest.h"

namespace LinuxPortalRequest {

QString requestPath(const QString &baseService, const QString &handleToken)
{
    QString sender = baseService.trimmed();
    const QString token = handleToken.trimmed();
    if (sender.isEmpty() || token.isEmpty())
        return {};
    if (sender.startsWith(QLatin1Char(':')))
        sender.remove(0, 1);
    sender.replace(QLatin1Char('.'), QLatin1Char('_'));
    sender.replace(QLatin1Char('-'), QLatin1Char('_'));
    return QStringLiteral("/org/freedesktop/portal/desktop/request/%1/%2")
        .arg(sender, token);
}

}
