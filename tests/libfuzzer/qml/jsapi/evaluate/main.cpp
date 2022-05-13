// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QCoreApplication>
#include <QJSEngine>

// libfuzzer test for QJSEngine::evaluate()

extern "C" int LLVMFuzzerTestOneInput(const char *Data, size_t Size) {
    const QByteArray ba = QByteArray::fromRawData(Data, Size);
    // avoid potential endless loops
    if (ba.contains("for") || ba.contains("while"))
        return 1;
    int c = 0;
    QCoreApplication a(c, nullptr);
    QJSEngine().evaluate(ba);
    return 0;
}
