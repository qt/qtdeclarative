// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "tst_qmlls_progress_p.h"

#include <QtLanguageServer/private/qlanguageserverspectypes_p.h>
#include <QtQmlLS/private/qqmllanguageserver_p.h>
#include <QtTest/qsignalspy.h>
#include <QtTest/qtest.h>

using namespace QmlLsp;
using namespace QLspSpecification;

void tst_qmlls_progress::backgroundBuild_data()
{
    QTest::addColumn<std::function<void(QLanguageServerProtocol *, bool *)>>("registerCheck");

    QTest::addRow("createNotification") << std::function{ [](QLanguageServerProtocol *client,
                                                             bool *ok) {
        client->registerWorkDoneProgressCreateRequestHandler(
                [ok](const QByteArray &,
                     const Requests::WorkDoneProgressCreateParamsType &paramsToCheck,
                     LSPResponse<Responses::WorkDoneProgressCreateResultType> &&response) {
                    QCOMPARE(std::get<int>(paramsToCheck.token), 0);
                    *ok = true;
                    response.sendResponse();
                });
    } };

    QTest::addRow("startNotification") << std::function{ [](QLanguageServerProtocol *client,
                                                            bool *ok) {
        client->registerWorkDoneProgressCreateRequestHandler(
                [](const QByteArray &, const Requests::WorkDoneProgressCreateParamsType &,
                   LSPResponse<Responses::WorkDoneProgressCreateResultType> &&response) {
                    response.sendResponse();
                });
        client->registerProgressNotificationHandler([ok](const QByteArray &,
                                                         const ProgressParams &paramsToCheck) {
            QCOMPARE(std::get<int>(paramsToCheck.token), 0);
            if (std::holds_alternative<WorkDoneProgressBegin>(paramsToCheck.value)) {
                QVERIFY(!*ok);
                WorkDoneProgressBegin paramValueToCheck =
                        std::get<WorkDoneProgressBegin>(paramsToCheck.value);
                QCOMPARE(paramValueToCheck.message, "Building \"\"");
                QCOMPARE(paramValueToCheck.cancellable, true);
                *ok = true;
            }
        });
    } };

    QTest::addRow("endNotification") << std::function{ [](QLanguageServerProtocol *client,
                                                          bool *ok) {
        client->registerWorkDoneProgressCreateRequestHandler(
                [](const QByteArray &, const Requests::WorkDoneProgressCreateParamsType &,
                   LSPResponse<Responses::WorkDoneProgressCreateResultType> &&response) {
                    response.sendResponse();
                });
        client->registerProgressNotificationHandler(
                [ok](const QByteArray &, const ProgressParams &paramsToCheck) {
                    QCOMPARE(std::get<int>(paramsToCheck.token), 0);

                    if (std::holds_alternative<WorkDoneProgressEnd>(paramsToCheck.value)) {
                        QVERIFY(!*ok);
                        QCOMPARE(std::get<WorkDoneProgressEnd>(paramsToCheck.value).message,
                                 "Build terminated");
                        *ok = true;
                    }
                });
    } };
}

struct ClientAndServer
{
    std::unique_ptr<QLanguageServerProtocol> client;
    std::unique_ptr<QQmlLanguageServer> server;

    ClientAndServer()
    {
        client = std::make_unique<QLanguageServerProtocol>(
                [this](const QByteArray &data) { server->receiveData(data, true); });
        server = std::make_unique<QQmlLanguageServer>(
                [this](const QByteArray &data) { client->receiveData(data); });
    }

    static ClientAndServer createAndInitialize()
    {
        ClientAndServer result;

        bool initializedOk = false;
        InitializeParams initializeParams;
        initializeParams.capabilities.window.emplace().workDoneProgress = true;
        result.client->requestInitialize(
                initializeParams,
                [&initializedOk](const InitializeResult &) { initializedOk = true; });
        [&initializedOk] { QTRY_VERIFY_WITH_TIMEOUT(initializedOk, 3000); }();
        result.client->notifyInitialized({});

        return result;
    }
};

void tst_qmlls_progress::backgroundBuild()
{
    QFETCH(std::function<void(QLanguageServerProtocol *, bool *)>, registerCheck);

    auto [client, server] = ClientAndServer::createAndInitialize();

    bool ok = false;
    registerCheck(client.get(), &ok);

    // simulate build trigger:
    server->codeModelManager()->backgroundBuildStarted("");
    server->codeModelManager()->backgroundBuildFinished("");

    QTRY_VERIFY_WITH_TIMEOUT(ok, 3000);
}

void tst_qmlls_progress::cancelBackgroundBuild()
{
    auto [client, server] = ClientAndServer::createAndInitialize();

    client->registerWorkDoneProgressCreateRequestHandler(
            [](const QByteArray &, const Requests::WorkDoneProgressCreateParamsType &,
               LSPResponse<Responses::WorkDoneProgressCreateResultType> &&response) {
                response.sendResponse();
            });
    client->registerProgressNotificationHandler(
            [](const QByteArray &, const ProgressParams &paramsToCheck) {
                QCOMPARE(std::get<int>(paramsToCheck.token), 0);
            });

    QSignalSpy spy(server->codeModelManager(), &QQmlCodeModelManager::backgroundBuildCancelled);

    // simulate build trigger
    emit server->codeModelManager()->backgroundBuildStarted("");

    QCOMPARE(spy.count(), 0);

    WorkDoneProgressCancelParams p;
    p.token = 0;
    client->notifyWorkDoneProgressCancel(p);

    QTRY_COMPARE_WITH_TIMEOUT(spy.count(), 1, 3000);
}

void tst_qmlls_progress::cancelBackgroundBuildWithInvalidToken()
{
    auto [client, server] = ClientAndServer::createAndInitialize();

    client->registerWorkDoneProgressCreateRequestHandler(
            [](const QByteArray &, const Requests::WorkDoneProgressCreateParamsType &,
               LSPResponse<Responses::WorkDoneProgressCreateResultType> &&response) {
                response.sendResponse();
            });
    client->registerProgressNotificationHandler(
            [](const QByteArray &, const ProgressParams &paramsToCheck) {
                QCOMPARE(std::get<int>(paramsToCheck.token), 0);
            });

    QSignalSpy spy(server->codeModelManager(), &QQmlCodeModelManager::backgroundBuildCancelled);

    // simulate build trigger
    emit server->codeModelManager()->backgroundBuildStarted("");

    QCOMPARE(spy.count(), 0);

    QTest::ignoreMessage(QtWarningMsg, "Ignoring unknown token 42 in cancellation request.");
    QTest::ignoreMessage(QtWarningMsg, "Ignoring unknown token 0 in cancellation request.");

    WorkDoneProgressCancelParams invalid;
    invalid.token = 42;
    client->notifyWorkDoneProgressCancel(invalid);

    WorkDoneProgressCancelParams valid;
    valid.token = 0;
    client->notifyWorkDoneProgressCancel(valid);

    WorkDoneProgressCancelParams duplicate;
    duplicate.token = valid.token;
    client->notifyWorkDoneProgressCancel(duplicate);

    // only the valid id should trigger the backgroundBuildCancelled. The duplicate shouldn't make anything crash.
    QTRY_COMPARE_WITH_TIMEOUT(spy.count(), 1, 3000);
}

void tst_qmlls_progress::orderOfProgressNotifications()
{
    auto [client, server] = ClientAndServer::createAndInitialize();

    int step = 0;

    client->registerWorkDoneProgressCreateRequestHandler(
            [&step](const QByteArray &, const Requests::WorkDoneProgressCreateParamsType &,
                    LSPResponse<Responses::WorkDoneProgressCreateResultType> &&response) {
                QCOMPARE(step, 0);
                ++step;

                // the server shouldn't send the progress end notification while the WorkDoneProgressCreate request
                // didn't finish
                using namespace std::chrono_literals;
                QTest::qWait(500ms);
                response.sendResponse();
            });
    client->registerProgressNotificationHandler(
            [&step](const QByteArray &, const ProgressParams &paramsToCheck) {
                QCOMPARE(std::get<int>(paramsToCheck.token), 0);
                std::visit(qOverloadedVisitor{ [&step](const WorkDoneProgressBegin &) {
                                                  QCOMPARE(step, 1);
                                                  ++step;
                                              },
                                               [](const WorkDoneProgressReport &) {
                                                   QFAIL("No progress reports are supported yet.");
                                               },
                                               [&step](const WorkDoneProgressEnd &) {
                                                   QCOMPARE(step, 2);
                                                   ++step;
                                               } },
                           paramsToCheck.value);
            });

    // simulate build trigger
    emit server->codeModelManager()->backgroundBuildStarted("");
    emit server->codeModelManager()->backgroundBuildFinished("");

    QTRY_COMPARE_WITH_TIMEOUT(step, 3, 3000);
}

QTEST_MAIN(tst_qmlls_progress)
