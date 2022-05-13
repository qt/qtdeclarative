// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef DATAOBJECT_H
#define DATAOBJECT_H

#include <QObject>

//![0]
class DataObject : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
    Q_PROPERTY(QString color READ color WRITE setColor NOTIFY colorChanged)
//![0]

public:
    DataObject(QObject *parent=0);
    DataObject(const QString &name, const QString &color, QObject *parent=0);

    QString name() const;
    void setName(const QString &name);

    QString color() const;
    void setColor(const QString &color);

signals:
    void nameChanged();
    void colorChanged();

private:
    QString m_name;
    QString m_color;
//![1]
};
//![1]

#endif // DATAOBJECT_H
