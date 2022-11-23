// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef MULTIFOREIGN_H
#define MULTIFOREIGN_H

#include <QtCore/qobject.h>
#include <QtQml/qqml.h>

class Autochthon : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString location READ location WRITE setLocation NOTIFY locationChanged FINAL)

public:
    const QString &location() const
    {
        return m_location;
    }

    void setLocation(const QString &newLocation)
    {
        if (m_location == newLocation)
            return;
        m_location = newLocation;
        emit locationChanged();
    }

signals:
    void locationChanged();

private:
    QString m_location;
};

struct Foreign1
{
    Q_GADGET
    QML_FOREIGN(Autochthon)
    QML_ELEMENT
};

struct Foreign2
{
    Q_GADGET
    QML_FOREIGN(Autochthon)
    QML_ELEMENT
};


#endif // MULTIFOREIGN_H
