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
#include <QString>
#include <QColor>

QT_BEGIN_NAMESPACE

// Recommended reading before moving further: https://github.com/Microsoft/DirectXTK/wiki/ComPtr
// Note esp. operator= vs. Attach and operator& vs. GetAddressOf

static const int MAX_DESCRIPTORS_PER_HEAP = 256;

QSGD3D12DescriptorHandle QSGD3D12DescriptorHeapManager::allocate(D3D12_DESCRIPTOR_HEAP_TYPE type, Flags flags)
{
    QSGD3D12DescriptorHandle h;
    for (Heap &heap : m_heaps) {
        if (heap.type == type && heap.count < MAX_DESCRIPTORS_PER_HEAP) {
            h = heap.start;
            h.cpu.ptr += heap.count * heap.handleSize;
            if (h.gpu.ptr)
                h.gpu.ptr += heap.count * heap.handleSize;
            ++heap.count;
            return h;
        }
    }

    Heap heap;
    heap.type = type;
    heap.handleSize = m_handleSizes[type];

    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.NumDescriptors = MAX_DESCRIPTORS_PER_HEAP;
    heapDesc.Type = type;
    if (flags & ShaderVisible)
        heapDesc.Flags |= D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

    HRESULT hr = m_device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&heap.heap));
    if (FAILED(hr)) {
        qWarning("Failed to create heap with type 0x%x: %x", type, hr);
        return h;
    }

    heap.start.cpu = heap.heap->GetCPUDescriptorHandleForHeapStart();
    qDebug("type %x start is %llu", type, heap.start.cpu.ptr);
    if (flags & ShaderVisible)
        heap.start.gpu = heap.heap->GetGPUDescriptorHandleForHeapStart();

    heap.count = 1;
    h = heap.start;
    m_heaps.append(heap);

    return h;
}

void QSGD3D12DescriptorHeapManager::initialize(ID3D12Device *device)
{
    m_device = device;

    for (int i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
        m_handleSizes[i] = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE(i));
}

void QSGD3D12DescriptorHeapManager::releaseResources()
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
        ComPtr<IDXGIAdapter> warpAdapter;
        m_factory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter));
        HRESULT hr = D3D12CreateDevice(warpAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_device));
        if (FAILED(hr)) {
            qWarning("Failed to create WARP device: 0x%x", hr);
            return;
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
    d->resize();
}

void QSGD3D12Engine::beginFrame()
{
    d->beginFrame();
}

void QSGD3D12Engine::endFrame()
{
    d->endFrame();
}

void QSGD3D12Engine::setPipelineState(const QSGD3D12PipelineState &pipelineState)
{
    d->setPipelineState(pipelineState);
}

void QSGD3D12Engine::setVertexBuffer(const quint8 *data, int size)
{
    d->setVertexBuffer(data, size);
}

void QSGD3D12Engine::setIndexBuffer(const quint8 *data, int size)
{
    d->setIndexBuffer(data, size);
}

void QSGD3D12Engine::setConstantBuffer(const quint8 *data, int size)
{
    d->setConstantBuffer(data, size);
}

void QSGD3D12Engine::queueViewport(const QRect &rect)
{
    d->queueViewport(rect);
}

void QSGD3D12Engine::queueScissor(const QRect &rect)
{
    d->queueScissor(rect);
}

void QSGD3D12Engine::queueSetRenderTarget()
{
    d->queueSetRenderTarget();
}

void QSGD3D12Engine::queueClearRenderTarget(const QColor &color)
{
    d->queueClearRenderTarget(color);
}

void QSGD3D12Engine::queueClearDepthStencil(float depthValue, quint8 stencilValue)
{
    d->queueClearDepthStencil(depthValue, stencilValue);
}

void QSGD3D12Engine::queueDraw(QSGGeometry::DrawingMode mode, int count, int vboOffset, int vboStride,
                               int cboOffset,
                               int startIndexIndex, QSGD3D12Format indexFormat)
{
    d->queueDraw(mode, count, vboOffset, vboStride, cboOffset, startIndexIndex, indexFormat);
}

void QSGD3D12Engine::present()
{
    d->present();
}

void QSGD3D12Engine::waitGPU()
{
    d->waitGPU();
}

quint32 QSGD3D12Engine::alignedConstantBufferSize(quint32 size)
{
    return (size + D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1) & ~(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1);
}

void QSGD3D12EnginePrivate::releaseResources()
{
    if (!initialized)
        return;

    bundleAllocator = nullptr;
    commandAllocator = nullptr;

    depthStencil = nullptr;
    for (int i = 0; i < swapChainBufferCount; ++i)
        renderTargets[i] = nullptr;

    vertexBuffer = nullptr;
    indexBuffer = nullptr;
    constantBuffer = nullptr;

    for (ComPtr<ID3D12PipelineState> &ps : m_psoCache)
        ps = nullptr;
    m_psoCache.clear();
    m_rootSig = nullptr;

    descHeapManager.releaseResources();

    commandQueue = nullptr;
    swapChain = nullptr;

    delete presentFence;

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

    DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
    swapChainDesc.BufferCount = swapChainBufferCount;
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

    if (FAILED(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator)))) {
        qWarning("Failed to create command allocator");
        return;
    }

    if (FAILED(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_BUNDLE, IID_PPV_ARGS(&bundleAllocator)))) {
        qWarning("Failed to create command bundle allocator");
        return;
    }

    descHeapManager.initialize(device);

    setupRenderTargets();

    if (FAILED(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator.Get(),
                                         nullptr, IID_PPV_ARGS(&commandList)))) {
        qWarning("Failed to create command list");
        return;
    }
    // created in recording state, close it for now
    commandList->Close();

    presentFence = createFence();

    vertexData = StagingBufferRef();
    indexData = StagingBufferRef();
    constantData = StagingBufferRef();

    initialized = true;
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

ID3D12Resource *QSGD3D12EnginePrivate::createDepthStencil(D3D12_CPU_DESCRIPTOR_HANDLE viewHandle, const QSize &size, int samples)
{
    D3D12_CLEAR_VALUE depthClearValue = {};
    depthClearValue.Format = DXGI_FORMAT_D32_FLOAT;
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
    bufDesc.Format = DXGI_FORMAT_D32_FLOAT;
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
    depthStencilDesc.Format = DXGI_FORMAT_D32_FLOAT;
    depthStencilDesc.ViewDimension = bufDesc.SampleDesc.Count <= 1 ? D3D12_DSV_DIMENSION_TEXTURE2D : D3D12_DSV_DIMENSION_TEXTURE2DMS;

    device->CreateDepthStencilView(resource, &depthStencilDesc, viewHandle);

    return resource;
}

void QSGD3D12EnginePrivate::setupRenderTargets()
{
    for (int i = 0; i < swapChainBufferCount; ++i) {
        if (FAILED(swapChain->GetBuffer(i, IID_PPV_ARGS(&renderTargets[i])))) {
            qWarning("Failed to get buffer %d from swap chain", i);
            return;
        }
        D3D12_CPU_DESCRIPTOR_HANDLE h = descHeapManager.allocate(D3D12_DESCRIPTOR_HEAP_TYPE_RTV).cpu;
        if (i == 0)
            rtv0 = h;
        device->CreateRenderTargetView(renderTargets[i].Get(), nullptr, h);
    }

    dsv = descHeapManager.allocate(D3D12_DESCRIPTOR_HEAP_TYPE_DSV).cpu;
    ID3D12Resource *ds = createDepthStencil(dsv, window->size(), 0);
    if (ds)
        depthStencil.Attach(ds);
}

void QSGD3D12EnginePrivate::resize()
{
    if (!initialized)
        return;

    qDebug() << window->size();

    // Clear these, otherwise resizing will fail.
    depthStencil = nullptr;
    for (int i = 0; i < swapChainBufferCount; ++i)
        renderTargets[i] = nullptr;

    HRESULT hr = swapChain->ResizeBuffers(swapChainBufferCount, window->width(), window->height(), DXGI_FORMAT_R8G8B8A8_UNORM, 0);
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

QSGD3D12Fence *QSGD3D12EnginePrivate::createFence() const
{
    QSGD3D12Fence *f = new QSGD3D12Fence;
    HRESULT hr = device->CreateFence(f->value, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&f->fence));
    if (FAILED(hr)) {
        qWarning("Failed to create fence: 0x%x", hr);
        return f;
    }
    f->event = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    return f;
}

void QSGD3D12EnginePrivate::waitForGPU(QSGD3D12Fence *f) const
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
                                                 D3D12_RESOURCE_STATE_GENERIC_READ, Q_NULLPTR, IID_PPV_ARGS(&buf));
    if (FAILED(hr))
        qWarning("Failed to create buffer resource: 0x%x", hr);

    return buf;
}

ID3D12Resource *QSGD3D12EnginePrivate::backBufferRT() const
{
    return renderTargets[swapChain->GetCurrentBackBufferIndex()].Get();
}

D3D12_CPU_DESCRIPTOR_HANDLE QSGD3D12EnginePrivate::backBufferRTV() const
{
    const int frameIndex = swapChain->GetCurrentBackBufferIndex();
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtv0;
    rtvHandle.ptr += frameIndex * descHeapManager.handleSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    return rtvHandle;
}

void QSGD3D12EnginePrivate::beginFrame()
{
    static int cnt = 0;
    qDebug() << "***** begin frame" << cnt;
    ++cnt;

    // The device may have been lost. This is the point to attempt to start again from scratch.
    if (!initialized && window)
        initialize(window);

    commandAllocator->Reset();
    commandList->Reset(commandAllocator.Get(), nullptr);

    transitionResource(backBufferRT(), commandList.Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
}

void QSGD3D12EnginePrivate::endFrame()
{
    qDebug() << "***** end frame";

    transitionResource(backBufferRT(), commandList.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

    HRESULT hr = commandList->Close();
    if (FAILED(hr)) {
        qWarning("Failed to close command list: 0x%x", hr);
        if (hr == E_INVALIDARG)
            qWarning("Invalid arguments. Some of the commands in the list is invalid in some way.");
    }

    ID3D12CommandList *commandLists[] = { commandList.Get() };
    commandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);
}

void QSGD3D12EnginePrivate::setPipelineState(const QSGD3D12PipelineState &pipelineState)
{
    // One single root signature for now. This will obviously need some improvements later on...
    if (!m_rootSig) {
        D3D12_ROOT_PARAMETER rootParameter;
        rootParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        rootParameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
        rootParameter.Descriptor.ShaderRegister = 0; // b0
        rootParameter.Descriptor.RegisterSpace = 0;

        D3D12_ROOT_SIGNATURE_DESC desc;
        desc.NumParameters = 1;
        desc.pParameters = &rootParameter;
        desc.NumStaticSamplers = 0;
        desc.pStaticSamplers = nullptr;
        desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

        ComPtr<ID3DBlob> signature;
        ComPtr<ID3DBlob> error;
        if (FAILED(D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error))) {
            qWarning("Failed to serialize root signature");
            return;
        }
        if (FAILED(device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(),
                                               IID_PPV_ARGS(&m_rootSig)))) {
            qWarning("Failed to create root signature");
            return;
        }
    }

    // Unlimited number of cached PSOs for now, may want to change this later.
    ComPtr<ID3D12PipelineState> pso = m_psoCache[pipelineState];

    if (!pso) {
        qDebug("NEW PSO");
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};

        D3D12_INPUT_ELEMENT_DESC inputElements[8];
        Q_ASSERT(pipelineState.inputElements.count() <= _countof(inputElements));
        UINT ieIdx = 0;
        for (const QSGD3D12InputElement &ie : pipelineState.inputElements) {
            D3D12_INPUT_ELEMENT_DESC ieDesc = {};
            ieDesc.SemanticName = ie.name;
            ieDesc.Format = DXGI_FORMAT(ie.format);
            ieDesc.InputSlot = ie.slot;
            ieDesc.AlignedByteOffset = ie.offset;
            ieDesc.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
            qDebug("input [%d]: %s %d 0x%x %d", ieIdx, ie.name, ie.offset, ie.format, ie.slot);
            inputElements[ieIdx++] = ieDesc;
        }

        psoDesc.InputLayout = { inputElements, ieIdx };

        psoDesc.pRootSignature = m_rootSig.Get();

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

        // ### this is wrong
        const D3D12_RENDER_TARGET_BLEND_DESC defaultRenderTargetBlendDesc = {
            TRUE, FALSE,
            D3D12_BLEND_SRC_ALPHA, D3D12_BLEND_INV_SRC_ALPHA, D3D12_BLEND_OP_ADD,
            D3D12_BLEND_ZERO, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
            D3D12_LOGIC_OP_NOOP,
            D3D12_COLOR_WRITE_ENABLE_ALL
        };
        D3D12_BLEND_DESC blendDesc = {};
        blendDesc.RenderTarget[0] = defaultRenderTargetBlendDesc;

        psoDesc.BlendState = blendDesc;

        psoDesc.DepthStencilState.DepthEnable = pipelineState.depthEnable;
        psoDesc.DepthStencilState.DepthWriteMask = pipelineState.depthWrite ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
        psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC(pipelineState.depthFunc);
        psoDesc.DepthStencilState.StencilEnable = pipelineState.stencilEnable;
        // ### stencil stuff
        psoDesc.SampleMask = UINT_MAX;
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE(pipelineState.topologyType);
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
        psoDesc.SampleDesc.Count = 1;

        HRESULT hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso));
        if (FAILED(hr)) {
            qWarning("Failed to create graphics pipeline state");
            return;
        }
        m_psoCache[pipelineState] = pso;
    }

    commandList->SetPipelineState(pso.Get());

    commandList->SetGraphicsRootSignature(m_rootSig.Get()); // invalidates bindings
}

void QSGD3D12EnginePrivate::setVertexBuffer(const quint8 *data, int size)
{
    vertexData.p = data;
    vertexData.size = size;
    vertexData.changed = true;
}

void QSGD3D12EnginePrivate::setIndexBuffer(const quint8 *data, int size)
{
    indexData.p = data;
    indexData.size = size;
    indexData.changed = true;
}

void QSGD3D12EnginePrivate::setConstantBuffer(const quint8 *data, int size)
{
    constantData.p = data;
    constantData.size = size;
    constantData.changed = true;
}

void QSGD3D12EnginePrivate::queueViewport(const QRect &rect)
{
    const D3D12_VIEWPORT viewport = { 0, 0, float(rect.width()), float(rect.height()), 0, 1 };
    commandList->RSSetViewports(1, &viewport);
}

void QSGD3D12EnginePrivate::queueScissor(const QRect &rect)
{
    const D3D12_RECT scissorRect = { 0, 0, rect.width() - 1, rect.height() - 1 };
    commandList->RSSetScissorRects(1, &scissorRect);
}

void QSGD3D12EnginePrivate::queueSetRenderTarget()
{
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = backBufferRTV();
    D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dsv;
    commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);
}

void QSGD3D12EnginePrivate::queueClearRenderTarget(const QColor &color)
{
    const float clearColor[] = { float(color.redF()), float(color.blueF()), float(color.greenF()), float(color.alphaF()) };
    commandList->ClearRenderTargetView(backBufferRTV(), clearColor, 0, nullptr);
}

void QSGD3D12EnginePrivate::queueClearDepthStencil(float depthValue, quint8 stencilValue)
{
    commandList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, depthValue, stencilValue, 0, nullptr);
}

void QSGD3D12EnginePrivate::queueDraw(QSGGeometry::DrawingMode mode, int count, int vboOffset, int vboStride,
                                      int cboOffset,
                                      int startIndexIndex, QSGD3D12Format indexFormat)
{
    // Due to the simplistic way our current renderer works, we can just upload the
    // entire vertex/index data in case it was rebuilt.

    if (vertexData.changed) {
        vertexData.changed = false;
        qDebug("upload vertex");
        // Only enlarge, never shrink
        const bool newBufferNeeded = vertexBuffer ? (vertexData.size > vertexBuffer->GetDesc().Width) : true;
        if (newBufferNeeded) {
            qDebug("new vertex buffer of size %d", vertexData.size);
            vertexBuffer.Attach(createBuffer(vertexData.size));
        }
        if (!vertexBuffer)
            return;
        quint8 *p = nullptr;
        D3D12_RANGE readRange = { 0, 0 };
        if (FAILED(vertexBuffer->Map(0, &readRange, reinterpret_cast<void **>(&p)))) {
            qWarning("Map failed for buffer of size %d", vertexData.size);
            return;
        }
        memcpy(p, vertexData.p, vertexData.size);
        vertexBuffer->Unmap(0, nullptr);
    }

    if (indexData.changed) {
        indexData.changed = false;
        if (indexData.size > 0) {
            qDebug("upload index");
            const bool newBufferNeeded = indexBuffer ? (indexData.size > indexBuffer->GetDesc().Width) : true;
            if (newBufferNeeded) {
                qDebug("new index buffer of size %d", indexData.size);
                indexBuffer.Attach(createBuffer(indexData.size));
            }
            if (!indexBuffer)
                return;
            quint8 *p = nullptr;
            D3D12_RANGE readRange = { 0, 0 };
            if (FAILED(indexBuffer->Map(0, &readRange, reinterpret_cast<void **>(&p)))) {
                qWarning("Map failed for buffer of size %d", indexData.size);
                return;
            }
            memcpy(p, indexData.p, indexData.size);
            indexBuffer->Unmap(0, nullptr);
        } else {
            indexBuffer = nullptr;
        }
    }

    if (constantData.changed) {
        constantData.changed = false;
        qDebug("upload constant");
        const bool newBufferNeeded = constantBuffer ? (constantData.size > constantBuffer->GetDesc().Width) : true;
        if (newBufferNeeded) {
            qDebug("new constant buffer of size %d", constantData.size);
            constantBuffer.Attach(createBuffer(constantData.size));
        }
        if (!constantBuffer)
            return;
        quint8 *p = nullptr;
        D3D12_RANGE readRange = { 0, 0 };
        if (FAILED(constantBuffer->Map(0, &readRange, reinterpret_cast<void **>(&p)))) {
            qWarning("Map failed for buffer of size %d", constantData.size);
            return;
        }
        memcpy(p, constantData.p, constantData.size);
        constantBuffer->Unmap(0, nullptr);
    }

    if (cboOffset >= 0 && constantBuffer)
        commandList->SetGraphicsRootConstantBufferView(0, constantBuffer->GetGPUVirtualAddress() + cboOffset);

    Q_ASSERT(vertexBuffer);

    D3D12_VERTEX_BUFFER_VIEW vbv;
    vbv.BufferLocation = vertexBuffer->GetGPUVirtualAddress() + vboOffset;
    vbv.SizeInBytes = vboStride * count;
    vbv.StrideInBytes = vboStride;

    Q_ASSERT(indexBuffer || startIndexIndex < 0);

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

    commandList->IASetVertexBuffers(0, 1, &vbv);

    if (startIndexIndex >= 0) {
        D3D12_INDEX_BUFFER_VIEW ibv;
        ibv.BufferLocation = indexBuffer->GetGPUVirtualAddress();
        ibv.SizeInBytes = indexData.size;
        ibv.Format = DXGI_FORMAT(indexFormat);
        commandList->IASetIndexBuffer(&ibv);
    }

    if (startIndexIndex >= 0)
        commandList->DrawIndexedInstanced(count, 1, startIndexIndex, 0, 0);
    else
        commandList->DrawInstanced(count, 1, 0, 0);
}

void QSGD3D12EnginePrivate::present()
{
    if (!initialized)
        return;

    qDebug("--- present with vsync ---");

    HRESULT hr = swapChain->Present(1, 0);
    if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET) {
        deviceManager()->deviceLossDetected();
        return;
    } else if (FAILED(hr)) {
        qWarning("Present failed: 0x%x", hr);
        return;
    }
}

void QSGD3D12EnginePrivate::waitGPU()
{
    if (!initialized)
        return;

    qDebug("--- blocking wait for GPU ---");
    waitForGPU(presentFence);
}

QT_END_NAMESPACE
