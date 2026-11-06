#include <QtTest/QTest>

#include "core/LinuxFastCapture.h"

#include <array>
#include <cstring>
#include <limits>

class LinuxFastCaptureTests : public QObject {
    Q_OBJECT

private slots:
    void copiesPaddedBgraRowsWithoutPadding();
    void sharesBgraFrameWithoutCopying();
    void rejectsInvalidSharedBgraBuffers();
    void rejectsInvalidBgraBuffers();
    void checksFreshnessAtTheAgeBoundary();
    void rejectsInvalidFreshnessTimestamps();
    void invalidatesFramesOnlyOnOverlayTransitions();
};

void LinuxFastCaptureTests::copiesPaddedBgraRowsWithoutPadding()
{
    constexpr int width = 2;
    constexpr int height = 2;
    constexpr int stride = 12;
    std::array<uchar, stride * height> source = {
        10, 20, 30, 40, 50, 60, 70, 80, 201, 202, 203, 204,
        90, 100, 110, 120, 130, 140, 150, 160, 211, 212, 213, 214
    };
    constexpr uchar expected[height][width * 4] = {
        {10, 20, 30, 40, 50, 60, 70, 80},
        {90, 100, 110, 120, 130, 140, 150, 160}
    };

    const QImage image = LinuxFastCapture::copyBgraFrame(
        source.data(), width, height, stride);
    QVERIFY(!image.isNull());
    QCOMPARE(image.size(), QSize(width, height));
    QCOMPARE(image.format(), QImage::Format_ARGB32);
    for (int y = 0; y < height; ++y) {
        QVERIFY(std::memcmp(image.constScanLine(y), expected[y], width * 4) == 0);
    }
    QCOMPARE(image.pixelColor(0, 0), QColor(30, 20, 10, 40));
    QCOMPARE(image.pixelColor(1, 0), QColor(70, 60, 50, 80));
    QCOMPARE(image.pixelColor(0, 1), QColor(110, 100, 90, 120));
    QCOMPARE(image.pixelColor(1, 1), QColor(150, 140, 130, 160));

    source[0] = 255;
    source[stride] = 255;
    QCOMPARE(image.pixelColor(0, 0), QColor(30, 20, 10, 40));
    QCOMPARE(image.pixelColor(0, 1), QColor(110, 100, 90, 120));
}

void LinuxFastCaptureTests::sharesBgraFrameWithoutCopying()
{
    struct CleanupState {
        int calls = 0;
    } cleanupState;
    auto cleanup = [](void *opaque) {
        ++static_cast<CleanupState *>(opaque)->calls;
    };
    std::array<uchar, 8> source = {10, 20, 30, 40, 50, 60, 70, 80};

    QImage image = LinuxFastCapture::shareBgraFrame(
        source.data(), 2, 1, 8, cleanup, &cleanupState);
    QVERIFY(!image.isNull());
    QCOMPARE(image.constBits(), source.data());
    QCOMPARE(image.pixelColor(0, 0), QColor(30, 20, 10, 40));

    QImage alias = image;
    QCOMPARE(alias.constBits(), source.data());
    image = {};
    QCOMPARE(cleanupState.calls, 0);
    QCOMPARE(alias.pixelColor(1, 0), QColor(70, 60, 50, 80));
    alias = {};
    QCOMPARE(cleanupState.calls, 1);
}

void LinuxFastCaptureTests::rejectsInvalidSharedBgraBuffers()
{
    int cleanupCalls = 0;
    auto cleanup = [](void *opaque) { ++*static_cast<int *>(opaque); };
    QVERIFY(LinuxFastCapture::shareBgraFrame(
        nullptr, 1, 1, 4, cleanup, &cleanupCalls).isNull());
    QCOMPARE(cleanupCalls, 1);

    std::array<uchar, 8> source{};
    QVERIFY(LinuxFastCapture::shareBgraFrame(
        source.data(), 2, 1, 7, cleanup, &cleanupCalls).isNull());
    QCOMPARE(cleanupCalls, 2);
}

void LinuxFastCaptureTests::rejectsInvalidBgraBuffers()
{
    std::array<uchar, 16> bytes{};
    QVERIFY(LinuxFastCapture::copyBgraFrame(nullptr, 1, 1, 4).isNull());
    QVERIFY(LinuxFastCapture::copyBgraFrame(bytes.data(), 0, 1, 4).isNull());
    QVERIFY(LinuxFastCapture::copyBgraFrame(bytes.data(), -1, 1, 4).isNull());
    QVERIFY(LinuxFastCapture::copyBgraFrame(bytes.data(), 1, 0, 4).isNull());
    QVERIFY(LinuxFastCapture::copyBgraFrame(bytes.data(), 1, -1, 4).isNull());
    QVERIFY(LinuxFastCapture::copyBgraFrame(bytes.data(), 2, 1, 7).isNull());
    QVERIFY(LinuxFastCapture::copyBgraFrame(bytes.data(), 1, 1, -4).isNull());
    QVERIFY(LinuxFastCapture::copyBgraFrame(
        bytes.data(), std::numeric_limits<int>::max(), 1,
        std::numeric_limits<int>::max()).isNull());
}

void LinuxFastCaptureTests::checksFreshnessAtTheAgeBoundary()
{
    constexpr qint64 now = 100000;
    QVERIFY(LinuxFastCapture::frameIsFresh(now, now));
    QVERIFY(LinuxFastCapture::frameIsFresh(now - 1500, now));
    QVERIFY(!LinuxFastCapture::frameIsFresh(now - 1501, now));
    QVERIFY(LinuxFastCapture::frameIsFresh(now - 20, now, 20));
    QVERIFY(!LinuxFastCapture::frameIsFresh(now - 21, now, 20));
    QVERIFY(LinuxFastCapture::frameIsFresh(now, now, 0));
    QVERIFY(!LinuxFastCapture::frameIsFresh(now - 1, now, 0));
}

void LinuxFastCaptureTests::rejectsInvalidFreshnessTimestamps()
{
    QVERIFY(!LinuxFastCapture::frameIsFresh(0, 1000));
    QVERIFY(!LinuxFastCapture::frameIsFresh(-1, 1000));
    QVERIFY(!LinuxFastCapture::frameIsFresh(1001, 1000));
    QVERIFY(!LinuxFastCapture::frameIsFresh(1000, -1));
    QVERIFY(!LinuxFastCapture::frameIsFresh(1000, 1000, -1));
}

void LinuxFastCaptureTests::invalidatesFramesOnlyOnOverlayTransitions()
{
    // Initial capture setup releases buffers while no overlay is visible;
    // this must not discard the prewarmed frame it is about to consume.
    QVERIFY(!LinuxFastCapture::overlayTransitionInvalidatesFrame(false, false));
    QVERIFY(LinuxFastCapture::overlayTransitionInvalidatesFrame(false, true));
    QVERIFY(LinuxFastCapture::overlayTransitionInvalidatesFrame(true, false));
    QVERIFY(!LinuxFastCapture::overlayTransitionInvalidatesFrame(true, true));
}

QTEST_APPLESS_MAIN(LinuxFastCaptureTests)

#include "LinuxFastCaptureTests.moc"
