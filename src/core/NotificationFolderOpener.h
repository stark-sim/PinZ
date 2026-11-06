#ifndef NOTIFICATIONFOLDEROPENER_H
#define NOTIFICATIONFOLDEROPENER_H

#include <QUrl>
#include <QString>
#include <QStringList>

#include <functional>

enum class NotificationDesktop {
    Windows,
    Linux,
    Other
};

using NotificationProcessLauncher =
    std::function<bool(const QString &, const QStringList &)>;
using NotificationUrlLauncher = std::function<bool(const QUrl &)>;

QString notificationDirectoryForPath(const QString &path);

bool openNotificationFolder(const QString &path,
                            NotificationDesktop desktop,
                            const NotificationProcessLauncher &processLauncher,
                            const NotificationUrlLauncher &urlLauncher);

#endif
