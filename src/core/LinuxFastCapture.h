#ifndef LINUXFASTCAPTURE_H
#define LINUXFASTCAPTURE_H

#include <QImage>
#include <QObject>

#include <memory>

struct FastCaptureFrame {
    QImage image;
    qint64 capturedAtMs = 0;
};

class QWidget;

class LinuxFastCapture : public QObject {
    Q_OBJECT

public:
    static LinuxFastCapture &instance();

    static QImage copyBgraFrame(const uchar *data, int width, int height, int bytesPerLine);
    static QImage shareBgraFrame(uchar *data, int width, int height, int bytesPerLine,
                                 QImageCleanupFunction cleanupFunction,
                                 void *cleanupInfo);
    static bool frameIsFresh(qint64 capturedAtMs, qint64 nowMs, qint64 maxAgeMs = 1500);
    static bool overlayTransitionInvalidatesFrame(bool wasVisible, bool visible);

    void setEnabled(bool enabled, QWidget *parent = nullptr);
    bool enabled() const;
    void prewarm(QWidget *parent = nullptr);
    FastCaptureFrame latestFrame() const;
    QImage diagnosticFrame() const;
    void setOverlayVisible(bool visible);
    void shutdown();
    bool isRunning() const;

private:
    explicit LinuxFastCapture(QObject *parent = nullptr);
    ~LinuxFastCapture() override;
    Q_DISABLE_COPY(LinuxFastCapture)

    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

#endif
