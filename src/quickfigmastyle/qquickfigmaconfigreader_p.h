// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQUICKFIGMACONFIGREADER_P_H
#define QQUICKFIGMACONFIGREADER_P_H

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
#include <QtQml/qqml.h>
#include <QtCore/QVariantMap>

QT_BEGIN_NAMESPACE

class QQuickFigmaConfigReader : public QObject
{
    Q_OBJECT
    QML_SINGLETON
    Q_PROPERTY(QString configPath READ configPath NOTIFY configPathChanged FINAL)
    Q_PROPERTY(QVariantMap images READ images CONSTANT FINAL)
    QML_NAMED_ELEMENT(ConfigReader)
    QML_ADDED_IN_VERSION(6, 6)

public:
    explicit QQuickFigmaConfigReader(QObject *parent = nullptr);

    QString configPath() const;
    void setConfigPath(const QString &path);

    QVariantMap images() const;

Q_SIGNALS:
    void configPathChanged();

private:
    Q_DISABLE_COPY(QQuickFigmaConfigReader)

    void updateConfigPath(const QString &path);
    void parseConfig();

    QString m_configPath;
    QVariantMap m_imagesConfig;
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QQuickFigmaConfigReader)

#endif // QQUICKFIGMACONFIGREADER_P_H
