/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQuick module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

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
    Q_UNUSED(key)
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
    Q_UNUSED(key)
    return 0;
}

QSGOpenVGContext *QSGOpenVGAdaptation::instance = nullptr;

QT_END_NAMESPACE
