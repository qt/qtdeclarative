// Copyright (C) 2023 The Qt Company Ltd.

#include <QtTest>
#include <QtCore/qobject.h>
#include <QtCore/qdir.h>
#include <QtCore/qfile.h>
#include <QtCore/qstring.h>
#include <QtCore/qbytearray.h>

class tst_QmlCppCodegenVerify : public QObject
{
    Q_OBJECT
private slots:
    void verifyGeneratedSources_data();
    void verifyGeneratedSources();
};

void tst_QmlCppCodegenVerify::verifyGeneratedSources_data()
{
    QTest::addColumn<QString>("file");

    QDir a(":/a");
    const QStringList entries =  a.entryList(QDir::Files);
    for (const QString &entry : entries)
        QTest::addRow("%s", entry.toUtf8().constData()) << entry;
}

void tst_QmlCppCodegenVerify::verifyGeneratedSources()
{
    QFETCH(QString, file);
    QFile a(":/a/" + file);
    QFile b(":/b/" + file.replace("codegen_test_module", "codegen_test_module_verify"));

    QVERIFY(a.open(QIODevice::ReadOnly));
    QVERIFY(b.open(QIODevice::ReadOnly));

    const QByteArray aData = a.readAll();
    const QByteArray bData = b.readAll()
                                     .replace("verify/TestTypes", "TestTypes")
                                     .replace("verify_TestTypes", "TestTypes");

    QCOMPARE(aData, bData);
}

QTEST_MAIN(tst_QmlCppCodegenVerify)

#include "tst_qmlcppcodegen_verify.moc"
