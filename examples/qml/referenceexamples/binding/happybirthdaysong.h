// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
#ifndef HAPPYBIRTHDAYSONG_H
#define HAPPYBIRTHDAYSONG_H

#include <QQmlPropertyValueSource>
#include <QQmlProperty>
#include <qqml.h>

#include <QStringList>
#include <qqml.h>

class HappyBirthdaySong : public QObject, public QQmlPropertyValueSource
{
    Q_OBJECT
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
    Q_INTERFACES(QQmlPropertyValueSource)
    QML_ELEMENT
public:
    explicit HappyBirthdaySong(QObject *parent = nullptr);

    void setTarget(const QQmlProperty &) override;

    QString name() const;
    void setName(const QString &);

private slots:
    void advance();

signals:
    void nameChanged();
private:
    qsizetype m_line = -1;
    QStringList m_lyrics;
    QQmlProperty m_target;
    QString m_name;
};

#endif // HAPPYBIRTHDAYSONG_H

