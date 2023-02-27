// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQUICKNINEPATCHCONFIGREADER_P_H
#define QQUICKNINEPATCHCONFIGREADER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qobject.h>
#include <QtCore/qjsondocument.h>
#include <QtCore/qjsonobject.h>
#include <QtQuick/qquickitem.h>

QT_BEGIN_NAMESPACE

class QQuickNinePatchConfigReader : public QQuickItem // should be just a QObject but it was having type registration problems
{
    Q_OBJECT
    Q_PROPERTY(QString configPath READ configPath NOTIFY configPathChanged FINAL)
    Q_PROPERTY(QJsonObject verticalStretch READ verticalStretch NOTIFY configPathChanged FINAL)
    Q_PROPERTY(QJsonObject horizontalStretch READ horizontalStretch NOTIFY configPathChanged FINAL)
    Q_PROPERTY(QJsonObject paddings READ paddings NOTIFY configPathChanged FINAL)
    Q_PROPERTY(QJsonObject insets READ insets NOTIFY configPathChanged FINAL)
    QML_NAMED_ELEMENT(NinePatchConfigReader)
    QML_ADDED_IN_VERSION(6, 6)

public:
    explicit QQuickNinePatchConfigReader(QQuickItem *parent = nullptr);

    QString configPath() const;
    void setConfigPath(const QString &path);

    QJsonObject verticalStretch() const;
    QJsonObject horizontalStretch() const;
    QJsonObject paddings() const;
    QJsonObject insets() const;

Q_SIGNALS:
    void configPathChanged();
    void verticalStretchChanged();
    void horizontalStretchChanged();
    void paddingsChanged();
    void insetsChanged();

protected:

private:
    Q_DISABLE_COPY(QQuickNinePatchConfigReader)

    void updateConfigPath(const QString &path);
    void parseConfig();

    QString m_configPath;
    QJsonObject m_verticalStretch;
    QJsonObject m_horizontalStretch;
    QJsonObject m_paddings;
    QJsonObject m_insets;
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QQuickNinePatchConfigReader)

#endif // QQUICKNINEPATCHCONFIGREADER_P_H
