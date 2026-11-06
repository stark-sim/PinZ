#ifndef UGUUUPLOADER_H
#define UGUUUPLOADER_H

#include "ImageUploader.h"

class QNetworkReply;
class QHttpMultiPart;

class UguuUploader : public ImageUploader {
    Q_OBJECT

public:
    explicit UguuUploader(QObject *parent = nullptr);
    ~UguuUploader() override;

    Provider provider() const override { return Provider::Uguu; }
    QString providerDisplayName() const override;

    void upload() override;
    void cancel() override;

private:
    QNetworkReply *m_reply = nullptr;
    QHttpMultiPart *m_multipart = nullptr;
};

ImageUploader *createUguuUploader(QObject *parent = nullptr);

#endif
