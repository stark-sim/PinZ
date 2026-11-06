#include "LinuxFastCapture.h"

#include "LinuxDesktopIntegration.h"
#include "LinuxPortalScreenCast.h"
#include "TranslationManager.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QGuiApplication>
#include <QMessageBox>
#include <QEventLoop>
#include <QMutex>
#include <QMutexLocker>
#include <QScreen>
#include <QSysInfo>
#include <QTimer>

#include <cstring>
#include <limits>
#include <utility>

#if defined(Q_OS_UNIX) && !defined(Q_OS_MACOS)
#include <gst/app/gstappsink.h>
#include <gst/gst.h>
#include <gst/video/video.h>
#endif

namespace {

struct SharedFrameState {
    QMutex mutex;
    FastCaptureFrame frame;
};

bool supportedCaptureEnvironment()
{
#if defined(Q_OS_UNIX) && !defined(Q_OS_MACOS)
    if (!LinuxPortalScreenCast::isWaylandSession()
        || LinuxDesktopIntegration::detect(qEnvironmentVariable("XDG_CURRENT_DESKTOP"),
                                           qEnvironmentVariable("XDG_SESSION_DESKTOP"))
            != LinuxDesktopEnvironment::Gnome) {
        return false;
    }
    const QList<QScreen *> screens = QGuiApplication::screens();
    return screens.size() == 1 && screens.constFirst();
#else
    return false;
#endif
}

#if defined(Q_OS_UNIX) && !defined(Q_OS_MACOS)

struct MappedBgraFrameContext {
    GstBuffer *buffer = nullptr;
    GstVideoFrame frame;
};

void unmapBgraFrame(void *opaque)
{
    const auto context = static_cast<MappedBgraFrameContext *>(opaque);
    if (!context)
        return;
    gst_video_frame_unmap(&context->frame);
    gst_buffer_unref(context->buffer);
    delete context;
}

GstFlowReturn onNewSample(GstAppSink *appSink, gpointer userData)
{
    const auto state = *static_cast<std::shared_ptr<SharedFrameState> *>(userData);
    const qint64 capturedAtMs = QDateTime::currentMSecsSinceEpoch();
    GstSample *sample = gst_app_sink_pull_sample(appSink);
    if (!sample)
        return GST_FLOW_OK;

    GstVideoInfo info;
    QImage image;
    int frameWidth = 0;
    int frameHeight = 0;
    int frameStride = 0;
    GstCaps *caps = gst_sample_get_caps(sample);
    GstBuffer *buffer = gst_sample_get_buffer(sample);
    if (caps && buffer && gst_video_info_from_caps(&info, caps)) {
        auto *mapped = new MappedBgraFrameContext;
        mapped->buffer = gst_buffer_ref(buffer);
        if (gst_video_frame_map(&mapped->frame, &info, buffer, GST_MAP_READ)) {
            frameWidth = GST_VIDEO_INFO_WIDTH(&info);
            frameHeight = GST_VIDEO_INFO_HEIGHT(&info);
            frameStride = GST_VIDEO_FRAME_PLANE_STRIDE(&mapped->frame, 0);
            image = LinuxFastCapture::shareBgraFrame(
                static_cast<uchar *>(GST_VIDEO_FRAME_PLANE_DATA(&mapped->frame, 0)),
                frameWidth, frameHeight, frameStride, unmapBgraFrame, mapped);
        } else {
            gst_buffer_unref(mapped->buffer);
            delete mapped;
        }
    }
    gst_sample_unref(sample);

    if (!image.isNull()) {
        QMutexLocker locker(&state->mutex);
        const bool firstFrame = state->frame.capturedAtMs == 0;
        state->frame = {std::move(image), capturedAtMs};
        if (firstFrame) {
            qInfo() << "[LinuxFastCapture] first BGRA frame"
                    << frameWidth << frameHeight << "stride" << frameStride;
        }
    }
    return GST_FLOW_OK;
}

void destroyFrameState(gpointer data, GClosure *)
{
    delete static_cast<std::shared_ptr<SharedFrameState> *>(data);
}

GstElement *createPipeline(const LinuxPortalScreenCast::Stream &stream,
                           const std::shared_ptr<SharedFrameState> &frameState,
                           GstElement **appSinkOut, gulong *sampleHandlerOut)
{
    GstElement *pipeline = gst_pipeline_new("eshot-fast-capture");
    GstElement *source = gst_element_factory_make("pipewiresrc", "fast-capture-source");
    GstElement *rate = gst_element_factory_make("videorate", "fast-capture-rate");
    GstElement *rateCaps = gst_element_factory_make("capsfilter", "fast-capture-rate-caps");
    GstElement *queue = gst_element_factory_make("queue", "fast-capture-queue");
    GstElement *convert = gst_element_factory_make("videoconvert", "fast-capture-convert");
    GstElement *appSink = gst_element_factory_make("appsink", "fast-capture-sink");
    GstElement *elements[] = {source, rate, rateCaps, queue, convert, appSink};
    if (!pipeline || !source || !rate || !rateCaps || !queue || !convert || !appSink) {
        for (GstElement *element : elements) {
            if (element)
                gst_object_unref(element);
        }
        if (pipeline)
            gst_object_unref(pipeline);
        qWarning() << "[LinuxFastCapture] required GStreamer elements are unavailable";
        return nullptr;
    }

    g_object_set(source, "fd", stream.remoteFd(), nullptr);
    GObjectClass *sourceClass = G_OBJECT_GET_CLASS(source);
    const bool hasTargetObject = g_object_class_find_property(sourceClass, "target-object");
    const bool hasPath = g_object_class_find_property(sourceClass, "path");
    if (stream.pipewireSerial > 0 && hasTargetObject) {
        const QByteArray target = QByteArray::number(stream.pipewireSerial);
        g_object_set(source, "target-object", target.constData(), nullptr);
    } else if (stream.nodeId > 0 && hasPath) {
        const QByteArray path = QByteArray::number(stream.nodeId);
        g_object_set(source, "path", path.constData(), nullptr);
    } else {
        gst_object_unref(pipeline);
        for (GstElement *element : elements)
            gst_object_unref(element);
        qWarning() << "[LinuxFastCapture] PipeWire stream has no usable source selector";
        return nullptr;
    }
    if (g_object_class_find_property(sourceClass, "keepalive-time"))
        g_object_set(source, "keepalive-time", 1000, nullptr);

    g_object_set(rate, "max-rate", 10, nullptr);
    GstCaps *tenFps = gst_caps_new_simple("video/x-raw", "framerate",
                                          GST_TYPE_FRACTION, 10, 1, nullptr);
    g_object_set(rateCaps, "caps", tenFps, nullptr);
    gst_caps_unref(tenFps);

    g_object_set(queue, "leaky", 2, "max-size-buffers", 2, nullptr);
    g_object_set(appSink, "max-buffers", 2, "drop", TRUE, "sync", FALSE, nullptr);
    gst_app_sink_set_emit_signals(GST_APP_SINK(appSink), TRUE);
    GstCaps *bgra = gst_caps_new_simple("video/x-raw", "format",
                                        G_TYPE_STRING, "BGRA", nullptr);
    gst_app_sink_set_caps(GST_APP_SINK(appSink), bgra);
    gst_caps_unref(bgra);

    gst_bin_add_many(GST_BIN(pipeline), source, rate, rateCaps, queue, convert, appSink, nullptr);
    if (!gst_element_link_many(source, rate, rateCaps, queue, convert, appSink, nullptr)) {
        gst_object_unref(pipeline);
        qWarning() << "[LinuxFastCapture] could not link the GStreamer pipeline";
        return nullptr;
    }

    const gulong sampleHandler = g_signal_connect_data(
        appSink, "new-sample", G_CALLBACK(onNewSample),
        new std::shared_ptr<SharedFrameState>(frameState), destroyFrameState,
        static_cast<GConnectFlags>(0));
    if (sampleHandler == 0) {
        gst_object_unref(pipeline);
        qWarning() << "[LinuxFastCapture] could not connect the appsink sample callback";
        return nullptr;
    }

    *appSinkOut = appSink;
    *sampleHandlerOut = sampleHandler;
    return pipeline;
}

#endif

} // namespace

struct LinuxFastCapture::Impl {
    explicit Impl(LinuxFastCapture *owner)
        : frames(std::make_shared<SharedFrameState>())
    {
#if defined(Q_OS_UNIX) && !defined(Q_OS_MACOS)
        busTimer = new QTimer(owner);
        busTimer->setInterval(50);
        QObject::connect(busTimer, &QTimer::timeout, owner, [this]() { pollBus(); });
#else
        Q_UNUSED(owner);
#endif
    }

    void clearFrame()
    {
        QMutexLocker locker(&frames->mutex);
        frames->frame = {};
    }

    void stop()
    {
        running = false;
#if defined(Q_OS_UNIX) && !defined(Q_OS_MACOS)
        busTimer->stop();
        if (sessionWatcher) {
            QObject::disconnect(sessionWatcher, nullptr, nullptr, nullptr);
            sessionWatcher->deleteLater();
            sessionWatcher = nullptr;
        }
        if (pipeline) {
            gst_element_set_state(pipeline, GST_STATE_NULL);
            if (appSink && sampleHandler)
                g_signal_handler_disconnect(appSink, sampleHandler);
            gst_object_unref(pipeline);
        }
        if (bus)
            gst_object_unref(bus);
        pipeline = nullptr;
        appSink = nullptr;
        bus = nullptr;
        sampleHandler = 0;
        remoteFd.reset();
        if (!sessionHandle.isEmpty()) {
            const QString handle = std::exchange(sessionHandle, QString());
            LinuxPortalScreenCast::closeSession(handle);
        }
#endif
        clearFrame();
    }

    void prewarm(LinuxFastCapture *owner, QWidget *parent)
    {
#if defined(Q_OS_UNIX) && !defined(Q_OS_MACOS)
        if (!enabled || running || overlayVisible || prewarmAttempted
            || !supportedCaptureEnvironment()) {
            return;
        }
        prewarmAttempted = true;
        if (!LinuxPortalScreenCast::isAvailable())
            return;

        GError *initError = nullptr;
        if (!gst_init_check(nullptr, nullptr, &initError)) {
            qWarning() << "[LinuxFastCapture] GStreamer initialization failed:"
                       << (initError ? initError->message : "unknown error");
            if (initError)
                g_error_free(initError);
            return;
        }

        QScreen *screen = QGuiApplication::screens().constFirst();
        const QString persistenceId = QStringLiteral("fast-capture/") + screen->name();
        QMessageBox *consentAnchor = nullptr;
        if (!parent || !parent->isVisible()) {
            consentAnchor = new QMessageBox(
                QMessageBox::Information, TranslationManager::tr("fastCapture"),
                TranslationManager::tr("fastCapturePermission"), QMessageBox::NoButton);
            consentAnchor->setWindowFlags(Qt::Window | Qt::WindowTitleHint
                                          | Qt::CustomizeWindowHint | Qt::WindowStaysOnTopHint);
            consentAnchor->show();
            consentAnchor->raise();
            consentAnchor->activateWindow();
            QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
            parent = consentAnchor;
        }
        LinuxPortalScreenCast::Stream stream = LinuxPortalScreenCast::selectStream(
            parent, 120000, persistenceId, false);
        delete consentAnchor;
        consentAnchor = nullptr;
        if (!stream.isValid() || stream.remoteFd() < 0 || !enabled || overlayVisible
            || !supportedCaptureEnvironment()) {
            LinuxPortalScreenCast::closeSession(stream.sessionHandle);
            return;
        }

        qInfo() << "[LinuxFastCapture] authorized stream node=" << stream.nodeId
                << "serial=" << stream.pipewireSerial << "position=" << stream.position
                << "size=" << stream.size;
        GstElement *newPipeline = createPipeline(stream, frames, &appSink, &sampleHandler);
        if (!newPipeline) {
            LinuxPortalScreenCast::closeSession(stream.sessionHandle);
            return;
        }
        pipeline = newPipeline;
        bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));
        remoteFd = stream.pipewireFd;
        sessionHandle = stream.sessionHandle;
        sessionWatcher = new LinuxPortalSessionWatcher(sessionHandle, owner);
        QObject::connect(sessionWatcher, &LinuxPortalSessionWatcher::closed,
                         owner, [this]() { stop(); });
        if (!bus || gst_element_set_state(pipeline, GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE) {
            qWarning() << "[LinuxFastCapture] could not start the GStreamer pipeline";
            stop();
            return;
        }
        running = true;
        qInfo() << "[LinuxFastCapture] persistent PipeWire pipeline is playing";
        busTimer->start();
#else
        Q_UNUSED(owner);
        Q_UNUSED(parent);
#endif
    }

    void pollBus()
    {
#if defined(Q_OS_UNIX) && !defined(Q_OS_MACOS)
        if (!running || !bus)
            return;
        while (GstMessage *message = gst_bus_pop_filtered(
                   bus, static_cast<GstMessageType>(GST_MESSAGE_ERROR | GST_MESSAGE_EOS))) {
            const GstMessageType type = GST_MESSAGE_TYPE(message);
            if (type == GST_MESSAGE_ERROR) {
                GError *error = nullptr;
                gchar *debug = nullptr;
                gst_message_parse_error(message, &error, &debug);
                qWarning() << "[LinuxFastCapture] pipeline error:"
                           << (error ? error->message : "unknown error")
                           << (debug ? debug : "");
                if (error)
                    g_error_free(error);
                g_free(debug);
            }
            gst_message_unref(message);
            stop();
            break;
        }
#endif
    }

    std::shared_ptr<SharedFrameState> frames;
    bool enabled = false;
    bool running = false;
    bool overlayVisible = false;
    bool prewarmAttempted = false;
#if defined(Q_OS_UNIX) && !defined(Q_OS_MACOS)
    QTimer *busTimer = nullptr;
    GstElement *pipeline = nullptr;
    GstElement *appSink = nullptr;
    GstBus *bus = nullptr;
    gulong sampleHandler = 0;
    QSharedPointer<int> remoteFd;
    QString sessionHandle;
    LinuxPortalSessionWatcher *sessionWatcher = nullptr;
#endif
};

LinuxFastCapture &LinuxFastCapture::instance()
{
    static LinuxFastCapture service;
    return service;
}

LinuxFastCapture::LinuxFastCapture(QObject *parent)
    : QObject(parent)
    , m_impl(std::make_unique<Impl>(this))
{
    if (QCoreApplication *app = QCoreApplication::instance()) {
        connect(app, &QCoreApplication::aboutToQuit, this, [this]() { shutdown(); });
    }
}

LinuxFastCapture::~LinuxFastCapture()
{
    m_impl->stop();
}

QImage LinuxFastCapture::copyBgraFrame(const uchar *data, int width, int height,
                                       int bytesPerLine)
{
    if (!data || width <= 0 || height <= 0 || QSysInfo::ByteOrder != QSysInfo::LittleEndian
        || width > std::numeric_limits<int>::max() / 4
        || bytesPerLine < width * 4) {
        return {};
    }

    // On little-endian desktops, ARGB32's native 0xAARRGGBB words are
    // exactly GStreamer's B,G,R,A byte order, so no channel swizzle is needed.
    QImage image(width, height, QImage::Format_ARGB32);
    if (image.isNull())
        return {};
    const size_t rowBytes = static_cast<size_t>(width) * 4;
    for (int y = 0; y < height; ++y) {
        const auto sourceOffset = static_cast<qint64>(y) * bytesPerLine;
        std::memcpy(image.scanLine(y), data + sourceOffset, rowBytes);
    }
    return image;
}

QImage LinuxFastCapture::shareBgraFrame(uchar *data, int width, int height,
                                        int bytesPerLine,
                                        QImageCleanupFunction cleanupFunction,
                                        void *cleanupInfo)
{
    if (!data || width <= 0 || height <= 0 || QSysInfo::ByteOrder != QSysInfo::LittleEndian
        || width > std::numeric_limits<int>::max() / 4
        || bytesPerLine < width * 4) {
        if (cleanupFunction)
            cleanupFunction(cleanupInfo);
        return {};
    }
    return QImage(data, width, height, bytesPerLine, QImage::Format_ARGB32,
                  cleanupFunction, cleanupInfo);
}

bool LinuxFastCapture::frameIsFresh(qint64 capturedAtMs, qint64 nowMs, qint64 maxAgeMs)
{
    return capturedAtMs > 0 && maxAgeMs >= 0 && nowMs >= capturedAtMs
        && nowMs - capturedAtMs <= maxAgeMs;
}

bool LinuxFastCapture::overlayTransitionInvalidatesFrame(bool wasVisible, bool visible)
{
    return wasVisible != visible;
}

void LinuxFastCapture::setEnabled(bool enabled, QWidget *parent)
{
    if (m_impl->enabled == enabled) {
        if (enabled)
            prewarm(parent);
        return;
    }
    m_impl->enabled = enabled;
    m_impl->prewarmAttempted = false;
    if (enabled)
        prewarm(parent);
    else
        shutdown();
}

bool LinuxFastCapture::enabled() const
{
    return m_impl->enabled;
}

void LinuxFastCapture::prewarm(QWidget *parent)
{
    m_impl->prewarm(this, parent);
}

FastCaptureFrame LinuxFastCapture::latestFrame() const
{
    if (!m_impl->enabled || !m_impl->running || m_impl->overlayVisible
        || !supportedCaptureEnvironment()) {
        if (m_impl->enabled) {
            qInfo() << "[LinuxFastCapture] service unavailable"
                    << "running=" << m_impl->running
                    << "overlayVisible=" << m_impl->overlayVisible
                    << "supported=" << supportedCaptureEnvironment();
        }
        return {};
    }
    QMutexLocker locker(&m_impl->frames->mutex);
    const FastCaptureFrame frame = m_impl->frames->frame;
    if (frame.image.isNull()
        || !frameIsFresh(frame.capturedAtMs, QDateTime::currentMSecsSinceEpoch())) {
        qInfo() << "[LinuxFastCapture] no fresh frame; using one-shot portal"
                << "imageNull=" << frame.image.isNull()
                << "capturedAtMs=" << frame.capturedAtMs
                << "nowMs=" << QDateTime::currentMSecsSinceEpoch();
        return {};
    }
    return frame;
}

QImage LinuxFastCapture::diagnosticFrame() const
{
    if (!m_impl->enabled || !m_impl->running)
        return {};
    QMutexLocker locker(&m_impl->frames->mutex);
    return m_impl->frames->frame.image;
}

void LinuxFastCapture::setOverlayVisible(bool visible)
{
    const bool invalidate = overlayTransitionInvalidatesFrame(m_impl->overlayVisible, visible);
    m_impl->overlayVisible = visible;
    if (invalidate)
        m_impl->clearFrame();
}

void LinuxFastCapture::shutdown()
{
    m_impl->stop();
}

bool LinuxFastCapture::isRunning() const
{
    return m_impl->running;
}
