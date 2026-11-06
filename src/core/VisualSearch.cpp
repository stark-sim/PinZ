#include "VisualSearch.h"

#include <QUrlQuery>
#include <iterator>

bool VisualSearchUploadFallbackState::takeNext(VisualSearchUploadProvider *provider)
{
    if (!provider)
        return false;
    static constexpr VisualSearchUploadProvider providers[] = {
        VisualSearchUploadProvider::Catbox,
        VisualSearchUploadProvider::Uguu,
        VisualSearchUploadProvider::TmpFiles,
    };
    if (m_nextProvider >= static_cast<int>(std::size(providers)))
        return false;
    *provider = providers[m_nextProvider++];
    return true;
}

void VisualSearchUploadFallbackState::recordFailure(const QString &providerName, const QString &reason)
{
    m_failures.append(QStringLiteral("%1: %2").arg(providerName, reason));
}

QString VisualSearchUploadFallbackState::failureSummary() const
{
    return m_failures.join(QLatin1Char('\n'));
}

VisualSearchProvider visualSearchProviderFromSettings(const QString &key)
{
    if (key == QStringLiteral("yandex"))
        return VisualSearchProvider::YandexImages;
    return VisualSearchProvider::GoogleLens;
}

QString visualSearchProviderKey(VisualSearchProvider provider)
{
    if (provider == VisualSearchProvider::YandexImages)
        return QStringLiteral("yandex");
    return QStringLiteral("google");
}

QString visualSearchDisplayName(VisualSearchProvider provider)
{
    if (provider == VisualSearchProvider::YandexImages)
        return QStringLiteral("Yandex Images");
    return QStringLiteral("Google Lens");
}

QString visualSearchTooltip(VisualSearchProvider provider)
{
    if (provider == VisualSearchProvider::YandexImages)
        return QStringLiteral("Search with Yandex Images");
    return QStringLiteral("Search with Google Lens");
}

QString visualSearchIconPath(VisualSearchProvider provider)
{
    if (provider == VisualSearchProvider::YandexImages)
        return QStringLiteral(":/icons/yandex_search.svg");
    return QStringLiteral(":/icons/search.svg");
}

QUrl visualSearchResultUrl(VisualSearchProvider provider, const QUrl &imageUrl)
{
    QUrl result(provider == VisualSearchProvider::YandexImages
                    ? QStringLiteral("https://yandex.com/images/search")
                    : QStringLiteral("https://lens.google.com/uploadbyurl"));
    QUrlQuery query;
    if (provider == VisualSearchProvider::YandexImages)
        query.addQueryItem(QStringLiteral("rpt"), QStringLiteral("imageview"));
    query.addQueryItem(QStringLiteral("url"), imageUrl.toString(QUrl::FullyEncoded));
    result.setQuery(query);
    return result;
}

QSize visualSearchUploadSize(const QSize &sourceSize, int maximumLongEdge)
{
    if (!sourceSize.isValid() || maximumLongEdge <= 0)
        return QSize();
    if (qMax(sourceSize.width(), sourceSize.height()) <= maximumLongEdge)
        return sourceSize;
    return sourceSize.scaled(maximumLongEdge, maximumLongEdge, Qt::KeepAspectRatio);
}
