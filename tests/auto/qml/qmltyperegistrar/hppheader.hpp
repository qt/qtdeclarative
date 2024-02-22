// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef HPPHEADER_HPP
#define HPPHEADER_HPP

#include <QtCore/qobject.h>
#include <QtQml/qqml.h>

class HppClass : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(int eieiei READ eieiei WRITE setEieiei NOTIFY eieieiChanged)
    Q_PROPERTY(int eieiei2 READ eieiei2)

public:
    int eieiei() const { return m_eieiei; }
    int eieiei2() const { return m_eieiei2; }

public slots:
    void setEieiei(int eieiei)
    {
        if (m_eieiei == eieiei)
            return;

        m_eieiei = eieiei;
        emit eieieiChanged(m_eieiei);
    }

signals:
    void eieieiChanged(int eieiei);

private:
    int m_eieiei;
    int m_eieiei2;
};

#endif // HPPHEADER_HPP
