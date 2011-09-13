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

#ifndef CUSTOM_PARTICLE_H
#define CUSTOM_PARTICLE_H
#include "qsgparticlepainter_p.h"
#include <QtDeclarative/private/qsgshadereffectnode_p.h>
#include <QSignalMapper>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Declarative)

class QSGNode;
struct PlainVertices;

class QSGShaderEffectMaterialObject;

//Genealogy: Hybrid of UltraParticle and ShaderEffect
class QSGCustomParticle : public QSGParticlePainter
{
    Q_OBJECT
    Q_PROPERTY(QByteArray fragmentShader READ fragmentShader WRITE setFragmentShader NOTIFY fragmentShaderChanged)
    Q_PROPERTY(QByteArray vertexShader READ vertexShader WRITE setVertexShader NOTIFY vertexShaderChanged)

public:
    explicit QSGCustomParticle(QSGItem* parent=0);
    ~QSGCustomParticle();

    QByteArray fragmentShader() const { return m_source.fragmentCode; }
    void setFragmentShader(const QByteArray &code);

    QByteArray vertexShader() const { return m_source.vertexCode; }
    void setVertexShader(const QByteArray &code);
public Q_SLOTS:
    void updateData();
    void changeSource(int);
Q_SIGNALS:
    void fragmentShaderChanged();
    void vertexShaderChanged();
protected:
    virtual void initialize(int gIdx, int pIdx);
    virtual void commit(int gIdx, int pIdx);

    QSGNode *updatePaintNode(QSGNode *, UpdatePaintNodeData *);
    void prepareNextFrame();
    void setSource(const QVariant &var, int index);
    void disconnectPropertySignals();
    void connectPropertySignals();
    void reset();
    void resize(int oldCount, int newCount);
    void updateProperties();
    void lookThroughShaderCode(const QByteArray &code);
    virtual void componentComplete();
    QSGShaderEffectNode *buildCustomNodes();
    void performPendingResize();

private:
    void buildData();


    bool m_pleaseReset;
    bool m_dirtyData;
    QSGShaderEffectProgram m_source;
    struct SourceData
    {
        QSignalMapper *mapper;
        QPointer<QSGItem> item;
        QByteArray name;
    };
    QVector<SourceData> m_sources;
    QSGShaderEffectMaterialObject *m_material;
    QSGShaderEffectNode* m_rootNode;
    QHash<int, QSGShaderEffectNode*> m_nodes;
    qreal m_lastTime;

};

QT_END_NAMESPACE

QT_END_HEADER

#endif //HEADER_GUARD
