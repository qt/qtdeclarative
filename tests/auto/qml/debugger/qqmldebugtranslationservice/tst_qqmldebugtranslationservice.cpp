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

#include <qqmldebugtranslationservice.h>

#include <private/qqmldebugconnector_p.h>
#include <private/qversionedpacket_p.h>
#include <private/qhooks_p.h>
#include <private/qqmldebugtranslationprotocol_p.h>
#include <private/qqmldebugconnection_p.h>

#include <QtQuick/qquickview.h>
#include <QtTest/qtest.h>

#include <algorithm>

using namespace QQmlDebugTranslation;

const char *QMLFILE = "test.qml";

static bool gotMessage = false;

void messageHandler(QtMsgType type, const QMessageLogContext &, const QString &msg)
{
    if (type == QtWarningMsg && msg == "disable WatchTextElides is not implemented") {
        gotMessage = true;
    }
}

class tst_QQmlDebugTranslationService : public QQmlDebugTest
{
    struct ResolvedHooks;
    Q_OBJECT
public:
    tst_QQmlDebugTranslationService()
    {
    }

private slots:
    void initTestCase() override
    {
        QQmlDebugTest::initTestCase();

        m_view.setSource((testFileUrl(QMLFILE)));
        QTRY_VERIFY2(m_view.status() == QQuickView::Ready, "Failed to load QML file");
        m_view.show();
        QVERIFY(QTest::qWaitForWindowActive(&m_view));
        initQtHooks();
        QVERIFY(hooks->qt_qmlDebugEnableService(qPrintable(QQmlDebugTranslationServiceImpl::s_key)));
    }

    void init()
    {
        hooks->qt_qmlDebugClearBuffer();
        QVERIFY(currentDebugServiceMessage().isEmpty());
    }

    void changeLanguage(const QString &language = QLocale::system().uiLanguages().first())
    {
        // only necessary for visual debugging
        // QTest::qWait(500);

        QVersionedPacket<QQmlDebugConnector> packet;
        sendMessageToService(createChangeLanguageRequest(packet, dataDirectoryUrl(), language));

        // check without any eventloop cycle it should be still empty
        QCOMPARE(currentDebugServiceMessage(), QByteArray());

        QVersionedPacket<QQmlDebugConnector> expectedReply;
        expectedReply << Reply::LanguageChanged;
        QCOMPARE(currentReply().at(0), expectedReply.data());
        // clear buffer explicit, because it is used in other test methods as helper method aswell
        hooks->qt_qmlDebugClearBuffer();
        QVERIFY(currentDebugServiceMessage().isEmpty());
    }

    void streamTranslationIssues()
    {
        TranslationIssue issue;
        issue.type = TranslationIssue::Type::Missing;
        CodeMarker codeMarker;
        codeMarker.url = "url";
        codeMarker.line = 8;
        codeMarker.column = 9;
        issue.codeMarker = codeMarker;
        issue.language = "language";
        QVersionedPacket<QQmlDebugConnector> writePacket;
        writePacket << Reply::MissingTranslations
                    << QVector<TranslationIssue>{issue};
        Reply replyType;
        QVector<TranslationIssue> replyTranslationIssues;
        QVersionedPacket<QQmlDebugConnector> readPacket(writePacket.data());
        readPacket >> replyType;

        readPacket >> replyTranslationIssues;

        QCOMPARE(replyTranslationIssues.at(0), issue);
    }

    void missingAllTranslations()
    {
        changeLanguage("ru");

        QVersionedPacket<QQmlDebugConnector> packet;
        sendMessageToService(createMissingTranslationsRequest(packet));
        const QList<QByteArray> replyMessages = currentReply();

        TranslationIssue firstMissingTranslationLine;
        firstMissingTranslationLine.type = TranslationIssue::Type::Missing;
        CodeMarker codeMarker;
        codeMarker.url = testFileUrl(QMLFILE);
        codeMarker.line = 41;
        codeMarker.column = 19;
        firstMissingTranslationLine.codeMarker = codeMarker;
        firstMissingTranslationLine.language = "ru ru-RU ru-Cyrl-RU";
        TranslationIssue secondMissingTranslationLine;
        secondMissingTranslationLine.type = TranslationIssue::Type::Missing;
        codeMarker.url = testFileUrl(QMLFILE);
        codeMarker.line = 46;
        codeMarker.column = 19;
        secondMissingTranslationLine.codeMarker = codeMarker;
        secondMissingTranslationLine.language = "ru ru-RU ru-Cyrl-RU";
        QVersionedPacket<QQmlDebugConnector> expectedReply;
        expectedReply << Reply::MissingTranslations
            << QVector<TranslationIssue>{firstMissingTranslationLine, secondMissingTranslationLine};

        QVERIFY2(replyMessages.at(0) == expectedReply.data(), qPrintable(debugReply(replyMessages)));
    }

    void missingOneTranslation()
    {
        changeLanguage("fr");

        QVersionedPacket<QQmlDebugConnector> packet;
        sendMessageToService(createMissingTranslationsRequest(packet));
        const QList<QByteArray> replyMessages = currentReply();


        TranslationIssue missingTranslationLine;
        missingTranslationLine.type = TranslationIssue::Type::Missing;
        CodeMarker codeMarker;
        codeMarker.url = testFileUrl(QMLFILE);
        codeMarker.line = 46;
        codeMarker.column = 19;
        missingTranslationLine.codeMarker = codeMarker;
        missingTranslationLine.language = "fr fr-FR fr-Latn-FR";
        QVersionedPacket<QQmlDebugConnector> expectedReply;
        expectedReply << Reply::MissingTranslations
            << QVector<TranslationIssue>{missingTranslationLine};

        QVERIFY2(replyMessages.at(0) == expectedReply.data(), qPrintable(debugReply(replyMessages)));
    }

    void getTranslatableTextOccurrences()
    {

        QVersionedPacket<QQmlDebugConnector> packet;
        sendMessageToService(createTranslatableTextOccurrencesRequest(packet));
        QVersionedPacket<QQmlDebugConnector> readPacket(currentReply().at(0));

        Reply replyType;
        QVector<QmlElement> replyQmlElementList;
        readPacket >> replyType;
        readPacket >> replyQmlElementList;
        QCOMPARE(replyQmlElementList.count(), 2);
    }

    void getStates()
    {
        QSKIP("Skip the test for now");

        QVersionedPacket<QQmlDebugConnector> packet;
        sendMessageToService(createStateListRequest(packet));
        QVersionedPacket<QQmlDebugConnector> readPacket(currentReply().at(0));

        Reply replyType;
        QVector<QmlState> replyStateList;
        readPacket >> replyType;
        readPacket >> replyStateList;
        QCOMPARE(replyStateList.count(), 2);
    }

    void loopThroughAllStates()
    {
        QSKIP("Skip the test for now");

        QVersionedPacket<QQmlDebugConnector> packet;
        sendMessageToService(createStateListRequest(packet));
        QVersionedPacket<QQmlDebugConnector> readPacket(currentReply().at(0));

        Reply replyType;
        QVector<QmlState> replyStateList;
        readPacket >> replyType;
        readPacket >> replyStateList;
        QCOMPARE(replyStateList.count(), 2);

        for (int i = 0; i < replyStateList.count(); i++) {
            auto stateName = replyStateList.at(i).name;
            hooks->qt_qmlDebugClearBuffer();
            QVersionedPacket<QQmlDebugConnector> packet;
            sendMessageToService(createChangeStateRequest(packet, stateName));

            QVersionedPacket<QQmlDebugConnector> readPacket(currentReply().at(0));

            Reply replyType;
            QVector<QmlState> replyStateList;

            QString changedStateName;
            readPacket >> replyType >> changedStateName;
            QCOMPARE(replyType, Reply::StateChanged);
            QCOMPARE(stateName, changedStateName);
        }
    }

    void getElideWarning()
    {

        changeLanguage("fr");

        QVersionedPacket<QQmlDebugConnector> packet;
        sendMessageToService(createWatchTextElidesRequest(packet));
        const QList<QByteArray> replyMessages = currentReply();


        const QByteArray reply1 = replyMessages.at(0);

        TranslationIssue elidedeTextLine;
        elidedeTextLine.type = TranslationIssue::Type::Elided;
        CodeMarker codeMarker;
        codeMarker.url = testFileUrl(QMLFILE);
        codeMarker.line = 41;
        codeMarker.column = 19;
        elidedeTextLine.codeMarker = codeMarker;
        elidedeTextLine.language = "fr fr-FR fr-Latn-FR";
        QVersionedPacket<QQmlDebugConnector> expectedTextElidedReply;
        expectedTextElidedReply << Reply::TextElided << elidedeTextLine;

        Reply replyType1;
        TranslationIssue replyTranslationIssue;
        QVersionedPacket<QQmlDebugConnector> readPacket1(reply1);
        readPacket1 >> replyType1;
        readPacket1 >> replyTranslationIssue;
        QVERIFY2(reply1 == expectedTextElidedReply.data(), qPrintable(debugReply(replyMessages)));

        {
            gotMessage = false;
            auto handler = qInstallMessageHandler(messageHandler);
            auto guard = qScopeGuard([&]() { qInstallMessageHandler(handler); });

            packet.clear();
            sendMessageToService(createDisableWatchTextElidesRequest(packet));
            QTRY_VERIFY(gotMessage);
        }
    }

    void getElideWarningsWhenStateChanged()
    {
        QSKIP("Skip the test for now due to forever-loop. To be fixed in final 6.2");

        // it is only eliding in fr
        changeLanguage("fr");
        QVersionedPacket<QQmlDebugConnector> packet;
        sendMessageToService(createWatchTextElidesRequest(packet));
        const QString stateName("BiggerFontState");

        packet.clear();

        sendMessageToService(createChangeStateRequest(packet, stateName));

        QList<QByteArray> replyMessagesWithElideWarnings;

        QByteArray stateChangedReply;
        while (stateChangedReply.isEmpty()) {
            for (const auto &reply : currentReply()) {
                QVersionedPacket<QQmlDebugConnector> packet(reply);
                Reply replyType;
                packet >> replyType;

                if (replyType == Reply::StateChanged)
                    stateChangedReply = reply;
                else
                    replyMessagesWithElideWarnings.append(replyMessagesWithElideWarnings);
            }
            hooks->qt_qmlDebugClearBuffer();
        }

        TranslationIssue expectedTranslationIssue;
        expectedTranslationIssue.type = TranslationIssue::Type::Elided;
        CodeMarker codeMarker;
        codeMarker.url = testFileUrl(QMLFILE);
        codeMarker.line = 41;
        codeMarker.column = 19;
        expectedTranslationIssue.codeMarker = codeMarker;
        expectedTranslationIssue.language = "fr fr-FR fr-Latn-FR";

        // text size is calculated many times, so it could be that we get
        // the same warning many times
        for (const QByteArray &message : replyMessagesWithElideWarnings) {
            Reply replyType;
            TranslationIssue translationIssue;
            QVersionedPacket<QQmlDebugConnector> readPacket1(message);
            readPacket1 >> replyType;
            readPacket1 >> translationIssue;
            QCOMPARE(replyType, Reply::TextElided);
            QVERIFY2(expectedTranslationIssue == translationIssue,
                     qPrintable(debugReply(replyMessagesWithElideWarnings)));

        }

        QVersionedPacket<QQmlDebugConnector> expectedStateChangedReply;
        expectedStateChangedReply << Reply::StateChanged << stateName;

        Reply replyType2;
        QString stateNameReply;
        QVersionedPacket<QQmlDebugConnector> readPacket2(stateChangedReply);
        readPacket2 >> replyType2 >> stateNameReply;
        QCOMPARE(stateChangedReply, expectedStateChangedReply.data());

        {
            gotMessage = false;
            auto handler = qInstallMessageHandler(messageHandler);
            auto guard = qScopeGuard([&]() { qInstallMessageHandler(handler); });

            packet.clear();
            sendMessageToService(createDisableWatchTextElidesRequest(packet));

            QTRY_VERIFY(gotMessage);
        }
    }

private:
    QByteArray debugServiceMessage(const QByteArray &data)
    {
        QByteArray message;
        message.append(QQmlDebugTranslationServiceImpl::s_key.toUtf8());
        message.append(' ');
        message.append(QByteArray::number(data.size()));
        message.append(' ');
        message.append(data);
        return message;
    }

    void initQtHooks()
    {
        hooks = (ResolvedHooks *)qtHookData[QHooks::Startup];
        QCOMPARE(bool(hooks), true); // Available after connector start only.
        QCOMPARE(hooks->version, quintptr(1));
        QCOMPARE(hooks->numEntries, quintptr(6));
        QVERIFY(bool(hooks->qt_qmlDebugSendDataToService));
        QVERIFY(bool(hooks->qt_qmlDebugMessageBuffer));
        QVERIFY(bool(hooks->qt_qmlDebugMessageLength));
        QVERIFY(bool(hooks->qt_qmlDebugEnableService));
        QVERIFY(bool(hooks->qt_qmlDebugObjectAvailable));
        QVERIFY(bool(hooks->qt_qmlDebugClearBuffer));

    }
    void sendMessageToService(const QByteArray &message)
    {
        hooks->qt_qmlDebugSendDataToService(
            qPrintable(QQmlDebugTranslationServiceImpl::s_key), message.toHex());
    }

    QByteArray currentDebugServiceMessage()
    {
        return QByteArray::fromRawData(*hooks->qt_qmlDebugMessageBuffer, *hooks->qt_qmlDebugMessageLength);
    }


    QList<QByteArray> currentReply()
    {
        [this]() {
            QTRY_VERIFY(!currentDebugServiceMessage().isEmpty());
        }();

        QList<QByteArray> messages;
        int position = 0;
        QByteArray reply = currentDebugServiceMessage();

        while (position < reply.size()) {
            const QByteArray startString = qPrintable(QQmlDebugTranslationServiceImpl::s_key + " ");
            const int messageSizePosition = position + startString.count();
            const int sizeValueLength = reply.indexOf(' ', messageSizePosition) - messageSizePosition;

            int messageSize = reply.mid(messageSizePosition, sizeValueLength).toInt();
            // 1 == is the space after the size
            int messagePosition = messageSizePosition + sizeValueLength + 1;
            messages.append(reply.mid(messagePosition, messageSize));
            position = messagePosition + messageSize;
        }

        return messages;
    }

    QString debugReply(const QList<QByteArray> &replyMessages) const
    {
        QString debugString("Reply: ");
        auto replyTypeToString = [](Reply reply) -> QString {
            switch (reply) {
            case Reply::LanguageChanged:
                return "LanguageChanged";
            case Reply::StateChanged:
                return "StateChanged";
            case Reply::MissingTranslations:
                return "MissingTranslations";
            case Reply::TranslatableTextOccurrences:
                return "TranslatableTextOccurrences";
            case Reply::TextElided:
                return "TextElided";
            default:
                Q_ASSERT_X(false, "not implemented", "not implemented");
            }
            return QString();

        };

        for (const QByteArray &message : replyMessages) {
            Reply replyType;
            QVersionedPacket<QQmlDebugConnector> readPacket(message);
            readPacket >> replyType;
            debugString.append(replyTypeToString(replyType));
            if (replyType == Reply::MissingTranslations) {
                QVector<TranslationIssue> translationIssues;
                readPacket >> translationIssues;
                QStringList translationIssueStrings;
                std::transform(translationIssues.cbegin(),
                               translationIssues.cend(),
                               std::back_inserter(translationIssueStrings),
                               [] (const TranslationIssue &translationIssue) {
                                    return translationIssue.toDebugString();
                               }
                );
                QString translationIssuesString(" found %1 issues: %2");
                debugString.append(translationIssuesString.arg(QString::number(translationIssues.size()),
                                                               translationIssueStrings.join("; ")));
            }
            if (replyType == Reply::TextElided) {
                TranslationIssue translationIssue;
                readPacket >> translationIssue;
                debugString.append(QString(" %1 ").arg(translationIssue.toDebugString()));
            }
        }
        return debugString;
    }

    QString missingRussionTranslationWarningLine42() const
    {
        const QString fileUrl(testFileUrl(QMLFILE).toString());
        return QString("%1:42:19: QQmlDebugTranslationService: In locale ru ru-RU ru-Cyrl-RU translation is missing.").arg(fileUrl);
    }
    QString missingFranceTranslationWarningLine47() const
    {
        const QString fileUrl(testFileUrl(QMLFILE).toString());
        return QString("%1:47:19: QQmlDebugTranslationService: In locale fr fr-FR fr-Latn-FR translation is missing.").arg(fileUrl);
    }
    QString missingSpainTranslationWarningLine47() const
    {
        const QString fileUrl(testFileUrl(QMLFILE).toString());
        return QString("%1:47:19: QQmlDebugTranslationService: In locale es es-ES es-Latn-ES translation is missing.").arg(fileUrl);
    }
    QString missingRussionTranslationWarningLine47() const
    {
        const QString fileUrl(testFileUrl(QMLFILE).toString());
        return QString("%1:47:19: QQmlDebugTranslationService: In locale ru ru-RU ru-Cyrl-RU translation is missing.").arg(fileUrl);
    }

    QString franceElideWarningMessageLine42() const
    {
        return QString(
            "%1:42:19: QQmlDebugTranslationService: In locale fr fr-FR fr-Latn-FR the translated text is eliding."
        ).arg(testFileUrl(QMLFILE).toString());
    }

    QQuickView m_view;
    QmlState m_currentState;

    struct ResolvedHooks
    {
        quintptr version;
        quintptr numEntries;
        const char **qt_qmlDebugMessageBuffer;
        int *qt_qmlDebugMessageLength;
        bool (*qt_qmlDebugSendDataToService)(const char *serviceName, const char *hexData);
        bool (*qt_qmlDebugEnableService)(const char *data);
        bool (*qt_qmlDebugDisableService)(const char *data);
        void (*qt_qmlDebugObjectAvailable)();
        void (*qt_qmlDebugClearBuffer)();
    } *hooks = nullptr;
};

int main(int argc, char *argv[])
{
    char **argv2 = new char *[argc + 2];
    for (int i = 0; i < argc; ++i)
        argv2[i] = argv[i];
    argv2[argc] = qstrdup(qPrintable(QString("-qmljsdebugger=native,services:%1")
                                             .arg(QQmlDebugTranslationServiceImpl::s_key)));
    ++argc;
    argv2[argc] = nullptr;

    QGuiApplication app(argc, argv2);
    app.setAttribute(Qt::AA_Use96Dpi, true); \
    tst_QQmlDebugTranslationService tc; \
    QTEST_SET_MAIN_SOURCE_PATH \
    return QTest::qExec(&tc, argc, argv);
}

#include "tst_qqmldebugtranslationservice.moc"
