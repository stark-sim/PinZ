#pragma once

#include <QUrl>

class QByteArray;

QUrl tmpFilesLandingUrl(const QByteArray &response);
QUrl tmpFilesDirectUrl(const QByteArray &html);
