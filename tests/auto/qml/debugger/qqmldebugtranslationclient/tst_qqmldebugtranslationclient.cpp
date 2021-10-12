/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

//QQmlDebugTest
#include <debugutil_p.h>
#include <qqmldebugprocess_p.h>

#include <qqmldebugtranslationservice.h>

#include <private/qqmldebugclient_p.h>
#include <private/qqmlpreviewclient_p.h>
#include <private/qqmldebugconnection_p.h>
#include <private/qqmldebugtranslationprotocol_p.h>
#include <private/qqmldebugconnector_p.h>
#include <private/qversionedpacket_p.h>
#include <private/qqmldebugtranslationclient_p.h>

#include <QtCore/qstring.h>
#include <QtCore/qlibraryinfo.h>
#include <QtCore/qtemporaryfile.h>
#include <QtCore/qelapsedtimer.h>
#include <QtTest/qtest.h>

#include <functional>

const char *QMLFILE = "typeTest.qml";

class tst_QQmlDebugTranslationClient : public QQmlDebugTest
{
    Q_OBJECT
public:
    tst_QQmlDebugTranslationClient()
        : QQmlDebugTest(QT_QMLTEST_DATADIR)
    {
    }

private slots:
    void init()
    {
        QQmlDebugTest::initTestCase();
        initDebugTranslationConnection();

        QVersionedPacket<QQmlDebugConnector> packet;
        m_debugTranslationClient->sendMessage(QQmlDebugTranslation::createChangeLanguageRequest(
                packet, dataDirectoryUrl(), "ru"));
        QTRY_VERIFY(m_debugTranslationClient->languageChanged);
    }

    void translationIssues()
    {
        QVersionedPacket<QQmlDebugConnector> packet;
        m_debugTranslationClient->sendMessage(
                QQmlDebugTranslation::createTranslationIssuesRequest(packet));

        QTRY_VERIFY(m_debugTranslationClient->translationIssues.size() > 0);
    }

    void translatableTextOccurrences()
    {
        QVersionedPacket<QQmlDebugConnector> packet;
        m_debugTranslationClient->sendMessage(
                QQmlDebugTranslation::createTranslatableTextOccurrencesRequest(packet));

        QTRY_COMPARE(m_debugTranslationClient->qmlElements.size(), 5);
    }

private:
    void initDebugTranslationConnection()
    {
        m_currentOutputLine = 0;

        auto executable = QLibraryInfo::path(QLibraryInfo::BinariesPath) + "/qml";
        auto services = QQmlDebugTranslationServiceImpl::s_key;
        auto extraArgs = testFile(QMLFILE);
        auto block = true;
        auto result = QQmlDebugTest::connectTo(executable, services, extraArgs, block);
        QCOMPARE(result, ConnectSuccess);
        QVERIFY2(verifyCurrentProcessOutputContains("QML Debugger: Waiting for connection"),
                 "QML Debugger connection not established.");
    }

    bool verifyCurrentProcessOutputContains(const QString &string, QString *output = nullptr)
    {
        bool contains = false;
        int verifiedCounter = 0;
        do {
            verifiedCounter++;
            auto currentProcessOutput = getNewProcessOutput();
            if (output)
                *output = currentProcessOutput;
            contains = currentProcessOutput.contains(string);

        } while (!contains && verifiedCounter < 100);
        return contains;
    }

    QString getNewProcessOutput(int updateTimeOut = 10)
    {
        int newCurrentOutputLine = 0;
        int counter = 0;
        do {
            counter++;
            newCurrentOutputLine = m_process->output().count();
            if (newCurrentOutputLine > m_currentOutputLine) {
                // lets wait a little bit more to not cut anything
                int triggeredCount = 0;
                int debugCounter = 0;
                do {
                    debugCounter++;
                    triggeredCount = m_process->output().count();
                    QTest::qWait(updateTimeOut);
                    newCurrentOutputLine = m_process->output().count();
                } while (triggeredCount != newCurrentOutputLine);
                QString currentOutputString = m_process->output().right(newCurrentOutputLine - m_currentOutputLine);
                if (m_enableClientOutput)
                    qDebug().noquote().nospace() << "Client: " << newCurrentOutputLine << "\n" << currentOutputString << "END - Client";
                m_currentOutputLine = newCurrentOutputLine;
                return currentOutputString;
            }
            QTest::qWait(updateTimeOut);
        } while (newCurrentOutputLine > m_currentOutputLine && counter < 100);
        return {};
    }

    QList<QQmlDebugClient *> createClients() override
    {
        m_previewClient = new QQmlPreviewClient(m_connection);
        // TODO create a simple client here instead of using the will be deleted one
        m_debugTranslationClient = new QQmlDebugTranslationClient(m_connection);
        return {m_debugTranslationClient};
        //return {m_previewClient};
        //return {m_previewClient, m_debugTranslationClient};
    }

    QPointer<QQmlDebugTranslationClient> m_debugTranslationClient;
    QPointer<QQmlPreviewClient> m_previewClient;
    int m_currentOutputLine = 0;
    bool m_enableClientOutput = true;
};

QTEST_MAIN(tst_QQmlDebugTranslationClient)

#include "tst_qqmldebugtranslationclient.moc"
