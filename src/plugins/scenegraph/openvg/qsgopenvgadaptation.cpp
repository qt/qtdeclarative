// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qsgopenvgadaptation_p.h"

#include "qsgopenvgcontext_p.h"
#include "qsgopenvgrenderloop_p.h"

QT_BEGIN_NAMESPACE

QSGOpenVGAdaptation::QSGOpenVGAdaptation(QObject *parent)
    : QSGContextPlugin(parent)
{
}

QStringList QSGOpenVGAdaptation::keys() const
{
    return QStringList() << QLatin1String("openvg");
}

QSGContext *QSGOpenVGAdaptation::create(const QString &key) const
{
    Q_UNUSED(key);
    if (!instance)
        instance = new QSGOpenVGContext();
    return instance;
}

QSGRenderLoop *QSGOpenVGAdaptation::createWindowManager()
{
    return new QSGOpenVGRenderLoop();
}

QSGContextFactoryInterface::Flags QSGOpenVGAdaptation::flags(const QString &key) const
{
    Q_UNUSED(key);
    return 0;
}

QSGOpenVGContext *QSGOpenVGAdaptation::instance = nullptr;

QT_END_NAMESPACE
