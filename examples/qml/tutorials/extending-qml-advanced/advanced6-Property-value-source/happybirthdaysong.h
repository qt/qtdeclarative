// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef HAPPYBIRTHDAYSONG_H
#define HAPPYBIRTHDAYSONG_H

#include <QQmlProperty>
#include <QQmlPropertyValueSource>
#include <qqml.h>
#include <QStringList>

class HappyBirthdaySong : public QObject, public QQmlPropertyValueSource
{
    Q_OBJECT
    Q_INTERFACES(QQmlPropertyValueSource)
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged FINAL)
    QML_ELEMENT
public:
    explicit HappyBirthdaySong(QObject *parent = nullptr);

    void setTarget(const QQmlProperty &) override;

    QString name() const;
    void setName(const QString &);

signals:
    void nameChanged();

private slots:
    void advance();

private:
    qsizetype m_line = -1;
    QStringList m_lyrics;
    QQmlProperty m_target;
    QString m_name;
};

#endif // HAPPYBIRTHDAYSONG_H
