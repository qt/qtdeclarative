// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
#ifndef UNDEFINEDHOLDER_H
#define UNDEFINEDHOLDER_H

#include <QtCore/qjsonvalue.h>
#include <QtCore/qobject.h>
#include <QtQml/qjsprimitivevalue.h>
#include <QtQml/qqml.h>

class UndefinedHolder : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QJsonValue json MEMBER m_json NOTIFY jsonChanged)
    Q_PROPERTY(QJSPrimitiveValue primitive MEMBER m_primitive NOTIFY primitiveChanged)

public:
    QJsonValue m_json = QJsonValue(42);
    QJSPrimitiveValue m_primitive = QJSPrimitiveValue(42);

signals:
    void jsonChanged();
    void primitiveChanged();
};

#endif
