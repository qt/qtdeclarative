/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtDeclarative module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QQUICKSHADEREFFECT_P_H
#define QQUICKSHADEREFFECT_P_H

#include "qquickitem.h"

#include "qsgmaterial.h"
#include <private/qsgadaptationlayer_p.h>
#include <private/qquickshadereffectnode_p.h>
#include "qquickshadereffectmesh_p.h"

#include <QtCore/qpointer.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Declarative)

const char *qtPositionAttributeName();
const char *qtTexCoordAttributeName();

class QSGContext;
class QSignalMapper;
class QQuickCustomMaterialShader;

class QQuickShaderEffect : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(QByteArray fragmentShader READ fragmentShader WRITE setFragmentShader NOTIFY fragmentShaderChanged)
    Q_PROPERTY(QByteArray vertexShader READ vertexShader WRITE setVertexShader NOTIFY vertexShaderChanged)
    Q_PROPERTY(bool blending READ blending WRITE setBlending NOTIFY blendingChanged)
    Q_PROPERTY(QVariant mesh READ mesh WRITE setMesh NOTIFY meshChanged)
    Q_PROPERTY(CullMode culling READ cullMode WRITE setCullMode NOTIFY cullModeChanged)
    Q_ENUMS(CullMode)

public:
    enum CullMode
    {
        NoCulling = QQuickShaderEffectMaterial::NoCulling,
        BackFaceCulling = QQuickShaderEffectMaterial::BackFaceCulling,
        FrontFaceCulling = QQuickShaderEffectMaterial::FrontFaceCulling
    };

    QQuickShaderEffect(QQuickItem *parent = 0);
    ~QQuickShaderEffect();

    virtual void componentComplete();

    QByteArray fragmentShader() const { return m_source.fragmentCode; }
    void setFragmentShader(const QByteArray &code);

    QByteArray vertexShader() const { return m_source.vertexCode; }
    void setVertexShader(const QByteArray &code);

    bool blending() const { return m_blending; }
    void setBlending(bool enable);

    QVariant mesh() const;
    void setMesh(const QVariant &mesh);

    CullMode cullMode() const { return m_cullMode; }
    void setCullMode(CullMode face);

Q_SIGNALS:
    void fragmentShaderChanged();
    void vertexShaderChanged();
    void blendingChanged();
    void meshChanged();
    void cullModeChanged();

protected:
    virtual void geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry);
    virtual QSGNode *updatePaintNode(QSGNode *, UpdatePaintNodeData *);

private Q_SLOTS:
    void changeSource(int index);
    void updateData();
    void updateGeometry();

private:
    friend class QQuickCustomMaterialShader;
    friend class QQuickShaderEffectNode;

    void setSource(const QVariant &var, int index);
    void disconnectPropertySignals();
    void connectPropertySignals();
    void reset();
    void updateProperties();
    void lookThroughShaderCode(const QByteArray &code);

    QQuickShaderEffectProgram m_source;
    QSize m_meshResolution;
    QQuickShaderEffectMesh *m_mesh;
    QQuickGridMesh m_defaultMesh;
    CullMode m_cullMode;

    struct SourceData
    {
        QSignalMapper *mapper;
        QPointer<QQuickItem> sourceObject;
        QByteArray name;
    };
    QVector<SourceData> m_sources;

    uint m_blending : 1;
    uint m_dirtyData : 1;

    uint m_programDirty : 1;
    uint m_dirtyMesh : 1;
    uint m_dirtyGeometry : 1;
};

// TODO: Remove after grace period.
class QQuickShaderEffectItem : public QQuickShaderEffect
{
public:
    QQuickShaderEffectItem(QQuickItem *parent = 0);
};

QT_END_NAMESPACE

QT_END_HEADER

#endif // QQUICKSHADEREFFECT_P_H
