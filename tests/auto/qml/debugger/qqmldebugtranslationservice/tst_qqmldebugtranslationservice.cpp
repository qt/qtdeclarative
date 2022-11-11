// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
        : QQmlDebugTest(QT_QMLTEST_DATADIR)
    {
    }

private slots:
    void initTestCase() override
    {
        QQmlDebugTest::initTestCase();

        m_view.setSource((testFileUrl(QMLFILE)));
        QTRY_VERIFY2(m_view.status() == QQuickView::Ready, "Failed to load QML file");
        initQtHooks();
        QVERIFY(hooks->qt_qmlDebugEnableService(qPrintable(QQmlDebugTranslationServiceImpl::s_key)));
    }

    void init() override
    {
        QQmlDebugTest::init();
        hooks->qt_qmlDebugClearBuffer();
        QVERIFY(currentDebugServiceMessage().isEmpty());
    }

    void verifyMissingAllTranslationsForMissingLanguage()
    {
        changeLanguage("ru");
        auto translationIssues = getTranslationIssues();

        QCOMPARE(translationIssues.size(), getTranslatableTextOccurrences().size());
        QCOMPARE(translationIssues.at(0).language, "ru-Cyrl-RU ru-RU ru");
    }

    void verifyCorrectNumberOfMissingTranslations()
    {
        changeLanguage("fr");

        auto translationIssues = getTranslationIssues();

        QCOMPARE(translationIssues.size(), 3);
        QCOMPARE(translationIssues.at(0).language, "fr-Latn-FR fr-FR fr");
    }

    void verifyCorrectNumberOfTranslatableTextOccurrences()
    {
        QCOMPARE(getTranslatableTextOccurrences().size(), 5);
    }

    void verifyCorrectNumberOfStates()
    {
        QCOMPARE(getStates().size(), 2);
    }

    void getElideWarnings()
    {
        QVersionedPacket<QQmlDebugConnector> packet;
        sendMessageToService(createWatchTextElidesRequest(packet));

        changeLanguage("es");
        auto translationIssues = getTranslationIssues();

        int elideWarningCount = 0;
        for (auto issue : translationIssues) {
            if (issue.type == TranslationIssue::Type::Elided) {
                elideWarningCount++;
            }
        }
        QCOMPARE(elideWarningCount, 1);
    }

    void getElideWarningsWhenStateChanged()
    {
        QVersionedPacket<QQmlDebugConnector> packet;
        sendMessageToService(createWatchTextElidesRequest(packet));

        changeLanguage("es");

        sendMessageToService(createChangeStateRequest(packet, "WayBiggerFontState"));

        auto translationIssues = getTranslationIssues();

        int elideWarningCount = 0;
        for (auto issue : translationIssues) {
            if (issue.type == TranslationIssue::Type::Elided) {
                elideWarningCount++;
            }
        }
        QCOMPARE(elideWarningCount, 1);
    }

    void loopThroughAllStates()
    {
        QVector<QmlState> stateList = getStates();

        QCOMPARE(stateList.size(), 2);

        for (int i = 0; i < stateList.size(); i++) {
            auto stateName = stateList.at(i).name;
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

private:

    QVector<QmlElement> getTranslatableTextOccurrences()
    {
        QVersionedPacket<QQmlDebugConnector> packet;
        sendMessageToService(createTranslatableTextOccurrencesRequest(packet));
        QVersionedPacket<QQmlDebugConnector> readPacket(currentReply().at(0));

        Reply replyType;
        QVector<QmlElement> qmlElementList;
        readPacket >> replyType;
        readPacket >> qmlElementList;

        return qmlElementList;
    }

    QVector<QmlState> getStates()
    {
        QVersionedPacket<QQmlDebugConnector> packet;
        sendMessageToService(createStateListRequest(packet));
        auto replies = currentReply();
        QVersionedPacket<QQmlDebugConnector> readPacket(replies.at(0));

        Reply replyType;
        QVector<QmlState> stateList;
        readPacket >> replyType;
        readPacket >> stateList;

        return stateList;
    }

    void changeLanguage(const QString &language = QLocale::system().uiLanguages().first())
    {
        QVersionedPacket<QQmlDebugConnector> packet;
        sendMessageToService(createChangeLanguageRequest(packet, dataDirectoryUrl(), language));
        QVersionedPacket<QQmlDebugConnector> readPacket(currentReply().at(0));

        // Use this for visual debugging
        // QTest::qWait(500);
    }

    QVector<TranslationIssue> getTranslationIssues()
    {
        QVersionedPacket<QQmlDebugConnector> packet;
        sendMessageToService(createTranslationIssuesRequest(packet));
        QVersionedPacket<QQmlDebugConnector> readPacket(currentReply().at(0));

        Reply replyType;
        QVector<TranslationIssue> translationIssues;
        readPacket >> replyType;
        readPacket >> translationIssues;

        return translationIssues;
    }

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
        clearBuffer();
        hooks->qt_qmlDebugSendDataToService(
            qPrintable(QQmlDebugTranslationServiceImpl::s_key), message.toHex());
    }

    void clearBuffer()
    {
        hooks->qt_qmlDebugClearBuffer();
        QCoreApplication::processEvents();
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
            const int messageSizePosition = position + startString.size();
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
            case Reply::TranslationIssues:
                return "TranslationIssues";
            case Reply::TranslatableTextOccurrences:
                return "TranslatableTextOccurrences";
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
            if (replyType == Reply::TranslationIssues) {
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
        }
        return debugString;
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
    QVector<char *> argv2(argc + 2);
    for (int i = 0; i < argc; ++i)
        argv2[i] = argv[i];

    QByteArray debuggerArg = QStringLiteral("-qmljsdebugger=native,services:%1")
                                  .arg(QQmlDebugTranslationServiceImpl::s_key).toUtf8();
    argv2[argc] = debuggerArg.data();
    argv2[++argc] = nullptr;

    QGuiApplication app(argc, argv2.data());
    app.setAttribute(Qt::AA_Use96Dpi, true); \
    tst_QQmlDebugTranslationService tc; \
    QTEST_SET_MAIN_SOURCE_PATH \
    return QTest::qExec(&tc, argc, argv);
}

#include "tst_qqmldebugtranslationservice.moc"
