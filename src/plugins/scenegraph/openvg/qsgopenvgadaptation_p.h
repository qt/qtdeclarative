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
