// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QCoreApplication>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QtGlobal>

// silence warnings
static QtMessageHandler mh = qInstallMessageHandler([](QtMsgType, const QMessageLogContext &,
                                                       const QString &) {});

extern "C" int LLVMFuzzerTestOneInput(const char *Data, size_t Size) {
    static int argc = 3;
    static char arg1[] = "fuzzer";
    static char arg2[] = "-platform";
    static char arg3[] = "minimal";
    static char *argv[] = {arg1, arg2, arg3, nullptr};
    static QCoreApplication qca(argc, argv);
    QQmlEngine qqe;
    QQmlComponent qqc(&qqe);
    static const QUrl u;
    qqc.setData(QByteArray::fromRawData(Data, Size), u);
    delete qqc.create();
    return 0;
}
