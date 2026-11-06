#include "NotificationFolderOpener.h"

#include <QDir>
#include <QFileInfo>

QString notificationDirectoryForPath(const QString &path)
{
    if (path.trimmed().isEmpty())
        return {};

    const QFileInfo info(path);
    const QString directory = info.isDir() ? info.absoluteFilePath() : info.absolutePath();
    return QDir(directory).absolutePath();
}

bool openNotificationFolder(const QString &path,
                            NotificationDesktop desktop,
                            const NotificationProcessLauncher &processLauncher,
                            const NotificationUrlLauncher &urlLauncher)
{
    const QString directory = notificationDirectoryForPath(path);
    if (directory.isEmpty() || !QDir(directory).exists())
        return false;

    if (desktop == NotificationDesktop::Linux) {
        if (processLauncher(QStringLiteral("xdg-open"), {directory}))
            return true;
        return urlLauncher(QUrl::fromLocalFile(directory));
    }

    const QUrl directoryUrl = QUrl::fromLocalFile(directory);
    if (urlLauncher(directoryUrl))
        return true;

    if (desktop == NotificationDesktop::Windows) {
        return processLauncher(QStringLiteral("explorer.exe"),
                               {QDir::toNativeSeparators(directory)});
    }
    return false;
}
