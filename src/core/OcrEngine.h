#ifndef OCRENGINE_H
#define OCRENGINE_H

#include <QObject>
#include <QString>
#include <QPixmap>
#include <QSet>

class QProcess;

class OcrEngine : public QObject {
    Q_OBJECT

public:
    explicit OcrEngine(QObject *parent = nullptr);
    ~OcrEngine() override;

    void recognize(const QPixmap &pixmap, const QString &languageTag = "auto",
                   const QString &preferredLanguageTag = QString());

    static QString tesseractPath();
    static QString tessdataDir();
    static QString mapLanguageTag(const QString &bcp47);

signals:
    void textReady(const QString &text);
    void failed(const QString &reason);
    void languageResolved(const QString &languageArgument);

private:
    void startAutomaticRecognition(const QString &imagePath, const QString &tessdataDirectory,
                                   const QString &preferredLanguage);
    void startRecognitionProcess(const QString &imagePath, const QString &tessdataDirectory,
                                 const QString &languageArgument);
    void failAndRemoveImage(const QString &imagePath, const QString &reason);

    QProcess *m_proc = nullptr;
    QSet<QString> m_pendingFiles;
};

#endif
