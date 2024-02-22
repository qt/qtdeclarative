// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef DECLARATIVETESTTYPE_H
#define DECLARATIVETESTTYPE_H

#include <QObject>
#include <qqml.h>

class DeclarativeTestType : public QObject
{
    Q_OBJECT
    QML_ELEMENT

public:
    explicit DeclarativeTestType(QObject *parent = nullptr) : QObject(parent) {}
};

#endif // DECLARATIVETESTTYPE_H
