// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <qtest.h>

#include <QAnimationDriver>

#include <QQuickWindow>
#include <QQuickRenderControl>
#include <QQuickRenderTarget>
#include <QQuickGraphicsDevice>
#include <QQuickGraphicsConfiguration>
#include <QQuickItem>
#include <QQmlEngine>
#include <QQmlComponent>

#include <QtQuickTestUtils/private/qmlutils_p.h>

#include <QtGui/private/qguiapplication_p.h>
#include <QtGui/qpa/qplatformintegration.h>

#include <rhi/qrhi.h>

#if QT_CONFIG(vulkan)
#include <QVulkanInstance>
#include <QVulkanFunctions>
#endif

#include <QOperatingSystemVersion>

class AnimationDriver : public QAnimationDriver
{
public:
    AnimationDriver(int msPerStep) : m_step(msPerStep) { }

    void advance() override
    {
        m_elapsed += m_step;
        advanceAnimation();
    }

    qint64 elapsed() const override
    {
        return m_elapsed;
    }

private:
    int m_step;
    qint64 m_elapsed = 0;
};

class tst_RenderControl : public QQmlDataTest
{
    Q_OBJECT

public:
    tst_RenderControl();

private slots:
    void initTestCase() override;
    void cleanupTestCase();
    void renderAndReadBackWithRhi_data();
    void renderAndReadBackWithRhi();
    void renderAndReadBackWithVulkanNative();

private:
#if QT_CONFIG(vulkan)
    QVulkanInstance vulkanInstance;
#endif
    AnimationDriver *animDriver;
};

tst_RenderControl::tst_RenderControl()
    : QQmlDataTest(QT_QMLTEST_DATADIR)
{
}

void tst_RenderControl::initTestCase()
{
    QQmlDataTest::initTestCase();

#if QT_CONFIG(vulkan)
    vulkanInstance.setLayers({ "VK_LAYER_LUNARG_standard_validation" });
    vulkanInstance.setExtensions(QQuickGraphicsConfiguration::preferredInstanceExtensions());
    vulkanInstance.create(); // may fail, that's sometimes ok, we'll check for it later
#endif

    // Install the animation driver once, globally, instead of in the
    // individual tests. This tends to work better as it avoids the need to
    // have more complicated logic when calling advance().

    static const int ANIM_ADVANCE_PER_FRAME = 16; // milliseconds
    animDriver = new AnimationDriver(ANIM_ADVANCE_PER_FRAME);
    animDriver->install();
}

void tst_RenderControl::cleanupTestCase()
{
    delete animDriver;
}

void tst_RenderControl::renderAndReadBackWithRhi_data()
{
    QTest::addColumn<QSGRendererInterface::GraphicsApi>("api");

#if QT_CONFIG(opengl)
    QTest::newRow("OpenGL") << QSGRendererInterface::OpenGLRhi;
#endif
#if QT_CONFIG(vulkan)
    QTest::newRow("Vulkan") << QSGRendererInterface::VulkanRhi;
#endif
#ifdef Q_OS_WIN
    QTest::newRow("D3D11") << QSGRendererInterface::Direct3D11Rhi;
#endif
#if defined(Q_OS_MACOS) || defined(Q_OS_IOS)
    QTest::newRow("Metal") << QSGRendererInterface::MetalRhi;
#endif
}

void tst_RenderControl::renderAndReadBackWithRhi()
{
    QFETCH(QSGRendererInterface::GraphicsApi, api);
#if QT_CONFIG(vulkan)
    if (api == QSGRendererInterface::VulkanRhi && !vulkanInstance.isValid())
        QSKIP("Skipping Vulkan-based QRhi readback test due to failing to create a VkInstance");
#endif

#ifdef Q_OS_ANDROID
    // QTBUG-102780
    if (api == QSGRendererInterface::VulkanRhi)
        QSKIP("Vulkan-based rendering tests on Android are flaky.");
#endif

    // Changing the graphics api is not possible once a QQuickWindow et al is
    // created, however we do support changing it once all QQuickWindow,
    // QQuickRenderControl, etc. instances are destroyed, before creating new
    // ones. That's why it is possible to have this test run with multiple QRhi
    // backends.
    QQuickWindow::setGraphicsApi(api);

    QScopedPointer<QQuickRenderControl> renderControl(new QQuickRenderControl);
    QScopedPointer<QQuickWindow> quickWindow(new QQuickWindow(renderControl.data()));
#if QT_CONFIG(vulkan)
    if (api == QSGRendererInterface::VulkanRhi)
        quickWindow->setVulkanInstance(&vulkanInstance);
#endif

    QScopedPointer<QQmlEngine> qmlEngine(new QQmlEngine);
    QScopedPointer<QQmlComponent> qmlComponent(new QQmlComponent(qmlEngine.data(),
                                                                 testFileUrl(QLatin1String("rect.qml"))));
    QVERIFY(!qmlComponent->isLoading());
    if (qmlComponent->isError()) {
        for (const QQmlError &error : qmlComponent->errors())
            qWarning() << error.url() << error.line() << error;
    }
    QVERIFY(!qmlComponent->isError());

    QObject *rootObject = qmlComponent->create();
    if (qmlComponent->isError()) {
        for (const QQmlError &error : qmlComponent->errors())
            qWarning() << error.url() << error.line() << error;
    }
    QVERIFY(!qmlComponent->isError());

    QQuickItem *rootItem = qobject_cast<QQuickItem *>(rootObject);
    QVERIFY(rootItem);
    static const QSize ITEM_SIZE = QSize(200, 200);
    QCOMPARE(rootItem->size(), ITEM_SIZE);

    quickWindow->contentItem()->setSize(rootItem->size());
    quickWindow->setGeometry(0, 0, rootItem->width(), rootItem->height());

    rootItem->setParentItem(quickWindow->contentItem());

    const bool initSuccess = renderControl->initialize();

    // now we cannot just test for initSuccess; it is highly likely that a
    // number of configurations will simply fail in a CI environment (Vulkan,
    // Metal, ...) So the only reasonable choice is to skip if initialize()
    // failed. The exception for now is OpenGL - that should (usually) work.
    if (!initSuccess) {
#if QT_CONFIG(opengl)
        if (api != QSGRendererInterface::OpenGLRhi
                || !QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::OpenGL))
#endif
        {
            QSKIP("Could not initialize graphics, perhaps unsupported graphics API, skipping");
        }
    }

    QVERIFY(initSuccess);

    QCOMPARE(quickWindow->rendererInterface()->graphicsApi(), api);

    QRhi *rhi = renderControl->rhi();
    Q_ASSERT(rhi);

    const QSize size = rootItem->size().toSize();
    QScopedPointer<QRhiTexture> tex(rhi->newTexture(QRhiTexture::RGBA8, size, 1,
                                                    QRhiTexture::RenderTarget | QRhiTexture::UsedAsTransferSource));
    QVERIFY(tex->create());

    QScopedPointer<QRhiRenderBuffer> ds(rhi->newRenderBuffer(QRhiRenderBuffer::DepthStencil, size, 1));
    QVERIFY(ds->create());

    QRhiTextureRenderTargetDescription rtDesc(QRhiColorAttachment(tex.data()));
    rtDesc.setDepthStencilBuffer(ds.data());
    QScopedPointer<QRhiTextureRenderTarget> texRt(rhi->newTextureRenderTarget(rtDesc));
    QScopedPointer<QRhiRenderPassDescriptor> rp(texRt->newCompatibleRenderPassDescriptor());
    texRt->setRenderPassDescriptor(rp.data());
    QVERIFY(texRt->create());

    // redirect Qt Quick rendering into our texture
    quickWindow->setRenderTarget(QQuickRenderTarget::fromRhiRenderTarget(texRt.data()));

    QSize currentSize = size;

    for (int frame = 0; frame < 100; ++frame) {
        // QTBUG-88761 - change the render target size at some point and verify the renderer continues to function
        if (frame == 80) {
            currentSize -= QSize(2, 2); // small enough change to not bother the pixel verification checks
            tex->setPixelSize(currentSize);
            QVERIFY(tex->create()); // internally we now have a whole new native texture object
            ds->setPixelSize(currentSize);
            QVERIFY(ds->create());

            // Starting from Qt 6.3 we need neither a texRt->create() nor a
            // quickWindow->setRenderTarget() here. Just recreating the
            // internal native objects behing tex and ds should (must!) be
            // functional on its own.

        } else if (frame == 85) {
            // like the above but now change the size radically so that we can
            // test that rendering (viewports etc.) is corect.
            currentSize = QSize(100, 100);
            tex->setPixelSize(currentSize);
            QVERIFY(tex->create());
            ds->setPixelSize(currentSize);
            QVERIFY(ds->create());
        } else if (frame == 86) {
            // reset to the default size
            currentSize = ITEM_SIZE;
            tex->setPixelSize(currentSize);
            QVERIFY(tex->create());
            ds->setPixelSize(currentSize);
            QVERIFY(ds->create());
        } else if (frame == 90) {
            // Go berserk, destroy and recreate the texture and related stuff
            // (the QRhi objects themselves, not just the native stuff
            // internally), it should still work.
            currentSize -= QSize(2, 2); // chip off another 2 pixels
            tex.reset(rhi->newTexture(QRhiTexture::RGBA8, currentSize, 1,
                                      QRhiTexture::RenderTarget | QRhiTexture::UsedAsTransferSource));
            QVERIFY(tex->create());
            ds.reset(rhi->newRenderBuffer(QRhiRenderBuffer::DepthStencil, currentSize, 1));
            QVERIFY(ds->create());
            rtDesc = QRhiTextureRenderTargetDescription(QRhiColorAttachment(tex.data()));
            rtDesc.setDepthStencilBuffer(ds.data());
            texRt.reset(rhi->newTextureRenderTarget(rtDesc));
            rp.reset(texRt->newCompatibleRenderPassDescriptor());
            texRt->setRenderPassDescriptor(rp.data());
            QVERIFY(texRt->create());
            quickWindow->setRenderTarget(QQuickRenderTarget::fromRhiRenderTarget(texRt.data()));
        }

        // have to process events, e.g. to get queued metacalls delivered
        QCoreApplication::processEvents();

        if (frame > 0) {
            // Quick animations will now think that ANIM_ADVANCE_PER_FRAME milliseconds have passed,
            // even though in reality we have a tight loop that generates frames unthrottled.
            animDriver->advance();
        }

        renderControl->polishItems();

        // kick off the next frame on the QRhi (this internally calls QRhi::beginOffscreenFrame())
        renderControl->beginFrame();

        renderControl->sync();
        renderControl->render();

        bool readCompleted = false;
        QRhiReadbackResult readResult;
        QImage result;
        readResult.completed = [&readCompleted, &readResult, &result, &rhi] {
            readCompleted = true;
            QImage wrapperImage(reinterpret_cast<const uchar *>(readResult.data.constData()),
                                readResult.pixelSize.width(), readResult.pixelSize.height(),
                                QImage::Format_RGBA8888_Premultiplied);
            if (rhi->isYUpInFramebuffer())
                result = wrapperImage.mirrored();
            else
                result = wrapperImage.copy();
        };
        QRhiResourceUpdateBatch *readbackBatch = rhi->nextResourceUpdateBatch();
        readbackBatch->readBackTexture(tex.data(), &readResult);
        renderControl->commandBuffer()->resourceUpdate(readbackBatch);

        // our frame is done, submit
        renderControl->endFrame();

        // offscreen frames in QRhi are synchronous, meaning the readback has
        // been finished at this point
        QVERIFY(readCompleted);

        QImage img = result;
        QVERIFY(!img.isNull());
        QCOMPARE(img.size(), currentSize);

        const int maxFuzz = 2;

        // The scene is: background, rectangle, text
        // where rectangle rotates

        QRgb background = img.pixel(5, 5);
        QVERIFY(qAbs(qRed(background) - 70) < maxFuzz);
        QVERIFY(qAbs(qGreen(background) - 130) < maxFuzz);
        QVERIFY(qAbs(qBlue(background) - 180) < maxFuzz);

        // Frame 85 is where we resize to (100, 100), skip for that but from 86
        // we will back to the proper (200, 200). If failures occur from frame
        // 85 or - more likely - 86, that is likely because the
        // scenegraph/QQuickWindow does not correctly pick up the render target
        // size changes.
        if (frame != 85) {
            background = img.pixel(195, 195);
            QVERIFY(qAbs(qRed(background) - 70) < maxFuzz);
            QVERIFY(qAbs(qGreen(background) - 130) < maxFuzz);
            QVERIFY(qAbs(qBlue(background) - 180) < maxFuzz);

            // after about 1.25 seconds (animation time, one iteration is 16 ms
            // thanks to our custom animation driver) the rectangle reaches a 90
            // degree rotation, that should be frame 76
            if (frame <= 2 || (frame >= 76 && frame <= 80)) {
                QRgb c = img.pixel(28, 28); // rectangle
                QVERIFY(qAbs(qRed(c) - 152) < maxFuzz);
                QVERIFY(qAbs(qGreen(c) - 251) < maxFuzz);
                QVERIFY(qAbs(qBlue(c) - 152) < maxFuzz);
            } else {
                QRgb c = img.pixel(28, 28); // background because rectangle got rotated so this pixel is not covered by it
                QVERIFY(qAbs(qRed(c) - 70) < maxFuzz);
                QVERIFY(qAbs(qGreen(c) - 130) < maxFuzz);
                QVERIFY(qAbs(qBlue(c) - 180) < maxFuzz);
            }
        }
    }
}

void tst_RenderControl::renderAndReadBackWithVulkanNative()
{
#if QT_CONFIG(vulkan)
    if (!vulkanInstance.isValid())
        QSKIP("Skipping native Vulkan test due to failing to create a VkInstance");

    QQuickWindow::setGraphicsApi(QSGRendererInterface::VulkanRhi);

    // We will create our own VkDevice and friends, which will then get used by
    // Qt Quick as well (instead of creating its own objects), so this is a test
    // of a typical "integrate Qt Quick content into an external (Vulkan-based)
    // rendering engine" case.

    QVulkanFunctions *f = vulkanInstance.functions();

    uint32_t physDevCount = 0;
    f->vkEnumeratePhysicalDevices(vulkanInstance.vkInstance(), &physDevCount, nullptr);
    if (!physDevCount)
        QSKIP("No Vulkan physical devices");

    QVarLengthArray<VkPhysicalDevice, 4> physDevs(physDevCount);
    VkResult err = f->vkEnumeratePhysicalDevices(vulkanInstance.vkInstance(), &physDevCount, physDevs.data());
    QVERIFY(err == VK_SUCCESS);
    QVERIFY(physDevCount);

    // Just use the first physical device for now.
    VkPhysicalDevice physDev = physDevs[0];

    uint32_t queueCount = 0;
    f->vkGetPhysicalDeviceQueueFamilyProperties(physDev, &queueCount, nullptr);
    QVarLengthArray<VkQueueFamilyProperties, 4> queueFamilyProps(queueCount);
    f->vkGetPhysicalDeviceQueueFamilyProperties(physDev, &queueCount, queueFamilyProps.data());

    int gfxQueueFamilyIdx = -1;
    for (int i = 0; i < queueFamilyProps.size(); ++i) {
        if (queueFamilyProps[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            gfxQueueFamilyIdx = i;
            break;
        }
    }
    QVERIFY(gfxQueueFamilyIdx >= 0);

    VkDeviceQueueCreateInfo queueInfo[2] = {};
    const float prio[] = { 0 };
    queueInfo[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueInfo[0].queueFamilyIndex = uint32_t(gfxQueueFamilyIdx);
    queueInfo[0].queueCount = 1;
    queueInfo[0].pQueuePriorities = prio;

    VkDevice dev = VK_NULL_HANDLE;
    VkDeviceCreateInfo devInfo = {};
    devInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    devInfo.queueCreateInfoCount = 1;
    devInfo.pQueueCreateInfos = queueInfo;
    err = f->vkCreateDevice(physDev, &devInfo, nullptr, &dev);
    if (err != VK_SUCCESS || !dev)
        QSKIP("Skipping Vulkan test due to failing to create VkDevice");

    QVulkanDeviceFunctions *df = vulkanInstance.deviceFunctions(dev);
    QVERIFY(df);

    {
        QScopedPointer<QQuickRenderControl> renderControl(new QQuickRenderControl);
        QScopedPointer<QQuickWindow> quickWindow(new QQuickWindow(renderControl.data()));
        quickWindow->setVulkanInstance(&vulkanInstance);

        QScopedPointer<QQmlEngine> qmlEngine(new QQmlEngine);
        QScopedPointer<QQmlComponent> qmlComponent(new QQmlComponent(qmlEngine.data(),
                                                                     testFileUrl(QLatin1String("rect.qml"))));
        QVERIFY(!qmlComponent->isLoading());
        if (qmlComponent->isError()) {
            for (const QQmlError &error : qmlComponent->errors())
                qWarning() << error.url() << error.line() << error;
        }
        QVERIFY(!qmlComponent->isError());

        QObject *rootObject = qmlComponent->create();
        if (qmlComponent->isError()) {
            for (const QQmlError &error : qmlComponent->errors())
                qWarning() << error.url() << error.line() << error;
        }
        QVERIFY(!qmlComponent->isError());

        QQuickItem *rootItem = qobject_cast<QQuickItem *>(rootObject);
        QVERIFY(rootItem);
        QCOMPARE(rootItem->size(), QSize(200, 200));

        // avoid trouble with the image - buffer copy later on
        QVERIFY(int(rootItem->width()) % 4 == 0);
        QVERIFY(int(rootItem->height()) % 4 == 0);

        quickWindow->contentItem()->setSize(rootItem->size());
        quickWindow->setGeometry(0, 0, rootItem->width(), rootItem->height());

        rootItem->setParentItem(quickWindow->contentItem());

        // Let Qt Quick and the underlying QRhi "adopt" our VkDevice, which
        // will conveniently mean resource handles (buffers, images) are valid
        // both there and here in our native Vulkan code as we all use the same
        // device.
        quickWindow->setGraphicsDevice(QQuickGraphicsDevice::fromDeviceObjects(physDev, dev, gfxQueueFamilyIdx));

        const bool initSuccess = renderControl->initialize();
        QVERIFY(initSuccess);
        QCOMPARE(quickWindow->rendererInterface()->graphicsApi(), QSGRendererInterface::VulkanRhi);

        // Will need a command pool/buffer to do the readback.
        VkCommandPool cmdPool = VK_NULL_HANDLE;
        VkCommandPoolCreateInfo poolInfo = {};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.queueFamilyIndex = uint32_t(gfxQueueFamilyIdx);
        VkResult err = df->vkCreateCommandPool(dev, &poolInfo, nullptr, &cmdPool);
        QCOMPARE(err, VK_SUCCESS);

        // Get a command queue, this is the same as what Qt Quick (QRhi) uses.
        VkQueue cmdQueue = VK_NULL_HANDLE;
        df->vkGetDeviceQueue(dev, uint32_t(gfxQueueFamilyIdx), 0, &cmdQueue);

        // Do some sanity checks
        QCOMPARE(physDev, *reinterpret_cast<VkPhysicalDevice *>(quickWindow->rendererInterface()->getResource(
                                                                    quickWindow.data(), QSGRendererInterface::PhysicalDeviceResource)));
        QCOMPARE(dev, *reinterpret_cast<VkDevice *>(quickWindow->rendererInterface()->getResource(
                                                        quickWindow.data(), QSGRendererInterface::DeviceResource)));
        QCOMPARE(cmdQueue, *reinterpret_cast<VkQueue *>(quickWindow->rendererInterface()->getResource(
                                                            quickWindow.data(), QSGRendererInterface::CommandQueueResource)));

        // Create the VkImage into which Qt Quick should render its contents.

        VkImage img = VK_NULL_HANDLE;
        VkImageCreateInfo imgInfo = {};
        imgInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imgInfo.imageType = VK_IMAGE_TYPE_2D;
        imgInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
        imgInfo.extent.width = uint32_t(rootItem->width());
        imgInfo.extent.height = uint32_t(rootItem->height());
        imgInfo.extent.depth = 1;
        imgInfo.mipLevels = imgInfo.arrayLayers = 1;
        imgInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imgInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imgInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        imgInfo.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;

        err = df->vkCreateImage(dev, &imgInfo, nullptr, &img);
        QCOMPARE(err, VK_SUCCESS);

        VkPhysicalDeviceMemoryProperties memProps;
        f->vkGetPhysicalDeviceMemoryProperties(physDev, &memProps);

        auto findMemTypeIndex = [&memProps](uint32_t wantedBits, const VkMemoryRequirements &memReqs) {
            uint32_t memTypeIndex = 0;
            for (uint32_t i = 0; i < memProps.memoryTypeCount; ++i) {
                if (memReqs.memoryTypeBits & (1 << i)) {
                    if ((memProps.memoryTypes[i].propertyFlags & wantedBits) == wantedBits) {
                        memTypeIndex = i;
                        break;
                    }
                }
            }
            return memTypeIndex;
        };

        VkMemoryRequirements memReq;
        df->vkGetImageMemoryRequirements(dev, img, &memReq);

        VkMemoryAllocateInfo memInfo = {};
        memInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        memInfo.allocationSize = memReq.size;
        memInfo.memoryTypeIndex = findMemTypeIndex(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, memReq);

        VkDeviceMemory imgMem = VK_NULL_HANDLE;
        err = df->vkAllocateMemory(dev, &memInfo, nullptr, &imgMem);
        QCOMPARE(err, VK_SUCCESS);

        err = df->vkBindImageMemory(dev, img, imgMem, 0);
        QCOMPARE(err, VK_SUCCESS);

        // Tell Qt Quick to target our VkImage.
        quickWindow->setRenderTarget(QQuickRenderTarget::fromVulkanImage(img,
                                                                         VK_IMAGE_LAYOUT_PREINITIALIZED,
                                                                         rootItem->size().toSize()));

        // Create a readback buffer.
        VkBuffer buf = VK_NULL_HANDLE;
        VkDeviceMemory bufMem = VK_NULL_HANDLE;
        VkBufferCreateInfo bufInfo = {};
        bufInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        const int bufSize = int(rootItem->width()) * int(rootItem->height()) * 4;
        bufInfo.size = bufSize;

        df->vkCreateBuffer(dev, &bufInfo, nullptr, &buf);
        df->vkGetBufferMemoryRequirements(dev, buf, &memReq);
        memInfo.allocationSize = memReq.size;
        memInfo.memoryTypeIndex = findMemTypeIndex(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, memReq);
        err = df->vkAllocateMemory(dev, &memInfo, nullptr, &bufMem);
        QCOMPARE(err, VK_SUCCESS);
        df->vkBindBufferMemory(dev, buf, bufMem, 0);

        for (int frame = 0; frame < 100; ++frame) {
            // have to process events, e.g. to get queued metacalls delivered
            QCoreApplication::processEvents();

            if (frame > 0) {
                // Quick animations will now think that ANIM_ADVANCE_PER_FRAME milliseconds have passed,
                // even though in reality we have a tight loop that generates frames unthrottled.
                animDriver->advance();
            }

            renderControl->polishItems();

            renderControl->beginFrame();
            renderControl->sync();
            renderControl->render();
            renderControl->endFrame(); // submits the command buffer generated by Qt Quick to the command queue
            // ...and, it also waits for completion. This is different from how an on-screen frame would behave,
            // offscreen frames are always synchronous with QRhi. Which is very handy for us here.

            // Now issue a readback.

            VkCommandBuffer cb = VK_NULL_HANDLE;
            VkCommandBufferAllocateInfo cmdBufInfo = {};
            cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            cmdBufInfo.commandPool = cmdPool;
            cmdBufInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            cmdBufInfo.commandBufferCount = 1;

            VkResult err = df->vkAllocateCommandBuffers(dev, &cmdBufInfo, &cb);
            QCOMPARE(err, VK_SUCCESS);

            VkCommandBufferBeginInfo cmdBufBeginInfo = {};
            cmdBufBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

            err = df->vkBeginCommandBuffer(cb, &cmdBufBeginInfo);
            QCOMPARE(err, VK_SUCCESS);

            // rendering into a VkImage with Qt Quick leaves it in COLOR_ATTACHMENT_OPTIMAL
            VkImageMemoryBarrier barrier = {};
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
            barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
            barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            barrier.image = img;

            df->vkCmdPipelineBarrier(cb, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                                     0, 0, nullptr, 0, nullptr,
                                     1, &barrier);

            VkBufferImageCopy copyDesc = {};
            copyDesc.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            copyDesc.imageSubresource.layerCount = 1;
            copyDesc.imageExtent.width = uint32_t(rootItem->width());
            copyDesc.imageExtent.height = uint32_t(rootItem->height());
            copyDesc.imageExtent.depth = 1;

            df->vkCmdCopyImageToBuffer(cb, img, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, buf, 1, &copyDesc);

            // Must restore the previous layout since nothing is telling Qt
            // here that the layout changed so it will expect it to still be in
            // COLOR_ATTACHMENT_OPTIMAL in the next iteration.
            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

            df->vkCmdPipelineBarrier(cb, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                     0, 0, nullptr, 0, nullptr,
                                     1, &barrier);

            err = df->vkEndCommandBuffer(cb);
            QCOMPARE(err, VK_SUCCESS);

            VkSubmitInfo submitInfo = {};
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = &cb;
            VkPipelineStageFlags psf = VK_PIPELINE_STAGE_TRANSFER_BIT;
            submitInfo.pWaitDstStageMask = &psf;

            err = df->vkQueueSubmit(cmdQueue, 1, &submitInfo, VK_NULL_HANDLE);
            QCOMPARE(err, VK_SUCCESS);

            // just block until the image-to-buffer-copy result is available
            df->vkQueueWaitIdle(cmdQueue);

            df->vkFreeCommandBuffers(dev, cmdPool, 1, &cb);

            uchar *p = nullptr;
            df->vkMapMemory(dev, bufMem, 0, bufSize, 0, reinterpret_cast<void **>(&p));
            // create a wrapper QImage
            QImage img(reinterpret_cast<const uchar *>(p), rootItem->width(), rootItem->height(), QImage::Format_RGBA8888_Premultiplied);

            // and the usual verification:

            const int maxFuzz = 2;

            // The scene is: background, rectangle, text
            // where rectangle rotates

            QRgb background = img.pixel(5, 5);
            QVERIFY(qAbs(qRed(background) - 70) < maxFuzz);
            QVERIFY(qAbs(qGreen(background) - 130) < maxFuzz);
            QVERIFY(qAbs(qBlue(background) - 180) < maxFuzz);

            background = img.pixel(195, 195);
            QVERIFY(qAbs(qRed(background) - 70) < maxFuzz);
            QVERIFY(qAbs(qGreen(background) - 130) < maxFuzz);
            QVERIFY(qAbs(qBlue(background) - 180) < maxFuzz);

            // after about 1.25 seconds (animation time, one iteration is 16 ms
            // thanks to our custom animation driver) the rectangle reaches a 90
            // degree rotation, that should be frame 76
            if (frame <= 2 || (frame >= 76 && frame <= 80)) {
                QRgb c = img.pixel(28, 28); // rectangle
                QVERIFY(qAbs(qRed(c) - 152) < maxFuzz);
                QVERIFY(qAbs(qGreen(c) - 251) < maxFuzz);
                QVERIFY(qAbs(qBlue(c) - 152) < maxFuzz);
            } else {
                QRgb c = img.pixel(28, 28); // background because rectangle got rotated so this pixel is not covered by it
                QVERIFY(qAbs(qRed(c) - 70) < maxFuzz);
                QVERIFY(qAbs(qGreen(c) - 130) < maxFuzz);
                QVERIFY(qAbs(qBlue(c) - 180) < maxFuzz);
            }

            img = QImage();
            df->vkUnmapMemory(dev, bufMem);
        }

        df->vkDestroyImage(dev, img, nullptr);
        df->vkFreeMemory(dev, imgMem, nullptr);

        df->vkDestroyBuffer(dev, buf, nullptr);
        df->vkFreeMemory(dev, bufMem, nullptr);

        df->vkDestroyCommandPool(dev, cmdPool, nullptr);
    }

    // now that everything is destroyed, get rid of the VkDevice too
    df->vkDestroyDevice(dev, nullptr);
    vulkanInstance.resetDeviceFunctions(dev);
#else
    QSKIP("No Vulkan support in Qt build, skipping native Vulkan test");
#endif
}

#include "tst_qquickrendercontrol.moc"

QTEST_MAIN(tst_RenderControl)
