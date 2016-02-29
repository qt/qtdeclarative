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

#ifndef QSGD3D12ENGINE_P_P_H
#define QSGD3D12ENGINE_P_P_H

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

#include "qsgd3d12engine_p.h"

#include <d3d12.h>
#include <dxgi1_4.h>
#include <wrl/client.h>

using namespace Microsoft::WRL;

// No moc-related features (Q_OBJECT, signals, etc.) can be used here to due
// moc-generated code failing to compile when combined with COM stuff.

QT_BEGIN_NAMESPACE

struct QSGD3D12DescriptorHandle
{
    QSGD3D12DescriptorHandle() { cpu.ptr = 0; gpu.ptr = 0; }
    D3D12_CPU_DESCRIPTOR_HANDLE cpu;
    D3D12_GPU_DESCRIPTOR_HANDLE gpu;
};

class QSGD3D12DescriptorHeapManager
{
public:
    void initialize(ID3D12Device *device);

    void releaseResources();

    enum Flag {
        ShaderVisible = 0x01
    };
    Q_DECLARE_FLAGS(Flags, Flag)

    QSGD3D12DescriptorHandle allocate(D3D12_DESCRIPTOR_HEAP_TYPE type, Flags flags = 0);
    quint32 handleSize(D3D12_DESCRIPTOR_HEAP_TYPE type) const { return m_handleSizes[type]; }

private:
    ID3D12Device *m_device = nullptr;
    struct Heap {
        D3D12_DESCRIPTOR_HEAP_TYPE type;
        ComPtr<ID3D12DescriptorHeap> heap;
        int count = 0;
        QSGD3D12DescriptorHandle start;
        quint32 handleSize;
    };
    QVector<Heap> m_heaps;
    quint32 m_handleSizes[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];
};

class QSGD3D12DeviceManager
{
public:
    ID3D12Device *ref();
    void unref();
    void deviceLossDetected();
    IDXGIFactory4 *dxgi();

    struct DeviceLossObserver {
        virtual void deviceLost() = 0;
    };
    void registerDeviceLossObserver(DeviceLossObserver *observer);

private:
    void ensureCreated();

    ComPtr<ID3D12Device> m_device;
    ComPtr<IDXGIFactory4> m_factory;
    QAtomicInt m_ref;
    QVector<DeviceLossObserver *> m_observers;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QSGD3D12DescriptorHeapManager::Flags)

struct QSGD3D12Fence
{
    ~QSGD3D12Fence() {
        if (event)
            CloseHandle(event);
    }
    ComPtr<ID3D12Fence> fence;
    HANDLE event = nullptr;
    QAtomicInt value;
};

class QSGD3D12EnginePrivate : public QSGD3D12DeviceManager::DeviceLossObserver
{
public:
    void initialize(QWindow *w);
    bool isInitialized() const { return initialized; }
    void releaseResources();
    void resize();

    void beginFrame();
    void endFrame();

    void setPipelineState(const QSGD3D12PipelineState &pipelineState);

    void setVertexBuffer(const quint8 *data, int size);
    void setIndexBuffer(const quint8 *data, int size);
    void setConstantBuffer(const quint8 *data, int size);
    void markConstantBufferDirty(int offset, int size);

    void queueViewport(const QRect &rect);
    void queueScissor(const QRect &rect);
    void queueSetRenderTarget();
    void queueClearRenderTarget(const QColor &color);
    void queueClearDepthStencil(float depthValue, quint8 stencilValue);

    void queueDraw(QSGGeometry::DrawingMode mode, int count, int vboOffset, int vboStride,
                   int cboOffset,
                   int startIndexIndex, QSGD3D12Format indexFormat);

    void present();
    void waitGPU();

    // the device is intentionally hidden here. all resources have to go
    // through the engine and, unlike with GL, cannot just be created in random
    // places due to the need for proper tracking, managing and releasing.
private:
    void setupRenderTargets();
    void deviceLost() override;

    DXGI_SAMPLE_DESC makeSampleDesc(DXGI_FORMAT format, int samples);
    ID3D12Resource *createDepthStencil(D3D12_CPU_DESCRIPTOR_HANDLE viewHandle, const QSize &size, int samples);

    QSGD3D12Fence *createFence() const;
    void waitForGPU(QSGD3D12Fence *f) const;

    void transitionResource(ID3D12Resource *resource, ID3D12GraphicsCommandList *commandList,
                            D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after) const;

    ID3D12Resource *createBuffer(int size);

    ID3D12Resource *backBufferRT() const;
    D3D12_CPU_DESCRIPTOR_HANDLE backBufferRTV() const;

    struct StagingBufferRef {
        const quint8 *p = nullptr;
        int size = 0;
        bool fullChange = true;
        QVector<QPair<int, int> > dirty;
        StagingBufferRef() { dirty.reserve(256); }
    };

    void updateBuffer(StagingBufferRef *br, ID3D12Resource *r, const char *dbgstr);

    bool initialized = false;
    QWindow *window = nullptr;
    int swapChainBufferCount = 2;
    ID3D12Device *device;
    ComPtr<ID3D12CommandQueue> commandQueue;
    ComPtr<IDXGISwapChain3> swapChain;
    ComPtr<ID3D12Resource> renderTargets[2];
    D3D12_CPU_DESCRIPTOR_HANDLE rtv0;
    D3D12_CPU_DESCRIPTOR_HANDLE dsv;
    ComPtr<ID3D12Resource> depthStencil;
    ComPtr<ID3D12CommandAllocator> commandAllocator;
    ComPtr<ID3D12CommandAllocator> bundleAllocator;
    ComPtr<ID3D12GraphicsCommandList> commandList;
    QSGD3D12DescriptorHeapManager descHeapManager;
    QSGD3D12Fence *presentFence = nullptr;

    StagingBufferRef vertexData;
    StagingBufferRef indexData;
    StagingBufferRef constantData;

    ComPtr<ID3D12Resource> vertexBuffer;
    ComPtr<ID3D12Resource> indexBuffer;
    ComPtr<ID3D12Resource> constantBuffer;

    QHash<QSGD3D12PipelineState, ComPtr<ID3D12PipelineState> > m_psoCache;

    ComPtr<ID3D12RootSignature> m_rootSig;
};

QT_END_NAMESPACE

#endif
