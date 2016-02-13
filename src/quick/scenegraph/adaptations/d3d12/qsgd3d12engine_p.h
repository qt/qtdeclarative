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

#ifndef QSGD3D12ENGINE_P_H
#define QSGD3D12ENGINE_P_H

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

#include <QWindow>
#include <qsggeometry.h>

QT_BEGIN_NAMESPACE

// No D3D or COM headers must be pulled in here. All that has to be isolated
// to engine_p_p.h and engine.cpp.

class QSGD3D12EnginePrivate;

// Shader bytecode and other strings are expected to be static so that a
// different pointer == different shader.

enum QSGD3D12Format {
    FmtUnknown = 0,

    FmtFloat4 = 2,    // DXGI_FORMAT_R32G32B32A32_FLOAT
    FmtFloat3 = 6,    // DXGI_FORMAT_R32G32B32_FLOAT
    FmtFloat2 = 16,   // DXGI_FORMAT_R32G32_FLOAT
    FmtFloat = 41,    // DXGI_FORMAT_R32_FLOAT

    // glVertexAttribPointer with GL_UNSIGNED_BYTE and normalized == true maps to the UNORM formats below
    FmtUNormByte4 = 28,   // DXGI_FORMAT_R8G8B8A8_UNORM
    FmtUNormByte2 = 49,   // DXGI_FORMAT_R8G8_UNORM
    FmtUNormByte = 61,    // DXGI_FORMAT_R8_UNORM

    // Index data types
    FmtUnsignedShort = 57,  // DXGI_FORMAT_R16_UINT
    FmtUnsignedInt = 42     // DXGI_FORMAT_R32_UINT
};

struct QSGD3D12InputElement
{
    QSGD3D12InputElement() { }
    QSGD3D12InputElement(const char *name, QSGD3D12Format format, quint32 slot, quint32 offset)
        : name(name), format(format), slot(slot), offset(offset) { }

    const char *name = nullptr;
    QSGD3D12Format format = FmtFloat4;
    quint32 slot = 0;
    quint32 offset = 0;

    bool operator==(const QSGD3D12InputElement &other) const {
        return name == other.name && format == other.format
                && slot == other.slot && offset == other.offset;
    }
};

inline uint qHash(const QSGD3D12InputElement &key, uint seed = 0)
{
    return qHash(key.name, seed) + key.format + key.offset;
}

struct QSGD3D12ShaderState
{
    const quint8 *vs = nullptr;
    quint32 vsSize = 0;
    const quint8 *ps = nullptr;
    quint32 psSize = 0;

    bool operator==(const QSGD3D12ShaderState &other) const {
        return vs == other.vs && vsSize == other.vsSize
                && ps == other.ps && psSize == other.psSize;
    }
};

inline uint qHash(const QSGD3D12ShaderState &key, uint seed = 0)
{
    return qHash(key.vs, seed) + key.vsSize + qHash(key.ps, seed) + key.psSize;
}

struct QSGD3D12PipelineState
{
    enum CullMode {
        CullNone = 1,
        CullFront,
        CullBack
    };

    enum DepthFunc {
        DepthNever = 1,
        DepthLess,
        DepthEqual,
        DepthLessEqual,
        DepthGreater,
        DepthNotEqual,
        DepthGreaterEqual,
        DepthAlways
    };

    enum TopologyType {
        TopologyTypePoint = 1,
        TopologyTypeLine,
        TopologyTypeTriangle
    };

    QSGD3D12ShaderState shaders;

    QVector<QSGD3D12InputElement> inputElements;

    CullMode cullMode = CullNone;
    bool frontCCW = true;
    bool premulBlend = false; // == GL_ONE, GL_ONE_MINUS_SRC_ALPHA
    bool depthEnable = true;
    DepthFunc depthFunc = DepthLess;
    bool depthWrite = true;
    bool stencilEnable = false;
    // ### stencil stuff
    TopologyType topologyType = TopologyTypeTriangle;

    bool operator==(const QSGD3D12PipelineState &other) const {
        return shaders == other.shaders
                && inputElements == other.inputElements
                && cullMode == other.cullMode
                && frontCCW == other.frontCCW
                && premulBlend == other.premulBlend
                && depthEnable == other.depthEnable
                && depthFunc == other.depthFunc
                && depthWrite == other.depthWrite
                && stencilEnable == other.stencilEnable
                && topologyType == other.topologyType;
    }
};

inline uint qHash(const QSGD3D12PipelineState &key, uint seed = 0)
{
    return qHash(key.shaders, seed) + qHash(key.inputElements, seed)
            + key.cullMode + key.frontCCW + key.premulBlend + key.depthEnable
            + key.depthFunc + key.depthWrite + key.stencilEnable + key.topologyType;
}

class QSGD3D12Engine
{
public:
    QSGD3D12Engine();
    ~QSGD3D12Engine();

    bool attachToWindow(QWindow *window);
    void releaseResources();
    void resize();

    void beginFrame();
    void endFrame();

    void setPipelineState(const QSGD3D12PipelineState &pipelineState);

    void setVertexBuffer(const quint8 *data, int size);
    void setIndexBuffer(const quint8 *data, int size);
    void setConstantBuffer(const quint8 *data, int size);

    void queueViewport(const QRect &rect);
    void queueScissor(const QRect &rect);
    void queueSetRenderTarget();
    void queueClearRenderTarget(const QColor &color);
    void queueClearDepthStencil(float depthValue, quint8 stencilValue);

    void queueDraw(QSGGeometry::DrawingMode mode, int count, int vboOffset, int vboStride,
                   int cboOffset,
                   int startIndexIndex = -1, QSGD3D12Format indexFormat = FmtUnsignedShort);

    void present();
    void waitGPU();

    static quint32 alignedConstantBufferSize(quint32 size);

private:
    QSGD3D12EnginePrivate *d;
    Q_DISABLE_COPY(QSGD3D12Engine)
};

QT_END_NAMESPACE

#endif // QSGD3D12ENGINE_P_H
