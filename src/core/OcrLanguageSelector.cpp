#include "OcrLanguageSelector.h"

#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSet>

namespace {
QStringList normalizedLanguages(const QStringList &languages)
{
    QStringList result;
    for (QString language : languages) {
        language = language.trimmed().toLower();
        if (!language.isEmpty() && language != QStringLiteral("osd")
            && !result.contains(language)) {
            result.append(language);
        }
    }
    return result;
}

bool isLatinLanguage(const QString &language)
{
    static const QSet<QString> languages = {
        QStringLiteral("eng"), QStringLiteral("tur"), QStringLiteral("deu"),
        QStringLiteral("fra"), QStringLiteral("spa"), QStringLiteral("ita"),
        QStringLiteral("por"), QStringLiteral("pol"), QStringLiteral("nld")
    };
    return languages.contains(language);
}

void appendInstalled(QStringList &result, const QStringList &installed,
                     const QString &language)
{
    if (installed.contains(language) && !result.contains(language))
        result.append(language);
}
}

QString ocrScriptFromOsdOutput(const QString &output)
{
    static const QRegularExpression expression(
        QStringLiteral(R"(^\s*Script\s*:\s*([^\r\n]+?)\s*$)"),
        QRegularExpression::CaseInsensitiveOption | QRegularExpression::MultilineOption);
    const QRegularExpressionMatch match = expression.match(output);
    return match.hasMatch() ? match.captured(1).trimmed() : QString();
}

QStringList installedOcrLanguageCodes(const QString &tessdataDirectory)
{
    if (tessdataDirectory.trimmed().isEmpty())
        return {};

    QDir directory(tessdataDirectory);
    QStringList result;
    const QFileInfoList files = directory.entryInfoList(
        {QStringLiteral("*.traineddata")}, QDir::Files, QDir::Name);
    for (const QFileInfo &file : files) {
        const QString code = file.completeBaseName().toLower();
        if (code != QStringLiteral("osd") && code != QStringLiteral("equ"))
            result.append(code);
    }
    return result;
}

bool ocrScriptDetectionAvailable(const QString &tessdataDirectory)
{
    return !tessdataDirectory.trimmed().isEmpty()
        && QFileInfo::exists(QDir(tessdataDirectory).filePath(QStringLiteral("osd.traineddata")));
}

QString automaticOcrLanguageArgument(const QStringList &installedLanguages,
                                     const QString &detectedScript,
                                     const QString &preferredLanguage)
{
    const QStringList installed = normalizedLanguages(installedLanguages);
    if (installed.isEmpty())
        return QString();

    const QString preferred = preferredLanguage.trimmed().toLower();
    const QString script = detectedScript.trimmed().toLower();
    QStringList selected;

    if (script.contains(QStringLiteral("cyrillic"))) {
        if (preferred == QStringLiteral("rus") || preferred == QStringLiteral("ukr"))
            appendInstalled(selected, installed, preferred);
        appendInstalled(selected, installed, QStringLiteral("rus"));
        appendInstalled(selected, installed, QStringLiteral("ukr"));
    } else if (script.contains(QStringLiteral("arabic"))) {
        appendInstalled(selected, installed, QStringLiteral("ara"));
    } else if (script.contains(QStringLiteral("japanese"))) {
        appendInstalled(selected, installed, QStringLiteral("jpn"));
    } else if (script.contains(QStringLiteral("hangul"))
               || script.contains(QStringLiteral("korean"))) {
        appendInstalled(selected, installed, QStringLiteral("kor"));
    } else if (script.contains(QStringLiteral("han"))) {
        if (preferred == QStringLiteral("chi_tra") || preferred == QStringLiteral("chi_sim"))
            appendInstalled(selected, installed, preferred);
        appendInstalled(selected, installed, QStringLiteral("chi_sim"));
        appendInstalled(selected, installed, QStringLiteral("chi_tra"));
    } else if (script.contains(QStringLiteral("latin"))) {
        if (isLatinLanguage(preferred))
            appendInstalled(selected, installed, preferred);
    }

    if (selected.isEmpty())
        appendInstalled(selected, installed, preferred);
    appendInstalled(selected, installed, QStringLiteral("eng"));

    // OSD reports only the dominant script. A page containing mostly English
    // plus a smaller Cyrillic or CJK section is commonly reported as Latin, so
    // keep installed non-Latin models in the automatic set for this fallback.
    // Latin models are intentionally not all loaded because they overlap
    // heavily and make recognition slower and less predictable.
    if (script.isEmpty() || script.contains(QStringLiteral("latin"))) {
        const QStringList secondaryScripts = {
            QStringLiteral("rus"), QStringLiteral("ukr"), QStringLiteral("ara"),
            QStringLiteral("jpn"), QStringLiteral("kor"),
            QStringLiteral("chi_sim"), QStringLiteral("chi_tra")
        };
        for (const QString &language : secondaryScripts)
            appendInstalled(selected, installed, language);
    }
    if (selected.isEmpty())
        selected.append(installed.first());

    return selected.join(QLatin1Char('+'));
}
