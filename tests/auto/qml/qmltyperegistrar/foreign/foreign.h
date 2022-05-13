// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef FOREIGN_H
#define FOREIGN_H

#include <QtCore/qobject.h>
#include <QtCore/qsize.h>

class Foreign : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int things READ things WRITE setThings NOTIFY thingsChanged)

public:
    int things() const;

public slots:
    void setThings(int things);

signals:
    void thingsChanged(int things);

private:
    int m_things = 0;
};

class SizeGadget : public QSize
{
    Q_GADGET
    Q_PROPERTY(int height READ height WRITE setHeight FINAL)
};

#endif // FOREIGN_H
