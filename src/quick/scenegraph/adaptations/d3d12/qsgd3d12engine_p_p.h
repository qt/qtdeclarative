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
#include <QCache>

#include <d3d12.h>
#include <dxgi1_4.h>
#include <wrl/client.h>

using namespace Microsoft::WRL;

// No moc-related features (Q_OBJECT, signals, etc.) can be used here to due
// moc-generated code failing to compile when combined with COM stuff.

// Recommended reading before moving further: https://github.com/Microsoft/DirectXTK/wiki/ComPtr
// Note esp. operator= vs. Attach and operator& vs. GetAddressOf

// ID3D12* is never passed to Qt containers directly. Always use ComPtr and put it into a struct.

QT_BEGIN_NAMESPACE

class QSGD3D12CPUDescriptorHeapManager
{
public:
    void initialize(ID3D12Device *device);

    void releaseResources();

    D3D12_CPU_DESCRIPTOR_HANDLE allocate(D3D12_DESCRIPTOR_HEAP_TYPE type);
    void release(D3D12_CPU_DESCRIPTOR_HANDLE handle, D3D12_DESCRIPTOR_HEAP_TYPE type);
    quint32 handleSize(D3D12_DESCRIPTOR_HEAP_TYPE type) const { return m_handleSizes[type]; }

private:
    ID3D12Device *m_device = nullptr;
    struct Heap {
        D3D12_DESCRIPTOR_HEAP_TYPE type;
        ComPtr<ID3D12DescriptorHeap> heap;
        D3D12_CPU_DESCRIPTOR_HANDLE start;
        quint32 handleSize;
        quint32 freeMap[8];
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

struct QSGD3D12CPUWaitableFence
{
    ~QSGD3D12CPUWaitableFence() {
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

    void resetVertexBuffer(const quint8 *data, int size);
    void markVertexBufferDirty(int offset, int size);
    void resetIndexBuffer(const quint8 *data, int size);
    void markIndexBufferDirty(int offset, int size);
    void resetConstantBuffer(const quint8 *data, int size);
    void markConstantBufferDirty(int offset, int size);

    void queueViewport(const QRect &rect);
    void queueScissor(const QRect &rect);
    void queueSetRenderTarget();
    void queueClearRenderTarget(const QColor &color);
    void queueClearDepthStencil(float depthValue, quint8 stencilValue, QSGD3D12Engine::ClearFlags which);
    void queueSetStencilRef(quint32 ref);

    void finalizePipeline(const QSGD3D12PipelineState &pipelineState);

    void queueDraw(QSGGeometry::DrawingMode mode, int count,
                   int vboOffset, int vboSize, int vboStride,
                   int cboOffset,
                   int startIndexIndex, QSGD3D12Format indexFormat);

    void present();
    void waitGPU();

    uint genTexture();
    void createTextureAsync(uint id, const QImage &image, QSGD3D12Engine::TextureCreateFlags flags);
    void releaseTexture(uint id);
    SIZE_T textureSRV(uint id) const;
    void activateTexture(uint id);

    // the device is intentionally hidden here. all resources have to go
    // through the engine and, unlike with GL, cannot just be created in random
    // places due to the need for proper tracking, managing and releasing.
private:
    void setupRenderTargets();
    void deviceLost() override;

    bool createCbvSrvUavHeap(int pframeIndex, int descriptorCount);
    void setDescriptorHeaps(bool force = false);
    void ensureGPUDescriptorHeap(int cbvSrvUavDescriptorCount);

    DXGI_SAMPLE_DESC makeSampleDesc(DXGI_FORMAT format, int samples);
    ID3D12Resource *createDepthStencil(D3D12_CPU_DESCRIPTOR_HANDLE viewHandle, const QSize &size, int samples);

    QSGD3D12CPUWaitableFence *createCPUWaitableFence() const;
    void waitForGPU(QSGD3D12CPUWaitableFence *f) const;

    void transitionResource(ID3D12Resource *resource, ID3D12GraphicsCommandList *commandList,
                            D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after) const;

    void uavBarrier(ID3D12Resource *resource, ID3D12GraphicsCommandList *commandList) const;

    ID3D12Resource *createBuffer(int size);

    ID3D12Resource *backBufferRT() const;
    D3D12_CPU_DESCRIPTOR_HANDLE backBufferRTV() const;

    struct CPUBufferRef {
        const quint8 *p = nullptr;
        quint32 size = 0;
        bool allDirty = true;
        QVector<QPair<int, int> > dirty;
        CPUBufferRef() { dirty.reserve(64); }
    };

    struct PersistentFrameData {
        struct ChangeTrackedBuffer {
            ComPtr<ID3D12Resource> buffer;
            QVector<QPair<int, int> > totalDirtyInFrame;
            quint32 dataSize = 0;
        };
        ChangeTrackedBuffer vertex;
        ChangeTrackedBuffer index;
        ChangeTrackedBuffer constant;
        ComPtr<ID3D12DescriptorHeap> gpuCbvSrvUavHeap;
        int gpuCbvSrvUavHeapSize;
        int cbvSrvUavNextFreeDescriptorIndex;
        QSet<uint> pendingTextureUploads;
        QSet<uint> pendingTextureMipMap;
        QSet<uint> pendingTextureReleases;
        struct DeleteQueueEntry {
            ComPtr<ID3D12Resource> res;
            ComPtr<ID3D12DescriptorHeap> descHeap;
            SIZE_T cpuDescriptorPtr = 0;
        };
        QVector<DeleteQueueEntry> deleteQueue;
        void deferredDelete(ComPtr<ID3D12Resource> res) { DeleteQueueEntry e; e.res = res; deleteQueue << e; }
        void deferredDelete(ComPtr<ID3D12DescriptorHeap> dh) { DeleteQueueEntry e; e.descHeap = dh; deleteQueue << e; }
        void deferredDelete(D3D12_CPU_DESCRIPTOR_HANDLE h) { DeleteQueueEntry e; e.cpuDescriptorPtr = h.ptr; deleteQueue << e; }
    };

    void markCPUBufferDirty(CPUBufferRef *dst, PersistentFrameData::ChangeTrackedBuffer *buf, int offset, int size);
    void ensureBuffer(CPUBufferRef *src,  PersistentFrameData::ChangeTrackedBuffer *buf, const char *dbgstr);
    void updateBuffer(CPUBufferRef *src, PersistentFrameData::ChangeTrackedBuffer *buf, const char *dbgstr);

    void beginDrawCalls(bool needsBackbufferTransition = false);
    void endDrawCalls(bool needsBackbufferTransition = false);

    static const int SWAP_CHAIN_BUFFER_COUNT = 2;
    static const int MAX_FRAMES_IN_FLIGHT = 2;

    bool initialized = false;
    bool inFrame = false;
    QWindow *window = nullptr;
    ID3D12Device *device;
    ComPtr<ID3D12CommandQueue> commandQueue;
    ComPtr<ID3D12CommandQueue> copyCommandQueue;
    ComPtr<IDXGISwapChain3> swapChain;
    ComPtr<ID3D12Resource> renderTargets[SWAP_CHAIN_BUFFER_COUNT];
    D3D12_CPU_DESCRIPTOR_HANDLE rtv[SWAP_CHAIN_BUFFER_COUNT];
    D3D12_CPU_DESCRIPTOR_HANDLE dsv;
    ComPtr<ID3D12Resource> depthStencil;
    ComPtr<ID3D12CommandAllocator> commandAllocator[MAX_FRAMES_IN_FLIGHT];
    ComPtr<ID3D12CommandAllocator> copyCommandAllocator;
    ComPtr<ID3D12GraphicsCommandList> commandList;
    ComPtr<ID3D12GraphicsCommandList> copyCommandList;
    QSGD3D12CPUDescriptorHeapManager cpuDescHeapManager;
    quint64 presentFrameIndex;
    quint64 frameIndex;
    QSGD3D12CPUWaitableFence *presentFence = nullptr;
    QSGD3D12CPUWaitableFence *frameFence[MAX_FRAMES_IN_FLIGHT];

    CPUBufferRef vertexData;
    CPUBufferRef indexData;
    CPUBufferRef constantData;

    PersistentFrameData pframeData[MAX_FRAMES_IN_FLIGHT];
    int currentPFrameIndex;

    struct PSOCacheEntry {
        ComPtr<ID3D12PipelineState> pso;
    };
    QCache<QSGD3D12PipelineState, PSOCacheEntry> psoCache;
    struct RootSigCacheEntry {
        ComPtr<ID3D12RootSignature> rootSig;
    };
    QCache<QSGD3D12RootSignature, RootSigCacheEntry> rootSigCache;

    struct Texture {
        bool entryInUse = false;
        ComPtr<ID3D12Resource> texture;
        D3D12_CPU_DESCRIPTOR_HANDLE srv;
        quint64 fenceValue = 0;
        bool waitAdded = false;
        ComPtr<ID3D12Resource> stagingBuffer;
        QVector<D3D12_CPU_DESCRIPTOR_HANDLE> mipUAVs;
        bool mipmap = false;
    };

    QVector<Texture> textures;
    ComPtr<ID3D12Fence> textureUploadFence;
    QAtomicInt nextTextureUploadFenceValue;

    struct TransientFrameData {
        QSGGeometry::DrawingMode drawingMode;
        bool indexBufferSet;
        QVector<uint> activeTextures;
        int drawCount;
        ID3D12PipelineState *lastPso;
        ID3D12RootSignature *lastRootSig;
        bool descHeapSet;

        QRect viewport;
        QRect scissor;
        quint32 stencilRef;
        QSGD3D12PipelineState pipelineState;
    };
    TransientFrameData tframeData;

    struct MipMapGen {
        bool initialize(QSGD3D12EnginePrivate *enginePriv);
        void releaseResources();
        void queueGenerate(const Texture &t);

        QSGD3D12EnginePrivate *engine;
        ComPtr<ID3D12RootSignature> rootSig;
        ComPtr<ID3D12PipelineState> pipelineState;
    };

    MipMapGen mipmapper;
};

QT_END_NAMESPACE

#endif
