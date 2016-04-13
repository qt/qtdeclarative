/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef D3D12RENDERER_H
#define D3D12RENDERER_H

#include "customrenderitem.h"
#include <qsgrendernode.h>

#ifdef HAS_D3D12

#include <d3d12.h>
#include <wrl/client.h>

using namespace Microsoft::WRL;

class D3D12Renderer : public CustomRenderer
{
public:
    D3D12Renderer(QQuickItem *item, QSGRenderNode *node);
    ~D3D12Renderer();
    void init() override;
    void render(const QSGRenderNode::RenderState *state) override;

private:
    QQuickItem *m_item;
    QSGRenderNode *m_node;
    ID3D12Device *m_device;
    ComPtr<ID3D12PipelineState> pipelineState;
    ComPtr<ID3D12RootSignature> rootSignature;
    ComPtr<ID3D12Resource> vertexBuffer;
    ComPtr<ID3D12Resource> constantBuffer;
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
    quint8 *vbPtr;
    quint8 *cbPtr;
};

#endif // HAS_D3D12

#endif
