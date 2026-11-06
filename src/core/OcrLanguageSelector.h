#pragma once

#include <QString>
#include <QStringList>

QString ocrScriptFromOsdOutput(const QString &output);
QStringList installedOcrLanguageCodes(const QString &tessdataDirectory);
bool ocrScriptDetectionAvailable(const QString &tessdataDirectory);
QString automaticOcrLanguageArgument(const QStringList &installedLanguages,
                                     const QString &detectedScript,
                                     const QString &preferredLanguage);
