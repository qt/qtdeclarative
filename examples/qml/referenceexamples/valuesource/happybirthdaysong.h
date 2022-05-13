// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef HAPPYBIRTHDAYSONG_H
#define HAPPYBIRTHDAYSONG_H

#include <QQmlPropertyValueSource>
#include <QQmlProperty>
#include <qqml.h>

#include <QStringList>

// ![0]
class HappyBirthdaySong : public QObject, public QQmlPropertyValueSource
{
    Q_OBJECT
    Q_INTERFACES(QQmlPropertyValueSource)
// ![0]
    Q_PROPERTY(QString name READ name WRITE setName)
// ![1]
    QML_ELEMENT
public:
    explicit HappyBirthdaySong(QObject *parent = nullptr);

    void setTarget(const QQmlProperty &) override;
// ![1]

    QString name() const;
    void setName(const QString &);

private slots:
    void advance();

private:
    qsizetype m_line = -1;
    QStringList m_lyrics;
    QQmlProperty m_target;
    QString m_name;
// ![2]
};
// ![2]

#endif // HAPPYBIRTHDAYSONG_H

