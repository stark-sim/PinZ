#pragma once

#include <QString>
#include <QSize>
#include <QStringList>
#include <QUrl>

enum class VisualSearchProvider {
    GoogleLens,
    YandexImages
};

enum class VisualSearchUploadProvider {
    Catbox,
    Uguu,
    TmpFiles,
};

class VisualSearchUploadFallbackState
{
public:
    bool takeNext(VisualSearchUploadProvider *provider);
    void recordFailure(const QString &providerName, const QString &reason);
    QString failureSummary() const;

private:
    int m_nextProvider = 0;
    QStringList m_failures;
};

VisualSearchProvider visualSearchProviderFromSettings(const QString &key);
QString visualSearchProviderKey(VisualSearchProvider provider);
QString visualSearchDisplayName(VisualSearchProvider provider);
QString visualSearchTooltip(VisualSearchProvider provider);
QString visualSearchIconPath(VisualSearchProvider provider);
QUrl visualSearchResultUrl(VisualSearchProvider provider, const QUrl &imageUrl);
QSize visualSearchUploadSize(const QSize &sourceSize, int maximumLongEdge = 2048);

class VisualSearchOperationState {
public:
    quint64 begin() { return ++m_generation; }
    bool isCurrent(quint64 generation) const { return generation == m_generation; }

private:
    quint64 m_generation = 0;
};
