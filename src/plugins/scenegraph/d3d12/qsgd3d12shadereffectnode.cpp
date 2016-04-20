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

#include "qsgd3d12shadereffectnode_p.h"
#include "qsgd3d12rendercontext_p.h"
#include "qsgd3d12engine_p.h"
#include <QtCore/qfile.h>
#include <QtQml/qqmlfile.h>

#include <d3d12shader.h>
#include <d3dcompiler.h>

QT_BEGIN_NAMESPACE

// NOTE: Avoid categorized logging. It is slow.

#define DECLARE_DEBUG_VAR(variable) \
    static bool debug_ ## variable() \
    { static bool value = qgetenv("QSG_RENDERER_DEBUG").contains(QT_STRINGIFY(variable)); return value; }

DECLARE_DEBUG_VAR(render)

QSGD3D12ShaderEffectNode::QSGD3D12ShaderEffectNode(QSGD3D12RenderContext *rc, QSGD3D12GuiThreadShaderEffectManager *mgr)
    : QSGShaderEffectNode(mgr),
      m_rc(rc),
      m_mgr(mgr)
{
    // ### no material yet, it will just crash
    //setMaterial(&m_material);
}

QRectF QSGD3D12ShaderEffectNode::normalizedTextureSubRect() const
{
    return QRectF(0, 0, 1, 1);
    // ###
}

void QSGD3D12ShaderEffectNode::sync(SyncData *syncData)
{
    if (Q_UNLIKELY(debug_render()))
        qDebug() << "shadereffect node sync" << syncData->dirty;

    // ###
}

QSGGuiThreadShaderEffectManager::ShaderType QSGD3D12GuiThreadShaderEffectManager::shaderType() const
{
    return HLSL;
}

int QSGD3D12GuiThreadShaderEffectManager::shaderCompilationType() const
{
    return OfflineCompilation;
}

int QSGD3D12GuiThreadShaderEffectManager::shaderSourceType() const
{
    return ShaderByteCode;
}

bool QSGD3D12GuiThreadShaderEffectManager::hasSeparateSamplerAndTextureObjects() const
{
    return true;
}

QString QSGD3D12GuiThreadShaderEffectManager::log() const
{
    return QString();
}

QSGGuiThreadShaderEffectManager::Status QSGD3D12GuiThreadShaderEffectManager::status() const
{
    return Compiled;
}

struct RefGuard {
    RefGuard(IUnknown *p) : p(p) { }
    ~RefGuard() { p->Release(); }
    IUnknown *p;
};

bool QSGD3D12GuiThreadShaderEffectManager::reflect(const QByteArray &src, ShaderInfo *result)
{
    const QString fn = QQmlFile::urlToLocalFileOrQrc(src);
    QFile f(fn);
    if (!f.open(QIODevice::ReadOnly)) {
        qWarning("ShaderEffect: Failed to read %s", qPrintable(fn));
        return false;
    }
    result->blob = f.readAll();
    f.close();

    ID3D12ShaderReflection *reflector;
    HRESULT hr = D3DReflect(result->blob.constData(), result->blob.size(), IID_PPV_ARGS(&reflector));
    if (FAILED(hr)) {
        qWarning("D3D shader reflection failed: 0x%x", hr);
        return false;
    }
    RefGuard rg(reflector);

    D3D12_SHADER_DESC shaderDesc;
    reflector->GetDesc(&shaderDesc);

    const uint progType = (shaderDesc.Version & 0xFFFF0000) >> 16;
    const uint major = (shaderDesc.Version & 0x000000F0) >> 4;
    const uint minor = (shaderDesc.Version & 0x0000000F);

    switch (progType) {
    case D3D12_SHVER_VERTEX_SHADER:
        result->type = ShaderInfo::TypeVertex;
        break;
    case D3D12_SHVER_PIXEL_SHADER:
        result->type = ShaderInfo::TypeFragment;
        break;
    default:
        result->type = ShaderInfo::TypeOther;
        qWarning("D3D shader is of unknown type 0x%x", shaderDesc.Version);
        return false;
    }

    if (major < 5) {
        qWarning("D3D shader model version %u.%u is too low", major, minor);
        return false;
    }

    const int ieCount = shaderDesc.InputParameters;
    const int cbufferCount = shaderDesc.ConstantBuffers;
    const int boundResCount = shaderDesc.BoundResources;

    if (ieCount < 1) {
        qWarning("Invalid shader: Not enough input parameters (%d)", ieCount);
        return false;
    }
    if (cbufferCount < 1) {
        qWarning("Invalid shader: Shader has no constant buffers");
        return false;
    }
    if (boundResCount < 1) {
        qWarning("Invalid shader: No resources bound. Expected to have at least a constant buffer bound.");
        return false;
    }

    if (Q_UNLIKELY(debug_render()))
        qDebug("Shader reflection size %d type %d v%u.%u input elems %d cbuffers %d boundres %d",
               result->blob.size(), result->type, major, minor, ieCount, cbufferCount, boundResCount);

    for (int i = 0; i < ieCount; ++i) {
        D3D12_SIGNATURE_PARAMETER_DESC desc;
        if (FAILED(reflector->GetInputParameterDesc(i, &desc))) {
            qWarning("D3D reflection: Failed to query input parameter %d", i);
            return false;
        }
        if (desc.SystemValueType != D3D_NAME_UNDEFINED)
            continue;
        ShaderInfo::InputParameter param;
        param.semanticName = QByteArray(desc.SemanticName);
        param.semanticIndex = desc.SemanticIndex;
        result->inputParameters.append(param);
    }

    for (int i = 0; i < boundResCount; ++i) {
        D3D12_SHADER_INPUT_BIND_DESC desc;
        if (FAILED(reflector->GetResourceBindingDesc(i, &desc))) {
            qWarning("D3D reflection: Failed to query resource binding %d", i);
            continue;
        }
        bool gotCBuffer = false;
        if (desc.Type == D3D_SIT_CBUFFER) {
            ID3D12ShaderReflectionConstantBuffer *cbuf = reflector->GetConstantBufferByName(desc.Name);
            D3D12_SHADER_BUFFER_DESC bufDesc;
            if (FAILED(cbuf->GetDesc(&bufDesc))) {
                qWarning("D3D reflection: Failed to query constant buffer description");
                continue;
            }
            if (gotCBuffer) {
                qWarning("D3D reflection: Found more than one constant buffers. Only the first one is used.");
                continue;
            }
            gotCBuffer = true;
            for (uint cbIdx = 0; cbIdx < bufDesc.Variables; ++cbIdx) {
                ID3D12ShaderReflectionVariable *cvar = cbuf->GetVariableByIndex(cbIdx);
                D3D12_SHADER_VARIABLE_DESC varDesc;
                if (FAILED(cvar->GetDesc(&varDesc))) {
                    qWarning("D3D reflection: Failed to query constant buffer variable %d", cbIdx);
                    return false;
                }
                ShaderInfo::Variable v;
                v.type = ShaderInfo::Constant;
                v.name = QByteArray(varDesc.Name);
                v.offset = varDesc.StartOffset;
                v.size = varDesc.Size;
                result->variables.append(v);
            }
        } else if (desc.Type == D3D_SIT_TEXTURE) {
            if (desc.Dimension != D3D_SRV_DIMENSION_TEXTURE2D) {
                qWarning("D3D reflection: Texture %s is not a 2D texture, ignoring.", qPrintable(desc.Name));
                continue;
            }
            if (desc.NumSamples != (UINT) -1) {
                qWarning("D3D reflection: Texture %s is multisample (%u), ignoring.", qPrintable(desc.Name), desc.NumSamples);
                continue;
            }
            if (desc.BindCount != 1) {
                qWarning("D3D reflection: Texture %s is an array, ignoring.", qPrintable(desc.Name));
                continue;
            }
            if (desc.Space != 0) {
                qWarning("D3D reflection: Texture %s is not using register space 0, ignoring.", qPrintable(desc.Name));
                continue;
            }
            ShaderInfo::Variable v;
            v.type = ShaderInfo::Texture;
            v.name = QByteArray(desc.Name);
            v.bindPoint = desc.BindPoint;
            result->variables.append(v);
        } else if (desc.Type == D3D_SIT_SAMPLER) {
            if (desc.BindCount != 1) {
                qWarning("D3D reflection: Sampler %s is an array, ignoring.", qPrintable(desc.Name));
                continue;
            }
            if (desc.Space != 0) {
                qWarning("D3D reflection: Sampler %s is not using register space 0, ignoring.", qPrintable(desc.Name));
                continue;
            }
            ShaderInfo::Variable v;
            v.type = ShaderInfo::Sampler;
            v.name = QByteArray(desc.Name);
            v.bindPoint = desc.BindPoint;
            result->variables.append(v);
        } else {
            qWarning("D3D reflection: Resource binding %d has an unknown type of %d and will be ignored.", i, desc.Type);
            continue;
        }
    }

    if (Q_UNLIKELY(debug_render())) {
        for (int i = 0; i < result->inputParameters.count(); ++i) {
            const ShaderInfo::InputParameter &p(result->inputParameters.at(i));
            qDebug() << "input" << i << p;
        }
        for (int i = 0; i < result->variables.count(); ++i) {
            const ShaderInfo::Variable &v(result->variables.at(i));
            qDebug() << "var" << i << v;
        }
    }

    return true;
}

QT_END_NAMESPACE
