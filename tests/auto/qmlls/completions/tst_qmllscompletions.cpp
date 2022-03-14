/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include <QtJsonRpc/private/qjsonrpcprotocol_p.h>
#include <QtLanguageServer/private/qlanguageserverprotocol_p.h>
#include <QtQuickTestUtils/private/qmlutils_p.h>

#include <QtCore/qobject.h>
#include <QtCore/qprocess.h>
#include <QtCore/qlibraryinfo.h>
#include <QtCore/qstringlist.h>

#include <QtTest/qtest.h>

#include <iostream>
#include <variant>

QT_USE_NAMESPACE
using namespace Qt::StringLiterals;
using namespace QLspSpecification;

class tst_QmllsCompletions : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_QmllsCompletions();
private slots:
    void initTestCase() final;
    void completions_data();
    void completions();
    void cleanupTestCase();

private:
    QProcess m_server;
    QLanguageServerProtocol m_protocol;
    QString m_qmllsPath;
    QList<QByteArray> m_uriToClose;
};

tst_QmllsCompletions::tst_QmllsCompletions()
    : QQmlDataTest(QT_QMLTEST_DATADIR),
      m_protocol([this](const QByteArray &data) { m_server.write(data); })
{
    connect(&m_server, &QProcess::readyReadStandardOutput, this, [this]() {
        QByteArray data = m_server.readAllStandardOutput();
        m_protocol.receiveData(data);
    });

    connect(&m_server, &QProcess::readyReadStandardError, this,
            [this]() { qWarning() << "LSPerr" << m_server.readAllStandardError(); });

    m_qmllsPath = QLibraryInfo::path(QLibraryInfo::BinariesPath) + QLatin1String("/qmlls");
#ifdef Q_OS_WIN
    m_qmllsPath += QLatin1String(".exe");
#endif
    // allow overriding of the executable, to be able to use a qmlEcho script (as described in
    // qmllanguageservertool.cpp)
    m_qmllsPath = qEnvironmentVariable("QMLLS", m_qmllsPath);
    m_server.setProgram(m_qmllsPath);
    m_protocol.registerPublishDiagnosticsNotificationHandler([](const QByteArray &, auto) {
        // ignoring qmlint notifications
    });
}

void tst_QmllsCompletions::initTestCase()
{
    QQmlDataTest::initTestCase();
    if (!QFileInfo::exists(m_qmllsPath)) {
        QString message =
                QStringLiteral("qmlls executable not found (looked for %0)").arg(m_qmllsPath);
        QSKIP(qPrintable(message)); // until we add a feature for this we avoid failing here
    }
    m_server.start();
    InitializeParams clientInfo;
    clientInfo.rootUri = QUrl::fromLocalFile(dataDirectory() + "/default").toString().toUtf8();
    TextDocumentClientCapabilities tDoc;
    PublishDiagnosticsClientCapabilities pDiag;
    tDoc.publishDiagnostics = pDiag;
    pDiag.versionSupport = true;
    clientInfo.capabilities.textDocument = tDoc;
    bool didInit = false;
    m_protocol.requestInitialize(clientInfo, [this, &didInit](const InitializeResult &serverInfo) {
        Q_UNUSED(serverInfo);
        m_protocol.notifyInitialized(InitializedParams());
        didInit = true;
    });
    QTRY_COMPARE_WITH_TIMEOUT(didInit, true, 10000);
    QFile file(testFile("completions/Yyy.qml"));
    QVERIFY(file.open(QIODevice::ReadOnly));

    DidOpenTextDocumentParams oParams;
    TextDocumentItem textDocument;
    QByteArray uri = testFileUrl("completions/Yyy.qml").toString().toUtf8();
    textDocument.uri = uri;
    textDocument.text = file.readAll();
    oParams.textDocument = textDocument;
    m_protocol.notifyDidOpenTextDocument(oParams);
    m_uriToClose.append(uri);
}

void tst_QmllsCompletions::completions_data()
{
    QTest::addColumn<QByteArray>("uri");
    QTest::addColumn<int>("lineNr");
    QTest::addColumn<int>("character");
    QTest::addColumn<QStringList>("expected");
    QTest::addColumn<QStringList>("notExpected");

    QByteArray uri = testFileUrl("completions/Yyy.qml").toString().toUtf8();
    QTest::newRow("objEmptyLine") << uri << 7 << 0
                                  << QStringList({ u"Rectangle"_s, u"property"_s, u"width"_s,
                                                   u"function"_s })
                                  << QStringList({ u"QtQuick"_s, u"vector4d"_s });
    QTest::newRow("inBindingLabel")
            << uri << 3 << 7 << QStringList({ u"Rectangle"_s, u"property"_s, u"width"_s })
            << QStringList({ u"QtQuick"_s, u"vector4d"_s });
    QTest::newRow("fileStart") << uri << 0 << 0 << QStringList({ u"Rectangle"_s, u"import"_s })
                               << QStringList({ u"QtQuick"_s, u"vector4d"_s, u"width"_s });
    QTest::newRow("importImport") << uri << 0 << 3 << QStringList({ u"Rectangle"_s, u"import"_s })
                                  << QStringList({ u"QtQuick"_s, u"vector4d"_s, u"width"_s });
    QTest::newRow("importModuleStart")
            << uri << 0 << 7 << QStringList({ u"QtQuick"_s })
            << QStringList({ u"vector4d"_s, u"width"_s, u"Rectangle"_s, u"import"_s });
    QTest::newRow("importVersionStart")
            << uri << 0 << 15 << QStringList({ u"2"_s, u"as"_s })
            << QStringList({ u"Rectangle"_s, u"import"_s, u"vector4d"_s, u"width"_s });
    //    QTest::newRow("importVersionMinor") << uri << 0 << 17 << QStringList({u"15"_s})
    //                                  << QStringList({u"as"_s, u"Rectangle"_s, u"import"_s,
    //                                  u"vector4d"_s, u"width"_s});
    QTest::newRow("inScript") << uri << 5 << 14
                              << QStringList(
                                         { u"Rectangle"_s, u"vector4d"_s, u"lala()"_s, u"width"_s })
                              << QStringList({ u"import"_s });
}

void tst_QmllsCompletions::completions()
{
    QFETCH(QByteArray, uri);
    QFETCH(int, lineNr);
    QFETCH(int, character);
    QFETCH(QStringList, expected);
    QFETCH(QStringList, notExpected);

    CompletionParams cParams;
    cParams.position.line = lineNr;
    cParams.position.character = character;
    cParams.textDocument.uri = uri;
    std::shared_ptr<bool> didFinish = std::make_shared<bool>(false);
    auto clean = [didFinish]() { *didFinish = true; };

    m_protocol.requestCompletion(
            cParams,
            [clean, uri, expected, notExpected](auto res) {
                QSet<QString> labels;
                if (const QList<CompletionItem> *cItems =
                            std::get_if<QList<CompletionItem>>(&res)) {
                    for (const CompletionItem &c : *cItems)
                        labels << c.label;
                }
                for (const QString &exp : expected) {
                    QVERIFY2(labels.contains(exp),
                             u"no %1 in %2"_s
                                     .arg(exp,
                                          QStringList(labels.begin(), labels.end()).join(u", "_s))
                                     .toUtf8());
                }
                for (const QString &nexp : notExpected) {
                    QVERIFY2(!labels.contains(nexp),
                             u"found unexpected completion  %1"_s.arg(nexp).toUtf8());
                }
                clean();
            },
            [clean](const ResponseError &err) {
                ProtocolBase::defaultResponseErrorHandler(err);
                QVERIFY2(false, "error computing the completion");
                clean();
            });
    QTRY_VERIFY_WITH_TIMEOUT(*didFinish, 30000);
}

void tst_QmllsCompletions::cleanupTestCase()
{
    for (const QByteArray &uri : m_uriToClose) {
        DidCloseTextDocumentParams closeP;
        closeP.textDocument.uri = uri;
        m_protocol.notifyDidCloseTextDocument(closeP);
    }
    m_server.closeWriteChannel();
    QTRY_COMPARE(m_server.state(), QProcess::NotRunning);
    QCOMPARE(m_server.exitStatus(), QProcess::NormalExit);
}

QTEST_MAIN(tst_QmllsCompletions)

#include <tst_qmllscompletions.moc>
