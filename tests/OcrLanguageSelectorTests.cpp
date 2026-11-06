#include <QtTest>

#include <QFile>
#include <QTemporaryDir>

#include "core/OcrLanguageSelector.h"

class OcrLanguageSelectorTests : public QObject
{
    Q_OBJECT

private slots:
    void parsesScriptFromOsdOutput()
    {
        const QString output = QStringLiteral(
            "Page number: 0\nOrientation in degrees: 0\nScript: Cyrillic\nScript confidence: 8.42\n");
        QCOMPARE(ocrScriptFromOsdOutput(output), QStringLiteral("Cyrillic"));
    }

    void combinesCyrillicWithEnglishWhenBothAreInstalled()
    {
        QCOMPARE(automaticOcrLanguageArgument(
                     {QStringLiteral("eng"), QStringLiteral("tur"), QStringLiteral("rus")},
                     QStringLiteral("Cyrillic"), QStringLiteral("tur")),
                 QStringLiteral("rus+eng"));
    }

    void keepsInstalledSecondaryScriptsWhenLatinIsDominant()
    {
        QCOMPARE(automaticOcrLanguageArgument(
                     {QStringLiteral("eng"), QStringLiteral("tur"), QStringLiteral("deu"),
                      QStringLiteral("fra"), QStringLiteral("rus")},
                     QStringLiteral("Latin"), QStringLiteral("tur")),
                 QStringLiteral("tur+eng+rus"));
    }

    void neverSelectsAMissingDetectedLanguage()
    {
        QCOMPARE(automaticOcrLanguageArgument(
                     {QStringLiteral("eng"), QStringLiteral("tur")},
                     QStringLiteral("Cyrillic"), QStringLiteral("tur")),
                 QStringLiteral("tur+eng"));
    }

    void mapsEastAsianScriptsToInstalledLanguagePacks()
    {
        QCOMPARE(automaticOcrLanguageArgument(
                     {QStringLiteral("eng"), QStringLiteral("jpn")},
                     QStringLiteral("Japanese"), QStringLiteral("eng")),
                 QStringLiteral("jpn+eng"));
        QCOMPARE(automaticOcrLanguageArgument(
                     {QStringLiteral("eng"), QStringLiteral("kor")},
                     QStringLiteral("Hangul"), QStringLiteral("eng")),
                 QStringLiteral("kor+eng"));
        QCOMPARE(automaticOcrLanguageArgument(
                     {QStringLiteral("eng"), QStringLiteral("chi_sim")},
                     QStringLiteral("Han"), QStringLiteral("eng")),
                 QStringLiteral("chi_sim+eng"));
    }

    void listsOnlyInstalledRecognitionLanguages()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        for (const QString &name : {QStringLiteral("eng.traineddata"),
                                    QStringLiteral("rus.traineddata"),
                                    QStringLiteral("osd.traineddata"),
                                    QStringLiteral("readme.txt")}) {
            QFile file(dir.filePath(name));
            QVERIFY(file.open(QIODevice::WriteOnly));
        }

        QCOMPARE(installedOcrLanguageCodes(dir.path()),
                 QStringList({QStringLiteral("eng"), QStringLiteral("rus")}));
        QVERIFY(ocrScriptDetectionAvailable(dir.path()));
    }
};

QTEST_MAIN(OcrLanguageSelectorTests)
#include "OcrLanguageSelectorTests.moc"
