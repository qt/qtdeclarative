/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQuick module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qsgdummyadaptation_p.h"

#include <private/qguiapplication_p.h>
#include <qpa/qplatformintegration.h>

QT_BEGIN_NAMESPACE

class QSGDummyContext : public QSGContext
{
public:
    QSGDummyContext(QObject *parent = 0) : QSGContext(parent) { }

    QSGRenderContext *createRenderContext() override { return QSGContext::createRenderContext(); }
    QSGRectangleNode *createRectangleNode() override { return QSGContext::createRectangleNode(); }
    QSGImageNode *createImageNode() override { return QSGContext::createImageNode(); }
    QSGPainterNode *createPainterNode(QQuickPaintedItem *item) override { return QSGContext::createPainterNode(item); }
    QSGGlyphNode *createGlyphNode(QSGRenderContext *rc, bool preferNativeGlyphNode) override  { return QSGContext::createGlyphNode(rc, preferNativeGlyphNode); }
    QSGNinePatchNode *createNinePatchNode() override { return QSGContext::createNinePatchNode(); }
    QSGLayer *createLayer(QSGRenderContext *rc) override { return QSGContext::createLayer(rc); }
    QSurfaceFormat defaultSurfaceFormat() const override { return QSGContext::defaultSurfaceFormat(); }
};

QSGDummyAdaptation::QSGDummyAdaptation(QObject *parent)
    : QSGContextPlugin(parent)
{
}

QStringList QSGDummyAdaptation::keys() const
{
    return QStringList() << QLatin1String("dummy");
}

QSGContext *QSGDummyAdaptation::create(const QString &) const
{
    if (!contextInstance) {
        qDebug("Creating OpenGL SG context via dummy");
        contextInstance = new QSGDummyContext;
    }

    return contextInstance;
}

QSGRenderLoop *QSGDummyAdaptation::createWindowManager()
{
    qDebug("Creating default OpenGL render loop via dummy");
    return nullptr;
}

QSGDummyContext *QSGDummyAdaptation::contextInstance = nullptr;

QT_END_NAMESPACE
