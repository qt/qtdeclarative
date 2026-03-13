// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include <QtWidgets/qstyleplugin.h>
#include <QtLabsStyleKit/qstylekitstyle.h>

QT_BEGIN_NAMESPACE

class QStyleKitStylePlugin : public QStylePlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QStyleFactoryInterface" FILE "stylekitstyle.json")
public:
    QStyle *create(const QString &key) override;
};

QStyle *QStyleKitStylePlugin::create(const QString &key)
{
    if (key.compare(QLatin1String("StyleKit"), Qt::CaseInsensitive) == 0)
        return new QStyleKitStyle();
    return nullptr;
}

QT_END_NAMESPACE

#include "main.moc"
