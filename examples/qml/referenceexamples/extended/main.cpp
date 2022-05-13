// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
#include <QApplication>
#include <QQmlEngine>
#include <QQmlComponent>
#include <QDebug>
#include <QLineEdit>
#include "lineedit.h"

int main(int argc, char ** argv)
{
    QApplication app(argc, argv);

// ![1]
    QQmlEngine engine;
    QQmlComponent component(&engine, QUrl("qrc:example.qml"));
    auto *edit = qobject_cast<QLineEdit *>(component.create());
// ![1]

    if (edit) {
        edit->show();
        return QApplication::exec();
    }

    qWarning() << component.errors();
    return EXIT_FAILURE;
}
