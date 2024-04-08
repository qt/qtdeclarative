// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef VARIANTLIST_QJSON_CONVERSION_HPP
#define VARIANTLIST_QJSON_CONVERSION_HPP

#include "qqmlintegration.h"
#include <QJsonObject>
#include <QJsonArray>
#include <QObject>
#include <QJsonDocument>
#include <QDebug>
#include <QQmlEngine>
#include <private/qjsvalue_p.h>
#include <private/qqmllistwrapper_p.h>
#include <private/qv4engine_p.h>
#include <private/qv4jsonobject_p.h>

class MiscUtils : public QObject
{
    Q_OBJECT
    QML_ELEMENT

public:
    Q_INVOKABLE QVariantList createVariantList() const
    {
        return { QString("cpp"), QString("variant"), QString("list") };
    }

    Q_INVOKABLE QQmlListProperty<QObject> createQmlListProperty()
    {
        QV4::ExecutionEngine engine(qmlEngine(this));
        static QObject objects[] = { QObject{}, QObject{}, QObject{} };
        objects[0].setObjectName("o0");
        objects[1].setObjectName("o1");
        objects[2].setObjectName("o2");
        static QList<QObject *> list{ &objects[0], &objects[1], &objects[2] };
        return QQmlListProperty<QObject>(this, &list);
    }

    Q_INVOKABLE void logArray(const QJsonArray &arr) const
    {
        const auto str = QString(QJsonDocument(arr).toJson(QJsonDocument::Compact));
        qDebug().noquote() << str;
    }

    Q_INVOKABLE void logObject(const QJsonObject &obj) const
    {
        const auto str = QString(QJsonDocument(obj).toJson(QJsonDocument::Compact));
        qDebug().noquote() << str;
    }
};

#endif // VARIANTLIST_QJSON_CONVERSION_HPP
