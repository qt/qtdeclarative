// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSGOPENVGADAPTATION_H
#define QSGOPENVGADAPTATION_H

#include <private/qsgcontextplugin_p.h>

QT_BEGIN_NAMESPACE

class QSGContext;
class QSGRenderLoop;
class QSGOpenVGContext;

class QSGOpenVGAdaptation : public QSGContextPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QSGContextFactoryInterface" FILE "openvg.json")
public:
    QSGOpenVGAdaptation(QObject *parent = nullptr);

    QStringList keys() const override;
    QSGContext *create(const QString &key) const override;
    QSGRenderLoop *createWindowManager() override;
    Flags flags(const QString &key) const override;
private:
    static QSGOpenVGContext *instance;
};

QT_END_NAMESPACE

#endif // QSGOPENVGADAPTATION_H
