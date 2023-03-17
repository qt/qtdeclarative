// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef THIRDPARTYDISPLAY_H
#define THIRDPARTYDISPLAY_H

#include <QColor>
#include <QObject>

class Q_DECL_EXPORT ThirdPartyDisplay : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString content READ content WRITE setContent NOTIFY contentChanged FINAL)
    Q_PROPERTY(QColor foregroundColor READ foregroundColor WRITE setForegroundColor NOTIFY colorsChanged FINAL)
    Q_PROPERTY(QColor backgroundColor READ backgroundColor WRITE setBackgroundColor NOTIFY colorsChanged FINAL)
public:
    const QString &content() const;
    void setContent(const QString &content);

    QColor foregroundColor() const;
    void setForegroundColor(QColor);

    QColor backgroundColor() const;
    void setBackgroundColor(QColor);

signals:
    void contentChanged();
    void colorsChanged();

private:
    QString m_content;
    QColor m_foregroundColor;
    QColor m_backgroundColor;
};

#endif // THIRDPARTYDISPLAY_H
