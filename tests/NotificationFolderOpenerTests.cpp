#include <QtTest>
#include <QTemporaryDir>

#include "core/NotificationFolderOpener.h"

class NotificationFolderOpenerTests : public QObject
{
    Q_OBJECT

private slots:
    void resolvesAFileToItsContainingDirectory()
    {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        QCOMPARE(notificationDirectoryForPath(directory.filePath(QStringLiteral("video.mp4"))),
                 directory.path());
    }

    void linuxUsesXdgOpenBeforeDesktopServices()
    {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        QStringList calls;
        const bool opened = openNotificationFolder(
            directory.filePath(QStringLiteral("video.mp4")), NotificationDesktop::Linux,
            [&calls](const QString &program, const QStringList &arguments) {
                calls << program << arguments;
                return true;
            },
            [&calls](const QUrl &) {
                calls << QStringLiteral("desktop-services");
                return true;
            });

        QVERIFY(opened);
        QCOMPARE(calls, QStringList({QStringLiteral("xdg-open"), directory.path()}));
    }

    void linuxFallsBackToDesktopServicesWhenXdgOpenCannotStart()
    {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        QStringList calls;
        const bool opened = openNotificationFolder(
            directory.filePath(QStringLiteral("video.mp4")), NotificationDesktop::Linux,
            [&calls](const QString &program, const QStringList &) {
                calls << program;
                return false;
            },
            [&calls](const QUrl &url) {
                calls << url.toLocalFile();
                return true;
            });

        QVERIFY(opened);
        QCOMPARE(calls, QStringList({QStringLiteral("xdg-open"), directory.path()}));
    }
};

QTEST_APPLESS_MAIN(NotificationFolderOpenerTests)
#include "NotificationFolderOpenerTests.moc"
