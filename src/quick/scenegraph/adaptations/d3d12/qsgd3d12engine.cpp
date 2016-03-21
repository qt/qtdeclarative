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

#include "qsgd3d12engine_p.h"
#include "qsgd3d12engine_p_p.h"
#include "cs_mipmapgen.hlslh"
#include <QString>
#include <QColor>
#include <qmath.h>
#include <QtCore/private/qsimd_p.h>

QT_BEGIN_NAMESPACE

// NOTE: Avoid categorized logging. It is slow.

#define DECLARE_DEBUG_VAR(variable) \
    static bool debug_ ## variable() \
    { static bool value = qgetenv("QSG_RENDERER_DEBUG").contains(QT_STRINGIFY(variable)); return value; }

DECLARE_DEBUG_VAR(render)

static const int MAX_DRAW_CALLS_PER_LIST = 128;

static const int MAX_CACHED_ROOTSIG = 16;
static const int MAX_CACHED_PSO = 64;

static const int GPU_CBVSRVUAV_DESCRIPTORS = 512;

static const int BUCKETS_PER_HEAP = 8; // must match freeMap
static const int DESCRIPTORS_PER_BUCKET = 32; // the bit map (freeMap) is quint32
static const int MAX_DESCRIPTORS_PER_HEAP = BUCKETS_PER_HEAP * DESCRIPTORS_PER_BUCKET;

D3D12_CPU_DESCRIPTOR_HANDLE QSGD3D12CPUDescriptorHeapManager::allocate(D3D12_DESCRIPTOR_HEAP_TYPE type)
{
    D3D12_CPU_DESCRIPTOR_HANDLE h = {};
    for (Heap &heap : m_heaps) {
        if (heap.type == type) {
            for (int bucket = 0; bucket < _countof(heap.freeMap); ++bucket)
                if (heap.freeMap[bucket]) {
                    unsigned long freePos = _bit_scan_forward(heap.freeMap[bucket]);
                    heap.freeMap[bucket] &= ~(1UL << freePos);
                    if (Q_UNLIKELY(debug_render()))
                        qDebug("descriptor handle type %x reserve in bucket %d index %d", type, bucket, freePos);
                    freePos += bucket * DESCRIPTORS_PER_BUCKET;
                    h = heap.start;
                    h.ptr += freePos * heap.handleSize;
                    return h;
                }
        }
    }

    Heap heap;
    heap.type = type;
    heap.handleSize = m_handleSizes[type];

    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.NumDescriptors = MAX_DESCRIPTORS_PER_HEAP;
    heapDesc.Type = type;
    // The heaps created here are _never_ shader-visible.

    HRESULT hr = m_device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&heap.heap));
    if (FAILED(hr)) {
        qWarning("Failed to create heap with type 0x%x: %x", type, hr);
        return h;
    }

    heap.start = heap.heap->GetCPUDescriptorHandleForHeapStart();

    if (Q_UNLIKELY(debug_render()))
        qDebug("new descriptor heap, type %x, start %llu", type, heap.start.ptr);

    heap.freeMap[0] = 0xFFFFFFFE;
    for (int i = 1; i < _countof(heap.freeMap); ++i)
        heap.freeMap[i] = 0xFFFFFFFF;

    h = heap.start;

    m_heaps.append(heap);

    return h;
}

void QSGD3D12CPUDescriptorHeapManager::release(D3D12_CPU_DESCRIPTOR_HANDLE handle, D3D12_DESCRIPTOR_HEAP_TYPE type)
{
    for (Heap &heap : m_heaps) {
        if (heap.type == type
                && handle.ptr >= heap.start.ptr
                && handle.ptr < heap.start.ptr + heap.handleSize * MAX_DESCRIPTORS_PER_HEAP) {
            unsigned long pos = (handle.ptr - heap.start.ptr) / heap.handleSize;
            const int bucket = pos / DESCRIPTORS_PER_BUCKET;
            const int indexInBucket = pos - bucket * DESCRIPTORS_PER_BUCKET;
            heap.freeMap[bucket] |= 1UL << indexInBucket;
            if (Q_UNLIKELY(debug_render()))
                qDebug("free descriptor handle type %x bucket %d index %d", type, bucket, indexInBucket);
            return;
        }
    }
    qWarning("QSGD3D12CPUDescriptorHeapManager: Attempted to release untracked descriptor handle %llu of type %d", handle.ptr, type);
}

void QSGD3D12CPUDescriptorHeapManager::initialize(ID3D12Device *device)
{
    m_device = device;

    for (int i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
        m_handleSizes[i] = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE(i));
}

void QSGD3D12CPUDescriptorHeapManager::releaseResources()
{
    for (Heap &heap : m_heaps)
        heap.heap = nullptr;

    m_heaps.clear();

    m_device = nullptr;
}

// One device per process, one everything else (engine) per window.
Q_GLOBAL_STATIC(QSGD3D12DeviceManager, deviceManager)

static void getHardwareAdapter(IDXGIFactory1 *factory, IDXGIAdapter1 **outAdapter)
{
    const D3D_FEATURE_LEVEL fl = D3D_FEATURE_LEVEL_11_0;
    ComPtr<IDXGIAdapter1> adapter;
    DXGI_ADAPTER_DESC1 desc;

    for (int adapterIndex = 0; factory->EnumAdapters1(adapterIndex, &adapter) != DXGI_ERROR_NOT_FOUND; ++adapterIndex) {
        DXGI_ADAPTER_DESC1 desc;
        adapter->GetDesc1(&desc);
        const QString name = QString::fromUtf16((char16_t *) desc.Description);
        qDebug("Adapter %d: '%s' (flags 0x%x)", adapterIndex, qPrintable(name), desc.Flags);
    }

    if (qEnvironmentVariableIsSet("QT_D3D_ADAPTER_INDEX")) {
        const int adapterIndex = qEnvironmentVariableIntValue("QT_D3D_ADAPTER_INDEX");
        if (SUCCEEDED(factory->EnumAdapters1(adapterIndex, &adapter))
                && SUCCEEDED(D3D12CreateDevice(adapter.Get(), fl, _uuidof(ID3D12Device), nullptr))) {
            adapter->GetDesc1(&desc);
            const QString name = QString::fromUtf16((char16_t *) desc.Description);
            qDebug("Using requested adapter '%s'", qPrintable(name));
            *outAdapter = adapter.Detach();
            return;
        }
    }

    for (int adapterIndex = 0; factory->EnumAdapters1(adapterIndex, &adapter) != DXGI_ERROR_NOT_FOUND; ++adapterIndex) {
        adapter->GetDesc1(&desc);
        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            continue;

        if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), fl, _uuidof(ID3D12Device), nullptr))) {
            const QString name = QString::fromUtf16((char16_t *) desc.Description);
            qDebug("Using adapter '%s'", qPrintable(name));
            break;
        }
    }

    *outAdapter = adapter.Detach();
}

ID3D12Device *QSGD3D12DeviceManager::ref()
{
    ensureCreated();
    m_ref.ref();
    return m_device.Get();
}

void QSGD3D12DeviceManager::unref()
{
    if (!m_ref.deref()) {
        if (Q_UNLIKELY(debug_render()))
            qDebug("destroying d3d device");
        m_device = nullptr;
        m_factory = nullptr;
    }
}

void QSGD3D12DeviceManager::deviceLossDetected()
{
    for (DeviceLossObserver *observer : qAsConst(m_observers))
        observer->deviceLost();

    // Nothing else to do here. All windows are expected to release their
    // resources and call unref() in response immediately.
}

IDXGIFactory4 *QSGD3D12DeviceManager::dxgi()
{
    ensureCreated();
    return m_factory.Get();
}

void QSGD3D12DeviceManager::ensureCreated()
{
    if (m_device)
        return;

    HRESULT hr = CreateDXGIFactory2(0, IID_PPV_ARGS(&m_factory));
    if (FAILED(hr)) {
        qWarning("Failed to create DXGI: 0x%x", hr);
        return;
    }

    ComPtr<IDXGIAdapter1> adapter;
    getHardwareAdapter(m_factory.Get(), &adapter);

    bool warp = true;
    if (adapter) {
        HRESULT hr = D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_device));
        if (SUCCEEDED(hr))
            warp = false;
        else
            qWarning("Failed to create device: 0x%x", hr);
    }

    if (warp) {
        qDebug("Using WARP");
        m_factory->EnumWarpAdapter(IID_PPV_ARGS(&adapter));
        HRESULT hr = D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_device));
        if (FAILED(hr)) {
            qWarning("Failed to create WARP device: 0x%x", hr);
            return;
        }
    }

    ComPtr<IDXGIAdapter3> adapter3;
    if (SUCCEEDED(adapter.As(&adapter3))) {
        DXGI_QUERY_VIDEO_MEMORY_INFO vidMemInfo;
        if (SUCCEEDED(adapter3->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &vidMemInfo))) {
            qDebug("Video memory info: LOCAL: Budget %llu KB CurrentUsage %llu KB AvailableForReservation %llu KB CurrentReservation %llu KB",
                   vidMemInfo.Budget / 1024, vidMemInfo.CurrentUsage / 1024,
                   vidMemInfo.AvailableForReservation / 1024, vidMemInfo.CurrentReservation / 1024);
        }
        if (SUCCEEDED(adapter3->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_NON_LOCAL, &vidMemInfo))) {
            qDebug("Video memory info: NON-LOCAL: Budget %llu KB CurrentUsage %llu KB AvailableForReservation %llu KB CurrentReservation %llu KB",
                   vidMemInfo.Budget / 1024, vidMemInfo.CurrentUsage / 1024,
                   vidMemInfo.AvailableForReservation / 1024, vidMemInfo.CurrentReservation / 1024);
        }
    }
}

void QSGD3D12DeviceManager::registerDeviceLossObserver(DeviceLossObserver *observer)
{
    if (!m_observers.contains(observer))
        m_observers.append(observer);
}

QSGD3D12Engine::QSGD3D12Engine()
{
    d = new QSGD3D12EnginePrivate;
}

QSGD3D12Engine::~QSGD3D12Engine()
{
    d->waitGPU();
    d->releaseResources();
    delete d;
}

bool QSGD3D12Engine::attachToWindow(QWindow *window)
{
    if (d->isInitialized()) {
        qWarning("QSGD3D12Engine: Cannot attach active engine to window");
        return false;
    }

    d->initialize(window);
    return d->isInitialized();
}

void QSGD3D12Engine::releaseResources()
{
    d->releaseResources();
}

void QSGD3D12Engine::resize()
{
    d->waitGPU();
    d->resize();
}

QWindow *QSGD3D12Engine::window() const
{
    return d->currentWindow();
}

void QSGD3D12Engine::beginFrame()
{
    d->beginFrame();
}

void QSGD3D12Engine::endFrame()
{
    d->endFrame();
}

void QSGD3D12Engine::finalizePipeline(const QSGD3D12PipelineState &pipelineState)
{
    d->finalizePipeline(pipelineState);
}

void QSGD3D12Engine::resetVertexBuffer(const quint8 *data, int size)
{
    d->resetVertexBuffer(data, size);
}

void QSGD3D12Engine::markVertexBufferDirty(int offset, int size)
{
    d->markVertexBufferDirty(offset, size);
}

void QSGD3D12Engine::resetIndexBuffer(const quint8 *data, int size)
{
    d->resetIndexBuffer(data, size);
}

void QSGD3D12Engine::markIndexBufferDirty(int offset, int size)
{
    d->markIndexBufferDirty(offset, size);
}

void QSGD3D12Engine::resetConstantBuffer(const quint8 *data, int size)
{
    d->resetConstantBuffer(data, size);
}

void QSGD3D12Engine::markConstantBufferDirty(int offset, int size)
{
    d->markConstantBufferDirty(offset, size);
}

void QSGD3D12Engine::queueViewport(const QRect &rect)
{
    d->queueViewport(rect);
}

void QSGD3D12Engine::queueScissor(const QRect &rect)
{
    d->queueScissor(rect);
}

void QSGD3D12Engine::queueSetRenderTarget(uint id)
{
    d->queueSetRenderTarget(id);
}

void QSGD3D12Engine::queueClearRenderTarget(const QColor &color)
{
    d->queueClearRenderTarget(color);
}

void QSGD3D12Engine::queueClearDepthStencil(float depthValue, quint8 stencilValue, ClearFlags which)
{
    d->queueClearDepthStencil(depthValue, stencilValue, which);
}

void QSGD3D12Engine::queueSetBlendFactor(const QVector4D &factor)
{
    d->queueSetBlendFactor(factor);
}

void QSGD3D12Engine::queueSetStencilRef(quint32 ref)
{
    d->queueSetStencilRef(ref);
}

void QSGD3D12Engine::queueDraw(QSGGeometry::DrawingMode mode, int count, int vboOffset, int vboSize, int vboStride,
                               int cboOffset,
                               int startIndexIndex, QSGD3D12Format indexFormat)
{
    d->queueDraw(mode, count, vboOffset, vboSize, vboStride, cboOffset, startIndexIndex, indexFormat);
}

void QSGD3D12Engine::present()
{
    d->present();
}

void QSGD3D12Engine::waitGPU()
{
    d->waitGPU();
}

uint QSGD3D12Engine::genTexture()
{
    return d->genTexture();
}

void QSGD3D12Engine::createTexture(uint id, const QSize &size, QImage::Format format, TextureCreateFlags flags)
{
    d->createTexture(id, size, format, flags);
}

void QSGD3D12Engine::queueTextureResize(uint id, const QSize &size)
{
    d->queueTextureResize(id, size);
}

void QSGD3D12Engine::queueTextureUpload(uint id, const QImage &image, const QPoint &dstPos)
{
    d->queueTextureUpload(id, QVector<QImage>() << image, QVector<QPoint>() << dstPos);
}

void QSGD3D12Engine::queueTextureUpload(uint id, const QVector<QImage> &images, const QVector<QPoint> &dstPos)
{
    d->queueTextureUpload(id, images, dstPos);
}

void QSGD3D12Engine::releaseTexture(uint id)
{
    d->releaseTexture(id);
}

SIZE_T QSGD3D12Engine::textureSRV(uint id) const
{
    return d->textureSRV(id);
}

void QSGD3D12Engine::activateTexture(uint id)
{
    d->activateTexture(id);
}

uint QSGD3D12Engine::genRenderTarget()
{
    return d->genRenderTarget();
}

void QSGD3D12Engine::createRenderTarget(uint id, const QSize &size, const QVector4D &clearColor, int samples)
{
    d->createRenderTarget(id, size, clearColor, samples);
}

void QSGD3D12Engine::releaseRenderTarget(uint id)
{
    d->releaseRenderTarget(id);
}

void QSGD3D12Engine::activateRenderTargetAsTexture(uint id)
{
    d->activateRenderTargetAsTexture(id);
}

static inline quint32 alignedSize(quint32 size, quint32 byteAlign)
{
    return (size + byteAlign - 1) & ~(byteAlign - 1);
}

quint32 QSGD3D12Engine::alignedConstantBufferSize(quint32 size)
{
    return alignedSize(size, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
}

QSGD3D12Format QSGD3D12Engine::toDXGIFormat(QSGGeometry::Type sgtype, int tupleSize, int *size)
{
    QSGD3D12Format format = FmtUnknown;

    static const QSGD3D12Format formatMap_ub[] = { FmtUnknown,
                                                   FmtUNormByte,
                                                   FmtUNormByte2,
                                                   FmtUnknown,
                                                   FmtUNormByte4 };

    static const QSGD3D12Format formatMap_f[] = { FmtUnknown,
                                                  FmtFloat,
                                                  FmtFloat2,
                                                  FmtFloat3,
                                                  FmtFloat4 };

    switch (sgtype) {
    case QSGGeometry::TypeUnsignedByte:
        format = formatMap_ub[tupleSize];
        if (size)
            *size = tupleSize;
        break;
    case QSGGeometry::TypeFloat:
        format = formatMap_f[tupleSize];
        if (size)
            *size = sizeof(float) * tupleSize;
        break;

    case QSGGeometry::TypeUnsignedShort:
        format = FmtUnsignedShort;
        if (size)
            *size = sizeof(ushort) * tupleSize;
        break;
    case QSGGeometry::TypeUnsignedInt:
        format = FmtUnsignedInt;
        if (size)
            *size = sizeof(uint) * tupleSize;
        break;

    case QSGGeometry::TypeByte:
    case QSGGeometry::TypeInt:
    case QSGGeometry::TypeShort:
        qWarning("no mapping for GL type 0x%x", sgtype);
        break;

    default:
        qWarning("unknown GL type 0x%x", sgtype);
        break;
    }

    return format;
}

int QSGD3D12Engine::mipMapLevels(const QSize &size)
{
    return ceil(log2(qMax(size.width(), size.height()))) + 1;
}

inline static bool isPowerOfTwo(int x)
{
    // Assumption: x >= 1
    return x == (x & -x);
}

QSize QSGD3D12Engine::mipMapAdjustedSourceSize(const QSize &size)
{
    if (size.isEmpty())
        return size;

    QSize adjustedSize = size;

    // ### for now only power-of-two sizes are mipmap-capable
    if (!isPowerOfTwo(size.width()))
        adjustedSize.setWidth(qNextPowerOfTwo(size.width()));
    if (!isPowerOfTwo(size.height()))
        adjustedSize.setHeight(qNextPowerOfTwo(size.height()));

    return adjustedSize;
}

void QSGD3D12EnginePrivate::releaseResources()
{
    if (!initialized)
        return;

    mipmapper.releaseResources();

    commandList = nullptr;
    copyCommandList = nullptr;

    copyCommandAllocator = nullptr;
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
        commandAllocator[i] = nullptr;

    depthStencil = nullptr;
    for (int i = 0; i < SWAP_CHAIN_BUFFER_COUNT; ++i)
        backBufferRT[i] = nullptr;

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        pframeData[i].vertex.buffer = nullptr;
        pframeData[i].index.buffer = nullptr;
        pframeData[i].constant.buffer = nullptr;
        pframeData[i].gpuCbvSrvUavHeap = nullptr;
    }

    psoCache.clear();
    rootSigCache.clear();
    textures.clear();
    renderTargets.clear();

    cpuDescHeapManager.releaseResources();

    commandQueue = nullptr;
    copyCommandQueue = nullptr;
    swapChain = nullptr;

    delete presentFence;
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
        delete frameFence[i];
    textureUploadFence = nullptr;

    deviceManager()->unref();

    initialized = false;

    // 'window' must be kept, may just be a device loss
}

void QSGD3D12EnginePrivate::initialize(QWindow *w)
{
    if (initialized)
        return;

    window = w;

    HWND hwnd = reinterpret_cast<HWND>(window->winId());

    if (qEnvironmentVariableIntValue("QT_D3D_DEBUG") != 0) {
        qDebug("Enabling debug layer");
        ComPtr<ID3D12Debug> debugController;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
            debugController->EnableDebugLayer();
    }

    QSGD3D12DeviceManager *dev = deviceManager();
    device = dev->ref();
    dev->registerDeviceLossObserver(this);

    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    if (FAILED(device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&commandQueue)))) {
        qWarning("Failed to create command queue");
        return;
    }

    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_COPY;
    if (FAILED(device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&copyCommandQueue)))) {
        qWarning("Failed to create copy command queue");
        return;
    }

    DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
    swapChainDesc.BufferCount = SWAP_CHAIN_BUFFER_COUNT;
    swapChainDesc.BufferDesc.Width = window->width() * window->devicePixelRatio();
    swapChainDesc.BufferDesc.Height = window->height() * window->devicePixelRatio();
    swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; // D3D12 requires the flip model
    swapChainDesc.OutputWindow = hwnd;
    swapChainDesc.SampleDesc.Count = 1; // Flip does not support MSAA so no choice here
    swapChainDesc.Windowed = TRUE;

    ComPtr<IDXGISwapChain> baseSwapChain;
    HRESULT hr = dev->dxgi()->CreateSwapChain(commandQueue.Get(), &swapChainDesc, &baseSwapChain);
    if (FAILED(hr)) {
        qWarning("Failed to create swap chain: 0x%x", hr);
        return;
    }
    if (FAILED(baseSwapChain.As(&swapChain))) {
        qWarning("Failed to cast swap chain");
        return;
    }

    dev->dxgi()->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER);

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        if (FAILED(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator[i])))) {
            qWarning("Failed to create command allocator");
            return;
        }
    }

    if (FAILED(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COPY, IID_PPV_ARGS(&copyCommandAllocator)))) {
        qWarning("Failed to create copy command allocator");
        return;
    }

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        if (!createCbvSrvUavHeap(i, GPU_CBVSRVUAV_DESCRIPTORS))
            return;
    }

    cpuDescHeapManager.initialize(device);

    setupRenderTargets();

    if (FAILED(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator[0].Get(),
                                         nullptr, IID_PPV_ARGS(&commandList)))) {
        qWarning("Failed to create command list");
        return;
    }
    // created in recording state, close it for now
    commandList->Close();

    if (FAILED(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_COPY, copyCommandAllocator.Get(),
                                         nullptr, IID_PPV_ARGS(&copyCommandList)))) {
        qWarning("Failed to create copy command list");
        return;
    }
    copyCommandList->Close();

    frameIndex = 0;

    presentFence = createCPUWaitableFence();
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
        frameFence[i] = createCPUWaitableFence();

    if (FAILED(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&textureUploadFence)))) {
        qWarning("Failed to create fence");
        return;
    }

    vertexData = CPUBufferRef();
    indexData = CPUBufferRef();
    constantData = CPUBufferRef();

    psoCache.setMaxCost(MAX_CACHED_PSO);
    rootSigCache.setMaxCost(MAX_CACHED_ROOTSIG);

    if (!mipmapper.initialize(this))
        return;

    currentRenderTarget = 0;

    initialized = true;
}

bool QSGD3D12EnginePrivate::createCbvSrvUavHeap(int pframeIndex, int descriptorCount)
{
    D3D12_DESCRIPTOR_HEAP_DESC gpuDescHeapDesc = {};
    gpuDescHeapDesc.NumDescriptors = descriptorCount;
    gpuDescHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    gpuDescHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

    if (FAILED(device->CreateDescriptorHeap(&gpuDescHeapDesc, IID_PPV_ARGS(&pframeData[pframeIndex].gpuCbvSrvUavHeap)))) {
        qWarning("Failed to create shader-visible CBV-SRV-UAV heap");
        return false;
    }

    pframeData[pframeIndex].gpuCbvSrvUavHeapSize = descriptorCount;

    return true;
}

DXGI_SAMPLE_DESC QSGD3D12EnginePrivate::makeSampleDesc(DXGI_FORMAT format, int samples)
{
    DXGI_SAMPLE_DESC sampleDesc;
    sampleDesc.Count = 1;
    sampleDesc.Quality = 0;

    if (samples > 1) {
        D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msaaInfo = {};
        msaaInfo.Format = format;
        msaaInfo.SampleCount = samples;
        if (SUCCEEDED(device->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &msaaInfo, sizeof(msaaInfo)))) {
            if (msaaInfo.NumQualityLevels > 0) {
                sampleDesc.Count = samples;
                sampleDesc.Quality = msaaInfo.NumQualityLevels - 1;
            } else {
                qWarning("No quality levels for multisampling?");
            }
        } else {
            qWarning("Failed to query multisample quality levels");
        }
    }

    return sampleDesc;
}

ID3D12Resource *QSGD3D12EnginePrivate::createColorBuffer(D3D12_CPU_DESCRIPTOR_HANDLE viewHandle, const QSize &size,
                                                         const QVector4D &clearColor, int samples)
{
    D3D12_CLEAR_VALUE clearValue = {};
    clearValue.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    clearValue.Color[0] = clearColor.x();
    clearValue.Color[1] = clearColor.y();
    clearValue.Color[2] = clearColor.z();
    clearValue.Color[3] = clearColor.w();

    D3D12_HEAP_PROPERTIES heapProp = {};
    heapProp.Type = D3D12_HEAP_TYPE_DEFAULT;

    D3D12_RESOURCE_DESC rtDesc = {};
    rtDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    rtDesc.Width = size.width();
    rtDesc.Height = size.height();
    rtDesc.DepthOrArraySize = 1;
    rtDesc.MipLevels = 1;
    rtDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    rtDesc.SampleDesc = makeSampleDesc(rtDesc.Format, samples);
    rtDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

    ID3D12Resource *resource = nullptr;
    if (FAILED(device->CreateCommittedResource(&heapProp, D3D12_HEAP_FLAG_NONE, &rtDesc,
                                               D3D12_RESOURCE_STATE_RENDER_TARGET, &clearValue, IID_PPV_ARGS(&resource)))) {
        qWarning("Failed to create offscreen render target of size %dx%d", size.width(), size.height());
        return nullptr;
    }

    device->CreateRenderTargetView(resource, nullptr, viewHandle);

    return resource;
}

ID3D12Resource *QSGD3D12EnginePrivate::createDepthStencil(D3D12_CPU_DESCRIPTOR_HANDLE viewHandle, const QSize &size, int samples)
{
    D3D12_CLEAR_VALUE depthClearValue = {};
    depthClearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthClearValue.DepthStencil.Depth = 1.0f;
    depthClearValue.DepthStencil.Stencil = 0;

    D3D12_HEAP_PROPERTIES heapProp = {};
    heapProp.Type = D3D12_HEAP_TYPE_DEFAULT;

    D3D12_RESOURCE_DESC bufDesc = {};
    bufDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    bufDesc.Width = size.width();
    bufDesc.Height = size.height();
    bufDesc.DepthOrArraySize = 1;
    bufDesc.MipLevels = 1;
    bufDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    bufDesc.SampleDesc = makeSampleDesc(bufDesc.Format, samples);
    bufDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    bufDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    ID3D12Resource *resource = nullptr;
    if (FAILED(device->CreateCommittedResource(&heapProp, D3D12_HEAP_FLAG_NONE, &bufDesc,
                                               D3D12_RESOURCE_STATE_DEPTH_WRITE, &depthClearValue, IID_PPV_ARGS(&resource)))) {
        qWarning("Failed to create depth-stencil buffer of size %dx%d", size.width(), size.height());
        return nullptr;
    }

    D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilDesc = {};
    depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthStencilDesc.ViewDimension = bufDesc.SampleDesc.Count <= 1 ? D3D12_DSV_DIMENSION_TEXTURE2D : D3D12_DSV_DIMENSION_TEXTURE2DMS;

    device->CreateDepthStencilView(resource, &depthStencilDesc, viewHandle);

    return resource;
}

void QSGD3D12EnginePrivate::setupRenderTargets()
{
    // ### multisampling needs an offscreen rt and explicit resolve, add this at some point

    for (int i = 0; i < SWAP_CHAIN_BUFFER_COUNT; ++i) {
        if (FAILED(swapChain->GetBuffer(i, IID_PPV_ARGS(&backBufferRT[i])))) {
            qWarning("Failed to get buffer %d from swap chain", i);
            return;
        }
        backBufferRTV[i] = cpuDescHeapManager.allocate(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        device->CreateRenderTargetView(backBufferRT[i].Get(), nullptr, backBufferRTV[i]);
    }

    backBufferDSV = cpuDescHeapManager.allocate(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    ID3D12Resource *ds = createDepthStencil(backBufferDSV, window->size(), 0);
    if (ds)
        depthStencil.Attach(ds);

    presentFrameIndex = 0;
}

void QSGD3D12EnginePrivate::resize()
{
    if (!initialized)
        return;

    if (Q_UNLIKELY(debug_render()))
        qDebug() << window->size();

    // Clear these, otherwise resizing will fail.
    depthStencil = nullptr;
    cpuDescHeapManager.release(backBufferDSV, D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    for (int i = 0; i < SWAP_CHAIN_BUFFER_COUNT; ++i) {
        backBufferRT[i] = nullptr;
        cpuDescHeapManager.release(backBufferRTV[i], D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    }

    HRESULT hr = swapChain->ResizeBuffers(SWAP_CHAIN_BUFFER_COUNT, window->width(), window->height(), DXGI_FORMAT_R8G8B8A8_UNORM, 0);
    if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET) {
        deviceManager()->deviceLossDetected();
        return;
    } else if (FAILED(hr)) {
        qWarning("Failed to resize buffers: 0x%x", hr);
        return;
    }

    setupRenderTargets();
}

void QSGD3D12EnginePrivate::deviceLost()
{
    qWarning("D3D device lost, will attempt to reinitialize");

    // Release all resources. This is important because otherwise reinitialization may fail.
    releaseResources();

    // Now in uninitialized state (but 'window' is still valid). Will recreate
    // all the resources on the next beginFrame().
}

QSGD3D12CPUWaitableFence *QSGD3D12EnginePrivate::createCPUWaitableFence() const
{
    QSGD3D12CPUWaitableFence *f = new QSGD3D12CPUWaitableFence;
    HRESULT hr = device->CreateFence(f->value, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&f->fence));
    if (FAILED(hr)) {
        qWarning("Failed to create fence: 0x%x", hr);
        return f;
    }
    f->event = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    return f;
}

void QSGD3D12EnginePrivate::waitForGPU(QSGD3D12CPUWaitableFence *f) const
{
    const UINT64 newValue = f->value.fetchAndAddAcquire(1) + 1;
    commandQueue->Signal(f->fence.Get(), newValue);
    if (f->fence->GetCompletedValue() < newValue) {
        HRESULT hr = f->fence->SetEventOnCompletion(newValue, f->event);
        if (FAILED(hr)) {
            qWarning("SetEventOnCompletion failed: 0x%x", hr);
            return;
        }
        WaitForSingleObject(f->event, INFINITE);
    }
}

void QSGD3D12EnginePrivate::transitionResource(ID3D12Resource *resource, ID3D12GraphicsCommandList *commandList,
                                               D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after) const
{
    D3D12_RESOURCE_BARRIER barrier;
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = resource;
    barrier.Transition.StateBefore = before;
    barrier.Transition.StateAfter = after;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

    commandList->ResourceBarrier(1, &barrier);
}

void QSGD3D12EnginePrivate::uavBarrier(ID3D12Resource *resource, ID3D12GraphicsCommandList *commandList) const
{
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    barrier.UAV.pResource = resource;

    commandList->ResourceBarrier(1, &barrier);
}

ID3D12Resource *QSGD3D12EnginePrivate::createBuffer(int size)
{
    ID3D12Resource *buf;

    D3D12_HEAP_PROPERTIES uploadHeapProp = {};
    uploadHeapProp.Type = D3D12_HEAP_TYPE_UPLOAD;

    D3D12_RESOURCE_DESC bufDesc = {};
    bufDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    bufDesc.Width = size;
    bufDesc.Height = 1;
    bufDesc.DepthOrArraySize = 1;
    bufDesc.MipLevels = 1;
    bufDesc.Format = DXGI_FORMAT_UNKNOWN;
    bufDesc.SampleDesc.Count = 1;
    bufDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    HRESULT hr = device->CreateCommittedResource(&uploadHeapProp, D3D12_HEAP_FLAG_NONE, &bufDesc,
                                                 D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&buf));
    if (FAILED(hr))
        qWarning("Failed to create buffer resource: 0x%x", hr);

    return buf;
}

ID3D12Resource *QSGD3D12EnginePrivate::currentBackBufferRT() const
{
    return backBufferRT[presentFrameIndex % SWAP_CHAIN_BUFFER_COUNT].Get();
}

D3D12_CPU_DESCRIPTOR_HANDLE QSGD3D12EnginePrivate::currentBackBufferRTV() const
{
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = backBufferRTV[0];
    rtvHandle.ptr += (presentFrameIndex % SWAP_CHAIN_BUFFER_COUNT) * cpuDescHeapManager.handleSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    return rtvHandle;
}

void QSGD3D12EnginePrivate::markCPUBufferDirty(CPUBufferRef *dst, PersistentFrameData::ChangeTrackedBuffer *buf, int offset, int size)
{
    if (!inFrame) {
        qWarning("%s: Cannot be called outside begin/endFrame", __FUNCTION__);
        return;
    }

    // Bail out when there was a resetV/I/CBuffer and the dirty list already spans the entire buffer data.
    if (!dst->dirty.isEmpty()) {
        if (dst->dirty[0].first == 0 && dst->dirty[0].second == dst->size)
            return;
    }

    const QPair<int, int> range = qMakePair(offset, size);
    if (!dst->dirty.contains(range)) {
        dst->dirty.append(range);
        buf->totalDirtyInFrame.append(range);
    }
}

void QSGD3D12EnginePrivate::ensureBuffer(CPUBufferRef *src, PersistentFrameData::ChangeTrackedBuffer *buf, const char *dbgstr)
{
    if (src->allDirty) {
        src->allDirty = false;
        // Only enlarge, never shrink
        const bool newBufferNeeded = buf->buffer ? (src->size > buf->buffer->GetDesc().Width) : true;
        if (newBufferNeeded) {
            // Round it up and overallocate a little bit so that a subsequent
            // buffer contents rebuild with a slightly larger total size does
            // not lead to creating a new buffer.
            quint32 sz = alignedSize(src->size, 4096);
            if (Q_UNLIKELY(debug_render()))
                qDebug("new %s buffer of size %d (actual data size %d)", dbgstr, sz, src->size);
            buf->buffer.Attach(createBuffer(sz));
        }
        // Cache the actual data size in the per-in-flight-frame data as well.
        buf->dataSize = src->size;
        // Mark everything as dirty.
        src->dirty.clear();
        buf->totalDirtyInFrame.clear();
        if (buf->buffer) {
            const QPair<int, int> range = qMakePair(0, src->size);
            src->dirty.append(range);
            buf->totalDirtyInFrame.append(range);
        }
    }
}

void QSGD3D12EnginePrivate::updateBuffer(CPUBufferRef *src, PersistentFrameData::ChangeTrackedBuffer *buf, const char *dbgstr)
{
    quint8 *p = nullptr;
    const D3D12_RANGE readRange = { 0, 0 };
    if (!src->dirty.isEmpty()) {
        if (FAILED(buf->buffer->Map(0, &readRange, reinterpret_cast<void **>(&p)))) {
            qWarning("Map failed for %s buffer of size %d", dbgstr, src->size);
            return;
        }
        for (const auto &r : qAsConst(src->dirty)) {
            if (Q_UNLIKELY(debug_render()))
                qDebug("%s o %d s %d", dbgstr, r.first, r.second);
            memcpy(p + r.first, src->p + r.first, r.second);
        }
        buf->buffer->Unmap(0, nullptr);
        src->dirty.clear();
    }
}

void QSGD3D12EnginePrivate::beginFrame()
{
    if (inFrame)
        qWarning("beginFrame called again without an endFrame");

    inFrame = true;

    if (Q_UNLIKELY(debug_render()))
        qDebug() << "***** begin frame, logical" << frameIndex << "present" << presentFrameIndex;

    // The device may have been lost. This is the point to attempt to start again from scratch.
    if (!initialized && window)
        initialize(window);

    // Block if needed. With 2 frames in flight frame N waits for frame N - 2, but not N - 1, to finish.
    currentPFrameIndex = frameIndex % MAX_FRAMES_IN_FLIGHT;
    if (frameIndex >= MAX_FRAMES_IN_FLIGHT) {
        frameFence[currentPFrameIndex]->fence->SetEventOnCompletion(frameIndex - MAX_FRAMES_IN_FLIGHT,
                                                                    frameFence[currentPFrameIndex]->event);
        WaitForSingleObject(frameFence[currentPFrameIndex]->event, INFINITE);
        commandAllocator[currentPFrameIndex]->Reset();
    }

    PersistentFrameData &pfd(pframeData[currentPFrameIndex]);
    pfd.cbvSrvUavNextFreeDescriptorIndex = 0;

    pfd.vertex.totalDirtyInFrame.clear();
    pfd.index.totalDirtyInFrame.clear();
    pfd.constant.totalDirtyInFrame.clear();

    if (frameIndex >= MAX_FRAMES_IN_FLIGHT) {
        // Now sync the buffer changes from the previous, potentially still in flight, frames.
        for (int delta = 1; delta < MAX_FRAMES_IN_FLIGHT; ++delta) {
            PersistentFrameData &prevFrameData(pframeData[(frameIndex - delta) % MAX_FRAMES_IN_FLIGHT]);
            if (pfd.vertex.buffer && pfd.vertex.dataSize == vertexData.size) {
                vertexData.dirty = prevFrameData.vertex.totalDirtyInFrame;
            } else {
                vertexData.dirty.clear();
                vertexData.allDirty = true;
            }
            if (pfd.index.buffer && pfd.index.dataSize == indexData.size) {
                indexData.dirty = prevFrameData.index.totalDirtyInFrame;
            } else {
                indexData.dirty.clear();
                indexData.allDirty = true;
            }
            if (pfd.constant.buffer && pfd.constant.dataSize == constantData.size) {
                constantData.dirty = prevFrameData.constant.totalDirtyInFrame;
            } else {
                constantData.dirty.clear();
                constantData.allDirty = true;
            }
        }

        // Do some texture upload bookkeeping.
        const quint64 finishedFrameIndex = frameIndex - MAX_FRAMES_IN_FLIGHT; // we know since we just blocked for this
        // pfd conveniently refers to the same slot that was used by that frame
        if (!pfd.pendingTextureUploads.isEmpty()) {
            if (Q_UNLIKELY(debug_render()))
                qDebug("Removing texture upload data for frame %d", finishedFrameIndex);
            for (uint id : qAsConst(pfd.pendingTextureUploads)) {
                const int idx = id - 1;
                Texture &t(textures[idx]);
                // fenceValue is 0 when the previous frame cleared it, skip in
                // this case. Skip also when fenceValue > the value it was when
                // adding the last GPU wait - this is the case when more
                // uploads were queued for the same texture in the meantime.
                if (t.fenceValue && t.fenceValue == t.lastWaitFenceValue) {
                    t.fenceValue = 0;
                    t.lastWaitFenceValue = 0;
                    t.stagingBuffers.clear();
                    t.stagingHeaps.clear();
                    if (Q_UNLIKELY(debug_render()))
                        qDebug("Cleaned staging data for texture %u", id);
                }
            }
            pfd.pendingTextureUploads.clear();
            if (!pfd.pendingTextureMipMap.isEmpty()) {
                if (Q_UNLIKELY(debug_render()))
                    qDebug() << "cleaning mipmap generation data for " << pfd.pendingTextureMipMap;
                // no special cleanup is needed as mipmap generation uses the frame's resources
                pfd.pendingTextureMipMap.clear();
            }
            bool hasPending = false;
            for (int delta = 1; delta < MAX_FRAMES_IN_FLIGHT; ++delta) {
                const PersistentFrameData &prevFrameData(pframeData[(frameIndex - delta) % MAX_FRAMES_IN_FLIGHT]);
                if (!prevFrameData.pendingTextureUploads.isEmpty()) {
                    hasPending = true;
                    break;
                }
            }
            if (!hasPending) {
                if (Q_UNLIKELY(debug_render()))
                    qDebug("no more pending textures");
                copyCommandAllocator->Reset();
            }
        }

        // Do the deferred deletes.
        if (!pfd.deleteQueue.isEmpty()) {
            for (PersistentFrameData::DeleteQueueEntry &e : pfd.deleteQueue) {
                e.res = nullptr;
                e.descHeap = nullptr;
                if (e.cpuDescriptorPtr) {
                    D3D12_CPU_DESCRIPTOR_HANDLE h = { e.cpuDescriptorPtr };
                    cpuDescHeapManager.release(h, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
                }
            }
            pfd.deleteQueue.clear();
        }
        // Deferred deletes issued outside a begin-endFrame go to the next
        // frame's out-of-frame delete queue as these cannot be executed in the
        // next beginFrame, only in next + MAX_FRAMES_IN_FLIGHT. Move to the
        // normal queue if this is the next beginFrame.
        if (!pfd.outOfFrameDeleteQueue.isEmpty()) {
            pfd.deleteQueue = pfd.outOfFrameDeleteQueue;
            pfd.outOfFrameDeleteQueue.clear();
        }

        // Mark released texture slots free.
        if (!pfd.pendingTextureReleases.isEmpty()) {
            for (uint id : qAsConst(pfd.pendingTextureReleases)) {
                Texture &t(textures[id - 1]);
                t.flags &= ~RenderTarget::EntryInUse; // createTexture() can now reuse this entry
                t.texture = nullptr;
            }
            pfd.pendingTextureReleases.clear();
        }
        if (!pfd.outOfFramePendingTextureReleases.isEmpty()) {
            pfd.pendingTextureReleases = pfd.outOfFramePendingTextureReleases;
            pfd.outOfFramePendingTextureReleases.clear();
        }
    }

    beginDrawCalls(true);
}

void QSGD3D12EnginePrivate::beginDrawCalls(bool needsBackbufferTransition)
{
    commandList->Reset(commandAllocator[frameIndex % MAX_FRAMES_IN_FLIGHT].Get(), nullptr);

    tframeData.drawingMode = QSGGeometry::DrawingMode(-1);
    tframeData.indexBufferSet = false;
    tframeData.drawCount = 0;
    tframeData.lastPso = nullptr;
    tframeData.lastRootSig = nullptr;
    tframeData.descHeapSet = false;

    if (needsBackbufferTransition)
        transitionResource(currentBackBufferRT(), commandList.Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
}

void QSGD3D12EnginePrivate::endFrame()
{
    if (Q_UNLIKELY(debug_render()))
        qDebug() << "***** end frame";

    endDrawCalls(true);

    commandQueue->Signal(frameFence[frameIndex % MAX_FRAMES_IN_FLIGHT]->fence.Get(), frameIndex + 1);
    ++frameIndex;

    inFrame = false;
}

void QSGD3D12EnginePrivate::endDrawCalls(bool needsBackbufferTransition)
{
    PersistentFrameData &pfd(pframeData[currentPFrameIndex]);

    // Now is the time to sync all the changed areas in the buffers.
    updateBuffer(&vertexData, &pfd.vertex, "vertex");
    updateBuffer(&indexData, &pfd.index, "index");
    updateBuffer(&constantData, &pfd.constant, "constant");

    // Add a wait on the 3D queue for the relevant texture uploads on the copy queue.
    if (!pfd.pendingTextureUploads.isEmpty()) {
        quint64 topFenceValue = 0;
        for (uint id : qAsConst(pfd.pendingTextureUploads)) {
            const int idx = id - 1;
            Texture &t(textures[idx]);
            Q_ASSERT(t.fenceValue);
            // skip if already added a Wait in the previous frame
            if (t.lastWaitFenceValue == t.fenceValue)
                continue;
            t.lastWaitFenceValue = t.fenceValue;
            if (t.fenceValue > topFenceValue)
                topFenceValue = t.fenceValue;
            if (t.mipmap())
                pfd.pendingTextureMipMap.insert(id);
        }
        if (topFenceValue) {
            if (Q_UNLIKELY(debug_render()))
                qDebug("added wait for texture fence %llu", topFenceValue);
            commandQueue->Wait(textureUploadFence.Get(), topFenceValue);
            // Generate mipmaps after the wait, when necessary.
            if (!pfd.pendingTextureMipMap.isEmpty()) {
                if (Q_UNLIKELY(debug_render()))
                    qDebug() << "starting mipmap generation for" << pfd.pendingTextureMipMap;
                for (uint id : qAsConst(pfd.pendingTextureMipMap))
                    mipmapper.queueGenerate(textures[id - 1]);
            }
        }
    }

    // Transition the backbuffer for present, if needed.
    if (needsBackbufferTransition)
        transitionResource(currentBackBufferRT(), commandList.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

    // Go!
    HRESULT hr = commandList->Close();
    if (FAILED(hr)) {
        qWarning("Failed to close command list: 0x%x", hr);
        if (hr == E_INVALIDARG)
            qWarning("Invalid arguments. Some of the commands in the list is invalid in some way.");
    }

    ID3D12CommandList *commandLists[] = { commandList.Get() };
    commandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);
}

// Root signature:
// [0] CBV - always present
// [1] table with 1 SRV per texture (optional)
// one static sampler per texture (optional)
//
// SRVs can be created freely via QSGD3D12CPUDescriptorHeapManager and stored
// in QSGD3D12TextureView. The engine will copy them onto a dedicated,
// shader-visible CBV-SRV-UAV heap in the correct order.

void QSGD3D12EnginePrivate::finalizePipeline(const QSGD3D12PipelineState &pipelineState)
{
    if (!inFrame) {
        qWarning("%s: Cannot be called outside begin/endFrame", __FUNCTION__);
        return;
    }

    tframeData.pipelineState = pipelineState;

    RootSigCacheEntry *cachedRootSig = rootSigCache[pipelineState.shaders.rootSig];
    if (!cachedRootSig) {
        if (Q_UNLIKELY(debug_render()))
            qDebug("NEW ROOTSIG");

        cachedRootSig = new RootSigCacheEntry;

        D3D12_ROOT_PARAMETER rootParams[4];
        int rootParamCount = 0;

        rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        rootParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        rootParams[0].Descriptor.ShaderRegister = 0; // b0
        rootParams[0].Descriptor.RegisterSpace = 0;
        ++rootParamCount;

        if (!pipelineState.shaders.rootSig.textureViews.isEmpty()) {
            rootParams[rootParamCount].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
            rootParams[rootParamCount].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
            rootParams[rootParamCount].DescriptorTable.NumDescriptorRanges = 1;
            D3D12_DESCRIPTOR_RANGE descRange;
            descRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
            descRange.NumDescriptors = pipelineState.shaders.rootSig.textureViews.count();
            descRange.BaseShaderRegister = 0; // t0, t1, ...
            descRange.RegisterSpace = 0;
            descRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
            rootParams[rootParamCount].DescriptorTable.pDescriptorRanges = &descRange;
            ++rootParamCount;
        }

        Q_ASSERT(rootParamCount <= _countof(rootParams));
        D3D12_ROOT_SIGNATURE_DESC desc;
        desc.NumParameters = rootParamCount;
        desc.pParameters = rootParams;
        desc.NumStaticSamplers = pipelineState.shaders.rootSig.textureViews.count();
        D3D12_STATIC_SAMPLER_DESC staticSamplers[8];
        int sdIdx = 0;
        Q_ASSERT(pipelineState.shaders.rootSig.textureViews.count() <= _countof(staticSamplers));
        for (const QSGD3D12TextureView &tv : qAsConst(pipelineState.shaders.rootSig.textureViews)) {
            D3D12_STATIC_SAMPLER_DESC sd = {};
            sd.Filter = D3D12_FILTER(tv.filter);
            sd.AddressU = D3D12_TEXTURE_ADDRESS_MODE(tv.addressModeHoriz);
            sd.AddressV = D3D12_TEXTURE_ADDRESS_MODE(tv.addressModeVert);
            sd.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
            sd.MinLOD = 0.0f;
            sd.MaxLOD = D3D12_FLOAT32_MAX;
            sd.ShaderRegister = sdIdx; // t0, t1, ...
            sd.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
            staticSamplers[sdIdx++] = sd;
        }
        desc.pStaticSamplers = staticSamplers;
        desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

        ComPtr<ID3DBlob> signature;
        ComPtr<ID3DBlob> error;
        if (FAILED(D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error))) {
            qWarning("Failed to serialize root signature");
            return;
        }
        if (FAILED(device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(),
                                               IID_PPV_ARGS(&cachedRootSig->rootSig)))) {
            qWarning("Failed to create root signature");
            return;
        }

        rootSigCache.insert(pipelineState.shaders.rootSig, cachedRootSig);
    }

    PSOCacheEntry *cachedPso = psoCache[pipelineState];
    if (!cachedPso) {
        if (Q_UNLIKELY(debug_render()))
            qDebug("NEW PSO");

        cachedPso = new PSOCacheEntry;

        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};

        D3D12_INPUT_ELEMENT_DESC inputElements[8];
        Q_ASSERT(pipelineState.inputElements.count() <= _countof(inputElements));
        int ieIdx = 0;
        for (const QSGD3D12InputElement &ie : pipelineState.inputElements) {
            D3D12_INPUT_ELEMENT_DESC ieDesc = {};
            ieDesc.SemanticName = ie.semanticName;
            ieDesc.SemanticIndex = ie.semanticIndex;
            ieDesc.Format = DXGI_FORMAT(ie.format);
            ieDesc.InputSlot = ie.slot;
            ieDesc.AlignedByteOffset = ie.offset;
            ieDesc.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
            if (Q_UNLIKELY(debug_render()))
                qDebug("input [%d]: %s %d 0x%x %d", ieIdx, ie.semanticName, ie.offset, ie.format, ie.slot);
            inputElements[ieIdx++] = ieDesc;
        }

        psoDesc.InputLayout = { inputElements, UINT(ieIdx) };

        psoDesc.pRootSignature = cachedRootSig->rootSig.Get();

        D3D12_SHADER_BYTECODE vshader;
        vshader.pShaderBytecode = pipelineState.shaders.vs;
        vshader.BytecodeLength = pipelineState.shaders.vsSize;
        D3D12_SHADER_BYTECODE pshader;
        pshader.pShaderBytecode = pipelineState.shaders.ps;
        pshader.BytecodeLength = pipelineState.shaders.psSize;

        psoDesc.VS = vshader;
        psoDesc.PS = pshader;

        D3D12_RASTERIZER_DESC rastDesc = {};
        rastDesc.FillMode = D3D12_FILL_MODE_SOLID;
        rastDesc.CullMode = D3D12_CULL_MODE(pipelineState.cullMode);
        rastDesc.FrontCounterClockwise = pipelineState.frontCCW;
        rastDesc.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
        rastDesc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
        rastDesc.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
        rastDesc.DepthClipEnable = TRUE;

        psoDesc.RasterizerState = rastDesc;

        D3D12_BLEND_DESC blendDesc = {};
        if (pipelineState.blend == QSGD3D12PipelineState::BlendNone) {
            D3D12_RENDER_TARGET_BLEND_DESC noBlendDesc = {};
            noBlendDesc.RenderTargetWriteMask = pipelineState.colorWrite ? D3D12_COLOR_WRITE_ENABLE_ALL : 0;
            blendDesc.RenderTarget[0] = noBlendDesc;
        } else if (pipelineState.blend == QSGD3D12PipelineState::BlendPremul) {
            const D3D12_RENDER_TARGET_BLEND_DESC premulBlendDesc = {
                TRUE, FALSE,
                D3D12_BLEND_ONE, D3D12_BLEND_INV_SRC_ALPHA, D3D12_BLEND_OP_ADD,
                D3D12_BLEND_ONE, D3D12_BLEND_INV_SRC_ALPHA, D3D12_BLEND_OP_ADD,
                D3D12_LOGIC_OP_NOOP,
                UINT8(pipelineState.colorWrite ? D3D12_COLOR_WRITE_ENABLE_ALL : 0)
            };
            blendDesc.RenderTarget[0] = premulBlendDesc;
        } else if (pipelineState.blend == QSGD3D12PipelineState::BlendColor) {
            const D3D12_RENDER_TARGET_BLEND_DESC colorBlendDesc = {
                TRUE, FALSE,
                D3D12_BLEND_BLEND_FACTOR, D3D12_BLEND_INV_SRC_COLOR, D3D12_BLEND_OP_ADD,
                D3D12_BLEND_BLEND_FACTOR, D3D12_BLEND_INV_SRC_ALPHA, D3D12_BLEND_OP_ADD,
                D3D12_LOGIC_OP_NOOP,
                UINT8(pipelineState.colorWrite ? D3D12_COLOR_WRITE_ENABLE_ALL : 0)
            };
            blendDesc.RenderTarget[0] = colorBlendDesc;
        }
        psoDesc.BlendState = blendDesc;

        psoDesc.DepthStencilState.DepthEnable = pipelineState.depthEnable;
        psoDesc.DepthStencilState.DepthWriteMask = pipelineState.depthWrite ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
        psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC(pipelineState.depthFunc);

        psoDesc.DepthStencilState.StencilEnable = pipelineState.stencilEnable;
        psoDesc.DepthStencilState.StencilReadMask = psoDesc.DepthStencilState.StencilWriteMask = 0xFF;
        D3D12_DEPTH_STENCILOP_DESC stencilOpDesc = {
            D3D12_STENCIL_OP(pipelineState.stencilFailOp),
            D3D12_STENCIL_OP(pipelineState.stencilDepthFailOp),
            D3D12_STENCIL_OP(pipelineState.stencilPassOp),
            D3D12_COMPARISON_FUNC(pipelineState.stencilFunc)
        };
        psoDesc.DepthStencilState.FrontFace = psoDesc.DepthStencilState.BackFace = stencilOpDesc;

        psoDesc.SampleMask = UINT_MAX;
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE(pipelineState.topologyType);
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
        psoDesc.SampleDesc.Count = 1;

        HRESULT hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&cachedPso->pso));
        if (FAILED(hr)) {
            qWarning("Failed to create graphics pipeline state");
            return;
        }

        psoCache.insert(pipelineState, cachedPso);
    }

    if (cachedPso->pso.Get() != tframeData.lastPso) {
        tframeData.lastPso = cachedPso->pso.Get();
        commandList->SetPipelineState(tframeData.lastPso);
    }

    if (cachedRootSig->rootSig.Get() != tframeData.lastRootSig) {
        tframeData.lastRootSig = cachedRootSig->rootSig.Get();
        commandList->SetGraphicsRootSignature(tframeData.lastRootSig);
    }

    if (!pipelineState.shaders.rootSig.textureViews.isEmpty())
        setDescriptorHeaps();
}

void QSGD3D12EnginePrivate::setDescriptorHeaps(bool force)
{
    if (force || !tframeData.descHeapSet) {
        tframeData.descHeapSet = true;
        ID3D12DescriptorHeap *heaps[] = { pframeData[currentPFrameIndex].gpuCbvSrvUavHeap.Get() };
        commandList->SetDescriptorHeaps(_countof(heaps), heaps);
    }
}

void QSGD3D12EnginePrivate::resetVertexBuffer(const quint8 *data, int size)
{
    vertexData.p = data;
    vertexData.size = size;
    vertexData.allDirty = true;
}

void QSGD3D12EnginePrivate::markVertexBufferDirty(int offset, int size)
{
    markCPUBufferDirty(&vertexData, &pframeData[currentPFrameIndex].vertex, offset, size);
}

void QSGD3D12EnginePrivate::resetIndexBuffer(const quint8 *data, int size)
{
    indexData.p = data;
    indexData.size = size;
    indexData.allDirty = true;
}

void QSGD3D12EnginePrivate::markIndexBufferDirty(int offset, int size)
{
    markCPUBufferDirty(&indexData, &pframeData[currentPFrameIndex].index, offset, size);
}

void QSGD3D12EnginePrivate::resetConstantBuffer(const quint8 *data, int size)
{
    constantData.p = data;
    constantData.size = size;
    constantData.allDirty = true;
}

void QSGD3D12EnginePrivate::markConstantBufferDirty(int offset, int size)
{
    markCPUBufferDirty(&constantData, &pframeData[currentPFrameIndex].constant, offset, size);
}

void QSGD3D12EnginePrivate::queueViewport(const QRect &rect)
{
    if (!inFrame) {
        qWarning("%s: Cannot be called outside begin/endFrame", __FUNCTION__);
        return;
    }

    tframeData.viewport = rect;
    const D3D12_VIEWPORT viewport = { float(rect.x()), float(rect.y()), float(rect.width()), float(rect.height()), 0, 1 };
    commandList->RSSetViewports(1, &viewport);
}

void QSGD3D12EnginePrivate::queueScissor(const QRect &rect)
{
    if (!inFrame) {
        qWarning("%s: Cannot be called outside begin/endFrame", __FUNCTION__);
        return;
    }

    tframeData.scissor = rect;
    const D3D12_RECT scissorRect = { rect.left(), rect.top(), rect.right(), rect.bottom() };
    commandList->RSSetScissorRects(1, &scissorRect);
}

void QSGD3D12EnginePrivate::queueSetRenderTarget(uint id)
{
    if (!inFrame) {
        qWarning("%s: Cannot be called outside begin/endFrame", __FUNCTION__);
        return;
    }

    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle;
    D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle;

    if (!id) {
        rtvHandle = currentBackBufferRTV();
        dsvHandle = backBufferDSV;
    } else {
        const int idx = id - 1;
        Q_ASSERT(idx < renderTargets.count());
        RenderTarget &rt(renderTargets[idx]);
        rtvHandle = rt.rtv;
        dsvHandle = rt.dsv;
        rt.flags |= RenderTarget::NeedsReadBarrier;
    }

    commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

    currentRenderTarget = id;
}

void QSGD3D12EnginePrivate::queueClearRenderTarget(const QColor &color)
{
    if (!inFrame) {
        qWarning("%s: Cannot be called outside begin/endFrame", __FUNCTION__);
        return;
    }

    const float clearColor[] = { float(color.redF()), float(color.blueF()), float(color.greenF()), float(color.alphaF()) };
    commandList->ClearRenderTargetView(currentBackBufferRTV(), clearColor, 0, nullptr);
}

void QSGD3D12EnginePrivate::queueClearDepthStencil(float depthValue, quint8 stencilValue, QSGD3D12Engine::ClearFlags which)
{
    if (!inFrame) {
        qWarning("%s: Cannot be called outside begin/endFrame", __FUNCTION__);
        return;
    }

    commandList->ClearDepthStencilView(backBufferDSV, D3D12_CLEAR_FLAGS(int(which)), depthValue, stencilValue, 0, nullptr);
}

void QSGD3D12EnginePrivate::queueSetBlendFactor(const QVector4D &factor)
{
    if (!inFrame) {
        qWarning("%s: Cannot be called outside begin/endFrame", __FUNCTION__);
        return;
    }

    tframeData.blendFactor = factor;
    const float f[4] = { factor.x(), factor.y(), factor.z(), factor.w() };
    commandList->OMSetBlendFactor(f);
}

void QSGD3D12EnginePrivate::queueSetStencilRef(quint32 ref)
{
    if (!inFrame) {
        qWarning("%s: Cannot be called outside begin/endFrame", __FUNCTION__);
        return;
    }

    tframeData.stencilRef = ref;
    commandList->OMSetStencilRef(ref);
}

void QSGD3D12EnginePrivate::queueDraw(QSGGeometry::DrawingMode mode, int count, int vboOffset, int vboSize, int vboStride,
                                      int cboOffset,
                                      int startIndexIndex, QSGD3D12Format indexFormat)
{
    if (!inFrame) {
        qWarning("%s: Cannot be called outside begin/endFrame", __FUNCTION__);
        return;
    }

    PersistentFrameData &pfd(pframeData[currentPFrameIndex]);

    // Ensure buffers are created but do not copy the data here, leave that to endDrawCalls().
    ensureBuffer(&vertexData, &pfd.vertex, "vertex");
    if (!pfd.vertex.buffer)
        return;

    if (indexData.size > 0) {
        ensureBuffer(&indexData, &pfd.index, "index");
        if (!pfd.index.buffer)
            return;
    } else if (indexData.allDirty) {
        indexData.allDirty = false;
        pfd.index.buffer = nullptr;
    }

    ensureBuffer(&constantData, &pfd.constant, "constant");
    if (!pfd.constant.buffer)
        return;

    // Set the CBV.
    if (cboOffset >= 0 && pfd.constant.buffer)
        commandList->SetGraphicsRootConstantBufferView(0, pfd.constant.buffer->GetGPUVirtualAddress() + cboOffset);

    // Set up vertex and index buffers.
    Q_ASSERT(pfd.vertex.buffer);
    Q_ASSERT(pfd.index.buffer || startIndexIndex < 0);

    if (mode != tframeData.drawingMode) {
        D3D_PRIMITIVE_TOPOLOGY topology;
        switch (mode) {
        case QSGGeometry::DrawPoints:
            topology = D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
            break;
        case QSGGeometry::DrawLines:
            topology = D3D_PRIMITIVE_TOPOLOGY_LINELIST;
            break;
        case QSGGeometry::DrawLineStrip:
            topology = D3D_PRIMITIVE_TOPOLOGY_LINESTRIP;
            break;
        case QSGGeometry::DrawTriangles:
            topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
            break;
        case QSGGeometry::DrawTriangleStrip:
            topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
            break;
        default:
            qFatal("Unsupported drawing mode 0x%x", mode);
            break;
        }
        commandList->IASetPrimitiveTopology(topology);
        tframeData.drawingMode = mode;
    }

    D3D12_VERTEX_BUFFER_VIEW vbv;
    vbv.BufferLocation = pfd.vertex.buffer->GetGPUVirtualAddress() + vboOffset;
    vbv.SizeInBytes = vboSize;
    vbv.StrideInBytes = vboStride;

    // must be set after the topology
    commandList->IASetVertexBuffers(0, 1, &vbv);

    if (startIndexIndex >= 0 && !tframeData.indexBufferSet) {
        tframeData.indexBufferSet = true;
        D3D12_INDEX_BUFFER_VIEW ibv;
        ibv.BufferLocation = pfd.index.buffer->GetGPUVirtualAddress();
        ibv.SizeInBytes = indexData.size;
        ibv.Format = DXGI_FORMAT(indexFormat);
        commandList->IASetIndexBuffer(&ibv);
    }

    // Copy the SRVs to a drawcall-dedicated area of the shader-visible descriptor heap.
    Q_ASSERT(tframeData.activeTextures.count() == tframeData.pipelineState.shaders.rootSig.textureViews.count());
    if (!tframeData.activeTextures.isEmpty()) {
        ensureGPUDescriptorHeap(tframeData.activeTextures.count());
        const uint stride = cpuDescHeapManager.handleSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        D3D12_CPU_DESCRIPTOR_HANDLE dst = pfd.gpuCbvSrvUavHeap->GetCPUDescriptorHandleForHeapStart();
        dst.ptr += pfd.cbvSrvUavNextFreeDescriptorIndex * stride;
        for (const TransientFrameData::ActiveTexture &t : qAsConst(tframeData.activeTextures)) {
            Q_ASSERT(t.id);
            const int idx = t.id - 1;
            const bool isTex = t.type == TransientFrameData::ActiveTexture::TypeTexture;
            device->CopyDescriptorsSimple(1, dst, isTex ? textures[idx].srv : renderTargets[idx].srv,
                                          D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
            dst.ptr += stride;
        }

        D3D12_GPU_DESCRIPTOR_HANDLE gpuAddr = pfd.gpuCbvSrvUavHeap->GetGPUDescriptorHandleForHeapStart();
        gpuAddr.ptr += pfd.cbvSrvUavNextFreeDescriptorIndex * stride;
        commandList->SetGraphicsRootDescriptorTable(1, gpuAddr);

        pfd.cbvSrvUavNextFreeDescriptorIndex += tframeData.activeTextures.count();
        tframeData.activeTextures.clear();
    }

    // Add the draw call.
    if (startIndexIndex >= 0)
        commandList->DrawIndexedInstanced(count, 1, startIndexIndex, 0, 0);
    else
        commandList->DrawInstanced(count, 1, 0, 0);

    ++tframeData.drawCount;
    if (tframeData.drawCount == MAX_DRAW_CALLS_PER_LIST) {
        if (Q_UNLIKELY(debug_render()))
            qDebug("Limit of %d draw calls reached, executing command list", MAX_DRAW_CALLS_PER_LIST);
        // submit the command list
        endDrawCalls();
        // start a new one
        beginDrawCalls();
        // prepare for the upcoming drawcalls
        queueSetRenderTarget(currentRenderTarget);
        queueViewport(tframeData.viewport);
        queueScissor(tframeData.scissor);
        queueSetBlendFactor(tframeData.blendFactor);
        queueSetStencilRef(tframeData.stencilRef);
        finalizePipeline(tframeData.pipelineState);
    }
}

void QSGD3D12EnginePrivate::ensureGPUDescriptorHeap(int cbvSrvUavDescriptorCount)
{
    PersistentFrameData &pfd(pframeData[currentPFrameIndex]);
    int newSize = pfd.gpuCbvSrvUavHeapSize;
    while (pfd.cbvSrvUavNextFreeDescriptorIndex + cbvSrvUavDescriptorCount > newSize)
        newSize *= 2;
    if (newSize != pfd.gpuCbvSrvUavHeapSize) {
        if (Q_UNLIKELY(debug_render()))
            qDebug("Out of space for SRVs, creating new CBV-SRV-UAV descriptor heap with descriptor count %d", newSize);
        deferredDelete(pfd.gpuCbvSrvUavHeap);
        createCbvSrvUavHeap(currentPFrameIndex, newSize);
        setDescriptorHeaps(true);
        pfd.cbvSrvUavNextFreeDescriptorIndex = 0;
    }
}

void QSGD3D12EnginePrivate::present()
{
    if (!initialized)
        return;

    if (Q_UNLIKELY(debug_render()))
        qDebug("--- present with vsync ---");

    HRESULT hr = swapChain->Present(1, 0);
    if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET) {
        deviceManager()->deviceLossDetected();
        return;
    } else if (FAILED(hr)) {
        qWarning("Present failed: 0x%x", hr);
        return;
    }

    ++presentFrameIndex;
}

void QSGD3D12EnginePrivate::waitGPU()
{
    if (!initialized)
        return;

    if (Q_UNLIKELY(debug_render()))
        qDebug("--- blocking wait for GPU ---");

    waitForGPU(presentFence);
}

template<class T> uint newId(T *tbl)
{
    uint id = 0;
    for (int i = 0; i < tbl->count(); ++i) {
        if (!(*tbl)[i].entryInUse()) {
            id = i + 1;
            break;
        }
    }

    if (!id) {
        tbl->resize(tbl->size() + 1);
        id = tbl->count();
    }

    (*tbl)[id - 1].flags = 0x01; // reset flags and set EntryInUse

    return id;
}

template<class T> void syncEntryFlags(T *e, int flag, bool b)
{
    if (b)
        e->flags |= flag;
    else
        e->flags &= ~flag;
}

uint QSGD3D12EnginePrivate::genTexture()
{
    const uint id = newId(&textures);
    textures[id - 1].fenceValue = 0;
    return id;
}

static inline DXGI_FORMAT textureFormat(QImage::Format format, bool wantsAlpha, bool mipmap,
                                        QImage::Format *imageFormat, int *bytesPerPixel)
{
    DXGI_FORMAT f = DXGI_FORMAT_R8G8B8A8_UNORM;
    QImage::Format convFormat = format;
    int bpp = 4;

    if (!mipmap) {
        switch (format) {
        case QImage::Format_Grayscale8:
        case QImage::Format_Indexed8:
        case QImage::Format_Alpha8:
            f = DXGI_FORMAT_R8_UNORM;
            bpp = 1;
            break;
        case QImage::Format_RGB32:
            f = DXGI_FORMAT_B8G8R8A8_UNORM;
            break;
        case QImage::Format_ARGB32:
            f = DXGI_FORMAT_B8G8R8A8_UNORM;
            convFormat = wantsAlpha ? QImage::Format_ARGB32_Premultiplied : QImage::Format_RGB32;
            break;
        case QImage::Format_ARGB32_Premultiplied:
            f = DXGI_FORMAT_B8G8R8A8_UNORM;
            convFormat = wantsAlpha ? format : QImage::Format_RGB32;
            break;
        default:
            convFormat = wantsAlpha ? QImage::Format_RGBA8888_Premultiplied : QImage::Format_RGBX8888;
            break;
        }
    } else {
        // Mipmap generation needs unordered access and BGRA is not an option for that. Stick to RGBA.
        convFormat = wantsAlpha ? QImage::Format_RGBA8888_Premultiplied : QImage::Format_RGBX8888;
    }

    if (imageFormat)
        *imageFormat = convFormat;

    if (bytesPerPixel)
        *bytesPerPixel = bpp;

    return f;
}

void QSGD3D12EnginePrivate::createTexture(uint id, const QSize &size, QImage::Format format,
                                          QSGD3D12Engine::TextureCreateFlags createFlags)
{
    Q_ASSERT(id);
    const int idx = id - 1;
    Q_ASSERT(idx < textures.count() && textures[idx].entryInUse());
    Texture &t(textures[idx]);

    syncEntryFlags(&t, Texture::Alpha, createFlags & QSGD3D12Engine::CreateWithAlpha);
    syncEntryFlags(&t, Texture::MipMap, createFlags & QSGD3D12Engine::CreateWithMipMaps);

    const QSize adjustedSize = !t.mipmap() ? size : QSGD3D12Engine::mipMapAdjustedSourceSize(size);

    D3D12_HEAP_PROPERTIES defaultHeapProp = {};
    defaultHeapProp.Type = D3D12_HEAP_TYPE_DEFAULT;

    D3D12_RESOURCE_DESC textureDesc = {};
    textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    textureDesc.Width = adjustedSize.width();
    textureDesc.Height = adjustedSize.height();
    textureDesc.DepthOrArraySize = 1;
    textureDesc.MipLevels = !t.mipmap() ? 1 : QSGD3D12Engine::mipMapLevels(adjustedSize);
    textureDesc.Format = textureFormat(format, t.alpha(), t.mipmap(), nullptr, nullptr);
    textureDesc.SampleDesc.Count = 1;
    textureDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    if (t.mipmap())
        textureDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

    HRESULT hr = device->CreateCommittedResource(&defaultHeapProp, D3D12_HEAP_FLAG_NONE, &textureDesc,
                                                 D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&t.texture));
    if (FAILED(hr)) {
        qWarning("Failed to create texture resource: 0x%x", hr);
        return;
    }

    t.srv = cpuDescHeapManager.allocate(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format = textureDesc.Format;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = textureDesc.MipLevels;

    device->CreateShaderResourceView(t.texture.Get(), &srvDesc, t.srv);

    if (t.mipmap()) {
        // Mipmap generation will need an UAV for each level that needs to be generated.
        t.mipUAVs.clear();
        for (int level = 1; level < textureDesc.MipLevels; ++level) {
            D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
            uavDesc.Format = textureDesc.Format;
            uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
            uavDesc.Texture2D.MipSlice = level;
            D3D12_CPU_DESCRIPTOR_HANDLE h = cpuDescHeapManager.allocate(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
            device->CreateUnorderedAccessView(t.texture.Get(), nullptr, &uavDesc, h);
            t.mipUAVs.append(h);
        }
    }

    if (Q_UNLIKELY(debug_render()))
        qDebug("created texture %u, size %dx%d, miplevels %d", id, adjustedSize.width(), adjustedSize.height(), textureDesc.MipLevels);
}

void QSGD3D12EnginePrivate::queueTextureResize(uint id, const QSize &size)
{
    Q_ASSERT(id);
    const int idx = id - 1;
    Q_ASSERT(idx < textures.count() && textures[idx].entryInUse());
    Texture &t(textures[idx]);

    if (!t.texture) {
        qWarning("Cannot resize non-created texture %u", id);
        return;
    }

    if (t.mipmap()) {
        qWarning("Cannot resize mipmapped texture %u", id);
        return;
    }

    if (Q_UNLIKELY(debug_render()))
        qDebug("resizing texture %u, size %dx%d", id, size.width(), size.height());

    D3D12_RESOURCE_DESC textureDesc = t.texture->GetDesc();
    textureDesc.Width = size.width();
    textureDesc.Height = size.height();

    D3D12_HEAP_PROPERTIES defaultHeapProp = {};
    defaultHeapProp.Type = D3D12_HEAP_TYPE_DEFAULT;

    ComPtr<ID3D12Resource> oldTexture = t.texture;
    deferredDelete(t.texture);

    HRESULT hr = device->CreateCommittedResource(&defaultHeapProp, D3D12_HEAP_FLAG_NONE, &textureDesc,
                                                 D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&t.texture));
    if (FAILED(hr)) {
        qWarning("Failed to create resized texture resource: 0x%x", hr);
        return;
    }

    deferredDelete(t.srv);
    t.srv = cpuDescHeapManager.allocate(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format = textureDesc.Format;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = textureDesc.MipLevels;

    device->CreateShaderResourceView(t.texture.Get(), &srvDesc, t.srv);

    D3D12_TEXTURE_COPY_LOCATION dstLoc;
    dstLoc.pResource = t.texture.Get();
    dstLoc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    dstLoc.SubresourceIndex = 0;

    D3D12_TEXTURE_COPY_LOCATION srcLoc;
    srcLoc.pResource = oldTexture.Get();
    srcLoc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    srcLoc.SubresourceIndex = 0;

    copyCommandList->Reset(copyCommandAllocator.Get(), nullptr);

    copyCommandList->CopyTextureRegion(&dstLoc, 0, 0, 0, &srcLoc, nullptr);

    copyCommandList->Close();
    ID3D12CommandList *commandLists[] = { copyCommandList.Get() };
    copyCommandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

    t.fenceValue = nextTextureUploadFenceValue.fetchAndAddAcquire(1) + 1;
    copyCommandQueue->Signal(textureUploadFence.Get(), t.fenceValue);

    if (Q_UNLIKELY(debug_render()))
        qDebug("submitted old content copy for texture %u on the copy queue, fence %llu", id, t.fenceValue);
}

void QSGD3D12EnginePrivate::queueTextureUpload(uint id, const QVector<QImage> &images, const QVector<QPoint> &dstPos)
{
    Q_ASSERT(id);
    Q_ASSERT(images.count() == dstPos.count());
    if (images.isEmpty())
        return;

    const int idx = id - 1;
    Q_ASSERT(idx < textures.count() && textures[idx].entryInUse());
    Texture &t(textures[idx]);
    Q_ASSERT(t.texture);

    // When mipmapping is not in use, image can be smaller than the size passed
    // to createTexture() and dstPos can specify a non-zero destination position.

    if (t.mipmap() && (images.count() != 1 || dstPos.count() != 1 || !dstPos[0].isNull())) {
        qWarning("Mipmapped textures (%u) do not support partial uploads", id);
        return;
    }

    // Make life simpler by disallowing queuing a new mipmapped upload before the previous one finishes.
    if (t.mipmap() && t.fenceValue) {
        qWarning("Attempted to queue mipmapped texture upload (%u) while a previous upload is still in progress", id);
        return;
    }

    t.fenceValue = nextTextureUploadFenceValue.fetchAndAddAcquire(1) + 1;

    if (Q_UNLIKELY(debug_render()))
        qDebug("adding upload for texture %u on the copy queue, fence %llu", id, t.fenceValue);

    D3D12_RESOURCE_DESC textureDesc = t.texture->GetDesc();
    const QSize adjustedTextureSize(textureDesc.Width, textureDesc.Height);

    int totalSize = 0;
    for (const QImage &image : images) {
        int bytesPerPixel;
        textureFormat(image.format(), t.alpha(), t.mipmap(), nullptr, &bytesPerPixel);
        const int w = !t.mipmap() ? image.width() : adjustedTextureSize.width();
        const int h = !t.mipmap() ? image.height() : adjustedTextureSize.height();
        const int stride = alignedSize(w * bytesPerPixel, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);
        totalSize += alignedSize(h * stride, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);
    }

    if (Q_UNLIKELY(debug_render()))
        qDebug("%d sub-uploads, heap size %d bytes", images.count(), totalSize);

    // Instead of individual committed resources for each upload buffer,
    // allocate only once and use placed resources.
    D3D12_HEAP_PROPERTIES uploadHeapProp = {};
    uploadHeapProp.Type = D3D12_HEAP_TYPE_UPLOAD;
    D3D12_HEAP_DESC uploadHeapDesc = {};
    uploadHeapDesc.SizeInBytes = totalSize;
    uploadHeapDesc.Properties = uploadHeapProp;
    uploadHeapDesc.Flags = D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;

    Texture::StagingHeap sheap;
    if (FAILED(device->CreateHeap(&uploadHeapDesc, IID_PPV_ARGS(&sheap.heap)))) {
        qWarning("Failed to create texture upload heap of size %d", totalSize);
        return;
    }
    t.stagingHeaps.append(sheap);

    copyCommandList->Reset(copyCommandAllocator.Get(), nullptr);

    int placedOffset = 0;
    for (int i = 0; i < images.count(); ++i) {
        QImage::Format convFormat;
        int bytesPerPixel;
        textureFormat(images[i].format(), t.alpha(), t.mipmap(), &convFormat, &bytesPerPixel);
        if (Q_UNLIKELY(debug_render() && i == 0))
            qDebug("source image format %d, target format %d, bpp %d", images[i].format(), convFormat, bytesPerPixel);

        QImage convImage = images[i].format() == convFormat ? images[i] : images[i].convertToFormat(convFormat);

        if (t.mipmap() && adjustedTextureSize != convImage.size())
            convImage = convImage.scaled(adjustedTextureSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

        const int stride = alignedSize(convImage.width() * bytesPerPixel, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);

        D3D12_RESOURCE_DESC bufDesc = {};
        bufDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        bufDesc.Width = stride * convImage.height();
        bufDesc.Height = 1;
        bufDesc.DepthOrArraySize = 1;
        bufDesc.MipLevels = 1;
        bufDesc.Format = DXGI_FORMAT_UNKNOWN;
        bufDesc.SampleDesc.Count = 1;
        bufDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

        Texture::StagingBuffer sbuf;
        if (FAILED(device->CreatePlacedResource(sheap.heap.Get(), placedOffset,
                                                &bufDesc, D3D12_RESOURCE_STATE_GENERIC_READ,
                                                nullptr, IID_PPV_ARGS(&sbuf.buffer)))) {
            qWarning("Failed to create texture upload buffer");
            return;
        }

        quint8 *p = nullptr;
        const D3D12_RANGE readRange = { 0, 0 };
        if (FAILED(sbuf.buffer->Map(0, &readRange, reinterpret_cast<void **>(&p)))) {
            qWarning("Map failed (texture upload buffer)");
            return;
        }
        for (int y = 0, ye = convImage.height(); y < ye; ++y) {
            memcpy(p, convImage.constScanLine(y), convImage.width() * bytesPerPixel);
            p += stride;
        }
        sbuf.buffer->Unmap(0, nullptr);

        D3D12_TEXTURE_COPY_LOCATION dstLoc;
        dstLoc.pResource = t.texture.Get();
        dstLoc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        dstLoc.SubresourceIndex = 0;

        D3D12_TEXTURE_COPY_LOCATION srcLoc;
        srcLoc.pResource = sbuf.buffer.Get();
        srcLoc.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        srcLoc.PlacedFootprint.Offset = 0;
        srcLoc.PlacedFootprint.Footprint.Format = textureDesc.Format;
        srcLoc.PlacedFootprint.Footprint.Width = convImage.width();
        srcLoc.PlacedFootprint.Footprint.Height = convImage.height();
        srcLoc.PlacedFootprint.Footprint.Depth = 1;
        srcLoc.PlacedFootprint.Footprint.RowPitch = stride;

        copyCommandList->CopyTextureRegion(&dstLoc, dstPos[i].x(), dstPos[i].y(), 0, &srcLoc, nullptr);

        t.stagingBuffers.append(sbuf);
        placedOffset += alignedSize(bufDesc.Width, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);
    }

    copyCommandList->Close();
    ID3D12CommandList *commandLists[] = { copyCommandList.Get() };
    copyCommandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);
    copyCommandQueue->Signal(textureUploadFence.Get(), t.fenceValue);
}

void QSGD3D12EnginePrivate::releaseTexture(uint id)
{
    if (!id)
        return;

    const int idx = id - 1;
    Q_ASSERT(idx < textures.count());

    if (Q_UNLIKELY(debug_render()))
        qDebug("releasing texture %d", id);

    Texture &t(textures[idx]);
    if (!t.entryInUse())
        return;

    if (t.texture) {
        deferredDelete(t.texture);
        deferredDelete(t.srv);
        for (D3D12_CPU_DESCRIPTOR_HANDLE h : t.mipUAVs)
            deferredDelete(h);
    }

    QSet<uint> *pendingTextureReleasesSet = inFrame
            ? &pframeData[currentPFrameIndex].pendingTextureReleases
            : &pframeData[(currentPFrameIndex + 1) % MAX_FRAMES_IN_FLIGHT].outOfFramePendingTextureReleases;

    pendingTextureReleasesSet->insert(id);
}

SIZE_T QSGD3D12EnginePrivate::textureSRV(uint id) const
{
    Q_ASSERT(id);
    const int idx = id - 1;
    Q_ASSERT(idx < textures.count() && textures[idx].entryInUse());
    return textures[idx].srv.ptr;
}

void QSGD3D12EnginePrivate::activateTexture(uint id)
{
    if (!inFrame) {
        qWarning("%s: Cannot be called outside begin/endFrame", __FUNCTION__);
        return;
    }

    Q_ASSERT(id);
    const int idx = id - 1;
    Q_ASSERT(idx < textures.count() && textures[idx].entryInUse());

    // activeTextures is a vector because the order matters
    tframeData.activeTextures.append(TransientFrameData::ActiveTexture(TransientFrameData::ActiveTexture::TypeTexture, id));

    if (textures[idx].fenceValue)
        pframeData[currentPFrameIndex].pendingTextureUploads.insert(id);
}

bool QSGD3D12EnginePrivate::MipMapGen::initialize(QSGD3D12EnginePrivate *enginePriv)
{
    engine = enginePriv;

    D3D12_STATIC_SAMPLER_DESC sampler = {};
    sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    sampler.MinLOD = 0.0f;
    sampler.MaxLOD = D3D12_FLOAT32_MAX;

    D3D12_DESCRIPTOR_RANGE descRange[2];
    descRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    descRange[0].NumDescriptors = 1;
    descRange[0].BaseShaderRegister = 0; // t0
    descRange[0].RegisterSpace = 0;
    descRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
    descRange[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
    descRange[1].NumDescriptors = 4;
    descRange[1].BaseShaderRegister = 0; // u0..u3
    descRange[1].RegisterSpace = 0;
    descRange[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    // Split into two to allow switching between the first and second set of UAVs later.
    D3D12_ROOT_PARAMETER rootParameters[3];
    rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    rootParameters[0].DescriptorTable.NumDescriptorRanges = 1;
    rootParameters[0].DescriptorTable.pDescriptorRanges = &descRange[0];

    rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    rootParameters[1].DescriptorTable.NumDescriptorRanges = 1;
    rootParameters[1].DescriptorTable.pDescriptorRanges = &descRange[1];

    rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    rootParameters[2].Constants.Num32BitValues = 4; // uint2 mip1Size, uint sampleLevel, uint totalMips
    rootParameters[2].Constants.ShaderRegister = 0; // b0
    rootParameters[2].Constants.RegisterSpace = 0;

    D3D12_ROOT_SIGNATURE_DESC desc = {};
    desc.NumParameters = 3;
    desc.pParameters = rootParameters;
    desc.NumStaticSamplers = 1;
    desc.pStaticSamplers = &sampler;

    ComPtr<ID3DBlob> signature;
    ComPtr<ID3DBlob> error;
    if (FAILED(D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error))) {
        QByteArray msg(static_cast<const char *>(error->GetBufferPointer()), error->GetBufferSize());
        qWarning("Failed to serialize compute root signature: %s", qPrintable(msg));
        return false;
    }
    if (FAILED(engine->device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(),
                                                   IID_PPV_ARGS(&rootSig)))) {
        qWarning("Failed to create compute root signature");
        return false;
    }

    D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.pRootSignature = rootSig.Get();
    psoDesc.CS.pShaderBytecode = g_CS_Generate4MipMaps;
    psoDesc.CS.BytecodeLength = sizeof(g_CS_Generate4MipMaps);

    if (FAILED(engine->device->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(&pipelineState)))) {
        qWarning("Failed to create compute pipeline state");
        return false;
    }

    return true;
}

void QSGD3D12EnginePrivate::MipMapGen::releaseResources()
{
    pipelineState = nullptr;
    rootSig = nullptr;
}

// The mipmap generator is used to insert commands on the main 3D queue. It is
// guaranteed that the queue has a wait for the base texture level upload
// before invoking queueGenerate(). There can be any number of invocations
// without waiting for earlier ones to finish. finished() is invoked when it is
// known for sure that frame containing the upload and mipmap generation has
// finished on the GPU.

void QSGD3D12EnginePrivate::MipMapGen::queueGenerate(const Texture &t)
{
    D3D12_RESOURCE_DESC textureDesc = t.texture->GetDesc();

    engine->commandList->SetPipelineState(pipelineState.Get());
    engine->commandList->SetComputeRootSignature(rootSig.Get());

    // 1 SRV + (miplevels - 1) UAVs
    const int descriptorCount = 1 + (textureDesc.MipLevels - 1);

    engine->ensureGPUDescriptorHeap(descriptorCount);

    // The descriptor heap is set on the command list either because the
    // ensure() call above resized, or, typically, due to a texture-dependent
    // draw call earlier.

    engine->transitionResource(t.texture.Get(), engine->commandList.Get(),
                               D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

    QSGD3D12EnginePrivate::PersistentFrameData &pfd(engine->pframeData[engine->currentPFrameIndex]);

    const uint stride = engine->cpuDescHeapManager.handleSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    D3D12_CPU_DESCRIPTOR_HANDLE h = pfd.gpuCbvSrvUavHeap->GetCPUDescriptorHandleForHeapStart();
    h.ptr += pfd.cbvSrvUavNextFreeDescriptorIndex * stride;

    engine->device->CopyDescriptorsSimple(1, h, t.srv, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    h.ptr += stride;

    for (int level = 1; level < textureDesc.MipLevels; ++level, h.ptr += stride)
        engine->device->CopyDescriptorsSimple(1, h, t.mipUAVs[level - 1], D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    D3D12_GPU_DESCRIPTOR_HANDLE gpuAddr = pfd.gpuCbvSrvUavHeap->GetGPUDescriptorHandleForHeapStart();
    gpuAddr.ptr += pfd.cbvSrvUavNextFreeDescriptorIndex * stride;

    engine->commandList->SetComputeRootDescriptorTable(0, gpuAddr);
    gpuAddr.ptr += stride; // now points to the first UAV

    for (int level = 1; level < textureDesc.MipLevels; level += 4, gpuAddr.ptr += stride * 4) {
        engine->commandList->SetComputeRootDescriptorTable(1, gpuAddr);

        QSize sz(textureDesc.Width, textureDesc.Height);
        sz.setWidth(qMax(1, sz.width() >> level));
        sz.setHeight(qMax(1, sz.height() >> level));

        const quint32 constants[4] = { quint32(sz.width()), quint32(sz.height()),
                                       quint32(level - 1),
                                       quint32(textureDesc.MipLevels - 1) };

        engine->commandList->SetComputeRoot32BitConstants(2, 4, constants, 0);
        engine->commandList->Dispatch(sz.width(), sz.height(), 1);
        engine->uavBarrier(t.texture.Get(), engine->commandList.Get());
    }

    engine->transitionResource(t.texture.Get(), engine->commandList.Get(),
                               D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    pfd.cbvSrvUavNextFreeDescriptorIndex += descriptorCount;
}

void QSGD3D12EnginePrivate::deferredDelete(ComPtr<ID3D12Resource> res)
{
    PersistentFrameData::DeleteQueueEntry e;
    e.res = res;
    QVector<PersistentFrameData::DeleteQueueEntry> *dq = inFrame
            ? &pframeData[currentPFrameIndex].deleteQueue
            : &pframeData[(currentPFrameIndex + 1) % MAX_FRAMES_IN_FLIGHT].outOfFrameDeleteQueue;
    (*dq) << e;
}

void QSGD3D12EnginePrivate::deferredDelete(ComPtr<ID3D12DescriptorHeap> dh)
{
    PersistentFrameData::DeleteQueueEntry e;
    e.descHeap = dh;
    QVector<PersistentFrameData::DeleteQueueEntry> *dq = inFrame
            ? &pframeData[currentPFrameIndex].deleteQueue
            : &pframeData[(currentPFrameIndex + 1) % MAX_FRAMES_IN_FLIGHT].outOfFrameDeleteQueue;
    (*dq) << e;
}

void QSGD3D12EnginePrivate::deferredDelete(D3D12_CPU_DESCRIPTOR_HANDLE h)
{
    PersistentFrameData::DeleteQueueEntry e;
    e.cpuDescriptorPtr = h.ptr;
    QVector<PersistentFrameData::DeleteQueueEntry> *dq = inFrame
            ? &pframeData[currentPFrameIndex].deleteQueue
            : &pframeData[(currentPFrameIndex + 1) % MAX_FRAMES_IN_FLIGHT].outOfFrameDeleteQueue;
    (*dq) << e;
}

uint QSGD3D12EnginePrivate::genRenderTarget()
{
    return newId(&renderTargets);
}

void QSGD3D12EnginePrivate::createRenderTarget(uint id, const QSize &size, const QVector4D &clearColor, int samples)
{
    Q_ASSERT(id);
    const int idx = id - 1;
    Q_ASSERT(idx < renderTargets.count() && renderTargets[idx].entryInUse());
    RenderTarget &rt(renderTargets[idx]);

    ID3D12Resource *res = createColorBuffer(rt.rtv, size, clearColor, samples);
    if (res)
        rt.color.Attach(res);

    ID3D12Resource *dsres = createDepthStencil(rt.dsv, size, samples);
    if (dsres)
        rt.ds.Attach(dsres);

    rt.rtv = cpuDescHeapManager.allocate(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    rt.dsv = cpuDescHeapManager.allocate(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    rt.srv = cpuDescHeapManager.allocate(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    device->CreateRenderTargetView(rt.color.Get(), nullptr, rt.rtv);
    device->CreateDepthStencilView(rt.ds.Get(), nullptr, rt.dsv);

    const bool multisample = rt.color->GetDesc().SampleDesc.Count > 1;
    syncEntryFlags(&rt, RenderTarget::Multisample, multisample);

    if (!multisample) {
        device->CreateShaderResourceView(rt.color.Get(), nullptr, rt.srv);
    } else {
        D3D12_HEAP_PROPERTIES defaultHeapProp = {};
        defaultHeapProp.Type = D3D12_HEAP_TYPE_DEFAULT;

        D3D12_RESOURCE_DESC textureDesc = {};
        textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        textureDesc.Width = size.width();
        textureDesc.Height = size.height();
        textureDesc.DepthOrArraySize = 1;
        textureDesc.MipLevels = 1;
        textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        textureDesc.SampleDesc.Count = 1;
        textureDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

        HRESULT hr = device->CreateCommittedResource(&defaultHeapProp, D3D12_HEAP_FLAG_NONE, &textureDesc,
                                                     D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&rt.colorResolve));
        if (FAILED(hr)) {
            qWarning("Failed to create resolve buffer: 0x%x", hr);
            return;
        }

        device->CreateShaderResourceView(rt.colorResolve.Get(), nullptr, rt.srv);
    }

}

void QSGD3D12EnginePrivate::releaseRenderTarget(uint id)
{
    if (!id)
        return;

    const int idx = id - 1;
    Q_ASSERT(idx < renderTargets.count());
    RenderTarget &rt(renderTargets[idx]);
    if (!rt.entryInUse())
        return;

    if (rt.colorResolve) {
        deferredDelete(rt.colorResolve);
        rt.colorResolve = nullptr;
    }
    if (rt.color) {
        deferredDelete(rt.color);
        rt.color = nullptr;
        deferredDelete(rt.rtv);
        deferredDelete(rt.srv);
    }
    if (rt.ds) {
        deferredDelete(rt.ds);
        rt.ds = nullptr;
        deferredDelete(rt.dsv);
    }

    rt.flags &= ~RenderTarget::EntryInUse;
}

void QSGD3D12EnginePrivate::activateRenderTargetAsTexture(uint id)
{
    if (!inFrame) {
        qWarning("%s: Cannot be called outside begin/endFrame", __FUNCTION__);
        return;
    }

    Q_ASSERT(id);
    const int idx = id - 1;
    Q_ASSERT(idx < renderTargets.count());
    RenderTarget &rt(renderTargets[idx]);
    Q_ASSERT(rt.entryInUse() && rt.color);

    if (rt.flags & RenderTarget::NeedsReadBarrier) {
        if (rt.flags & RenderTarget::Multisample) {
            transitionResource(rt.color.Get(), commandList.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_RESOLVE_SOURCE);
            transitionResource(rt.colorResolve.Get(), commandList.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RESOLVE_DEST);
            commandList->ResolveSubresource(rt.colorResolve.Get(), 0, rt.color.Get(), 0, DXGI_FORMAT_R8G8B8A8_UNORM);
            transitionResource(rt.colorResolve.Get(), commandList.Get(), D3D12_RESOURCE_STATE_RESOLVE_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
            transitionResource(rt.color.Get(), commandList.Get(), D3D12_RESOURCE_STATE_RESOLVE_SOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
        } else {
            transitionResource(rt.color.Get(), commandList.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        }
        rt.flags &= ~RenderTarget::NeedsReadBarrier;
    }

    tframeData.activeTextures.append(TransientFrameData::ActiveTexture::ActiveTexture(TransientFrameData::ActiveTexture::TypeRenderTarget, id));
}

QT_END_NAMESPACE
