#include <QtTest/QTest>
#include <QUrlQuery>

#include "core/VisualSearch.h"

class VisualSearchTests : public QObject
{
    Q_OBJECT

private slots:
    void parsesProviderSettings();
    void roundTripsProviderKeys();
    void exposesProviderMetadata();
    void buildsEncodedResultUrls();
    void ignoresStaleOperationGenerations();
    void capsVisualSearchUploadDimensions();
    void retriesTemporaryUploadProvidersInOrder();
    void reportsEveryTemporaryUploadFailure();
};

void VisualSearchTests::parsesProviderSettings()
{
    QCOMPARE(visualSearchProviderFromSettings(QStringLiteral("google")),
             VisualSearchProvider::GoogleLens);
    QCOMPARE(visualSearchProviderFromSettings(QStringLiteral("yandex")),
             VisualSearchProvider::YandexImages);
    QCOMPARE(visualSearchProviderFromSettings(QStringLiteral("unknown")),
             VisualSearchProvider::GoogleLens);
    QCOMPARE(visualSearchProviderFromSettings(QString()),
             VisualSearchProvider::GoogleLens);
}

void VisualSearchTests::roundTripsProviderKeys()
{
    QCOMPARE(visualSearchProviderKey(VisualSearchProvider::GoogleLens),
             QStringLiteral("google"));
    QCOMPARE(visualSearchProviderKey(VisualSearchProvider::YandexImages),
             QStringLiteral("yandex"));
    QCOMPARE(visualSearchProviderFromSettings(
                 visualSearchProviderKey(VisualSearchProvider::GoogleLens)),
             VisualSearchProvider::GoogleLens);
    QCOMPARE(visualSearchProviderFromSettings(
                 visualSearchProviderKey(VisualSearchProvider::YandexImages)),
             VisualSearchProvider::YandexImages);
}

void VisualSearchTests::exposesProviderMetadata()
{
    QCOMPARE(visualSearchDisplayName(VisualSearchProvider::GoogleLens),
             QStringLiteral("Google Lens"));
    QCOMPARE(visualSearchTooltip(VisualSearchProvider::GoogleLens),
             QStringLiteral("Search with Google Lens"));
    QCOMPARE(visualSearchIconPath(VisualSearchProvider::GoogleLens),
             QStringLiteral(":/icons/search.svg"));

    QCOMPARE(visualSearchDisplayName(VisualSearchProvider::YandexImages),
             QStringLiteral("Yandex Images"));
    QCOMPARE(visualSearchTooltip(VisualSearchProvider::YandexImages),
             QStringLiteral("Search with Yandex Images"));
    QCOMPARE(visualSearchIconPath(VisualSearchProvider::YandexImages),
             QStringLiteral(":/icons/yandex_search.svg"));
}

void VisualSearchTests::buildsEncodedResultUrls()
{
    const QUrl image(QStringLiteral("https://images.example.test/a crop.png?x=1&y=two words"));

    const QUrl google = visualSearchResultUrl(VisualSearchProvider::GoogleLens, image);
    QCOMPARE(google.scheme(), QStringLiteral("https"));
    QCOMPARE(google.host(), QStringLiteral("lens.google.com"));
    QCOMPARE(google.path(), QStringLiteral("/uploadbyurl"));
    const QString encodedGoogle = google.toString(QUrl::FullyEncoded);
    QVERIFY(encodedGoogle.contains(
        QStringLiteral("url=https://images.example.test/a%20crop.png?x%3D1%26y%3Dtwo%20words")));

    const QUrl yandex = visualSearchResultUrl(VisualSearchProvider::YandexImages, image);
    QCOMPARE(yandex.scheme(), QStringLiteral("https"));
    QCOMPARE(yandex.host(), QStringLiteral("yandex.com"));
    QCOMPARE(yandex.path(), QStringLiteral("/images/search"));
    QCOMPARE(QUrlQuery(yandex).queryItemValue(QStringLiteral("rpt")), QStringLiteral("imageview"));
    QVERIFY(yandex.toString(QUrl::FullyEncoded).contains(
        QStringLiteral("url=https://images.example.test/a%20crop.png?x%3D1%26y%3Dtwo%20words")));
}

void VisualSearchTests::ignoresStaleOperationGenerations()
{
    VisualSearchOperationState state;
    const quint64 oldGeneration = state.begin();
    const quint64 currentGeneration = state.begin();
    QVERIFY(!state.isCurrent(oldGeneration));
    QVERIFY(state.isCurrent(currentGeneration));
}

void VisualSearchTests::capsVisualSearchUploadDimensions()
{
    QCOMPARE(visualSearchUploadSize(QSize(4096, 2048)), QSize(2048, 1024));
    QCOMPARE(visualSearchUploadSize(QSize(1200, 800)), QSize(1200, 800));
    QCOMPARE(visualSearchUploadSize(QSize()), QSize());
}

void VisualSearchTests::retriesTemporaryUploadProvidersInOrder()
{
    VisualSearchUploadFallbackState state;
    VisualSearchUploadProvider provider;

    QVERIFY(state.takeNext(&provider));
    QCOMPARE(provider, VisualSearchUploadProvider::Catbox);
    QVERIFY(state.takeNext(&provider));
    QCOMPARE(provider, VisualSearchUploadProvider::Uguu);
    QVERIFY(state.takeNext(&provider));
    QCOMPARE(provider, VisualSearchUploadProvider::TmpFiles);
    QVERIFY(!state.takeNext(&provider));
}

void VisualSearchTests::reportsEveryTemporaryUploadFailure()
{
    VisualSearchUploadFallbackState state;
    state.recordFailure(QStringLiteral("Catbox"), QStringLiteral("Host unreachable"));
    state.recordFailure(QStringLiteral("Uguu"), QStringLiteral("Timeout"));

    const QString summary = state.failureSummary();
    QVERIFY(summary.contains(QStringLiteral("Catbox: Host unreachable")));
    QVERIFY(summary.contains(QStringLiteral("Uguu: Timeout")));
}

QTEST_APPLESS_MAIN(VisualSearchTests)

#include "VisualSearchTests.moc"
