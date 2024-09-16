// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QTBUG_118112_H
#define QTBUG_118112_H


#include <QtCore/QObject>
#include <QtQml/qqml.h>

class SampleHeader: public QObject {
    Q_OBJECT
};

class SampleUser: public QObject {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(SampleHeader* header READ header WRITE setHeader NOTIFY headerChanged)
    Q_PROPERTY(int invalid READ invalid WRITE setInvalid NOTIFY invalidChanged)

public:
    SampleHeader* header() const {return m_header;}
    void setHeader(SampleHeader* header) {m_header = header;}
    int invalid() const { return m_invalid;}
    void setInvalid(int invalid) { m_invalid = invalid; }

signals:
    void headerChanged();
    void invalidChanged();

private:
    SampleHeader* m_header;
    int m_invalid;
};

#endif // QTBUG_118112_H
