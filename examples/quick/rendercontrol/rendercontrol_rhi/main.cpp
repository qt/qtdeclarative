// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QMainWindow>
#include <QApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMenuBar>
#include <QStatusBar>
#include <QFileDialog>
#include <QMessageBox>
#include <QLabel>
#include <QScrollArea>
#include <QGridLayout>
#include <QPushButton>
#include <QSlider>
#include <QCheckBox>
#include <QScrollBar>
#include <QListWidget>
#include <QPainter>

#include <QQuickWindow>
#include <QQuickRenderControl>
#include <QQuickRenderTarget>
#include <QQuickGraphicsDevice>
#include <QQuickGraphicsConfiguration>
#include <QQuickItem>
#include <QQmlEngine>
#include <QQmlComponent>

#include <QAnimationDriver>
#include <QElapsedTimer>

#include <rhi/qrhi.h>

//! [anim-driver]
class AnimationDriver : public QAnimationDriver
{
public:
    AnimationDriver(QObject *parent = nullptr)
        : QAnimationDriver(parent),
          m_step(16)
    {
    }

    void setStep(int milliseconds)
    {
        m_step = milliseconds;
    }

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
//! [anim-driver]

class ImageLabel;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow();
    ~MainWindow();

    void load(const QString &filename);
    void render();

private slots:
    void openRequested();

private:
    void reset();
    void stepAnimations();

    AnimationDriver *m_animationDriver = nullptr;

    std::unique_ptr<QQuickRenderControl> m_renderControl;
    std::unique_ptr<QQuickWindow> m_scene;
    std::unique_ptr<QQmlEngine> m_qmlEngine;
    std::unique_ptr<QQmlComponent> m_qmlComponent;

    std::unique_ptr<QRhiTexture> m_texture;
    std::unique_ptr<QRhiRenderBuffer> m_ds;
    std::unique_ptr<QRhiTextureRenderTarget> m_rt;
    std::unique_ptr<QRhiRenderPassDescriptor> m_rpDesc;

    qint64 m_frameCount = 0;
    QSize m_thumbnailSize;
    QVector<double> m_cpuTimes;
    QVector<double> m_gpuTimes;

    QLabel *m_apiMsg;
    QLabel *m_statusMsg;
    QLabel *m_avgCpuMsg;
    QLabel *m_avgGpuMsg;
    QLabel *m_driverInfoMsg;

    QScrollArea *m_scrollArea;
    QGridLayout *m_grid;
    QVector<QWidget *> m_gridWidgets;

public:
#if QT_CONFIG(vulkan)
    QVulkanInstance *m_vulkanInstance = nullptr;
#endif

    ImageLabel *m_focus = nullptr;
    QLabel *m_fullSizeViewerWindow = nullptr;
    QVector<QImage> m_frames;

    bool m_mirrorVertically = false;
};

MainWindow::MainWindow()
{
    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *vlayout = new QVBoxLayout(centralWidget);
    QHBoxLayout *controlLayout = new QHBoxLayout;
    vlayout->addLayout(controlLayout);

    QPushButton *btn = new QPushButton(tr("Next frame"));
    QObject::connect(btn, &QPushButton::clicked, btn, [this] {
        render();
    });
    controlLayout->addWidget(btn);

    QPushButton *btnMulti = new QPushButton(tr("Next 10 frames"));
    QObject::connect(btnMulti, &QPushButton::clicked, btn, [this] {
        for (int i = 0; i < 10; ++i)
            QMetaObject::invokeMethod(this, &MainWindow::render, Qt::QueuedConnection);
    });
    controlLayout->addWidget(btnMulti);

//! [anim-slider]
    QSlider *animSlider = new QSlider;
    animSlider->setOrientation(Qt::Horizontal);
    animSlider->setMinimum(1);
    animSlider->setMaximum(1000);
    QLabel *animLabel = new QLabel;
    QObject::connect(animSlider, &QSlider::valueChanged, animSlider, [this, animLabel, animSlider] {
        if (m_animationDriver)
            m_animationDriver->setStep(animSlider->value());
        animLabel->setText(tr("Simulated elapsed time per frame: %1 ms").arg(animSlider->value()));
    });
    animSlider->setValue(16);
    QCheckBox *animCheckBox = new QCheckBox(tr("Custom animation driver"));
    animCheckBox->setToolTip(tr("Note: Installing the custom animation driver makes widget drawing unreliable, depending on the platform.\n"
                                "This is due to widgets themselves relying on QPropertyAnimation and similar, which are driven by the same QAnimationDriver.\n"
                                "In any case, the functionality of the widgets are not affected, just the rendering may lag behind.\n"
                                "When not checked, Qt Quick animations advance based on the system time, i.e. the time elapsed since the last press of the Next button."));
    QObject::connect(animCheckBox, &QCheckBox::stateChanged, animCheckBox, [this, animCheckBox, animSlider, animLabel] {
        if (animCheckBox->isChecked()) {
            animSlider->setEnabled(true);
            animLabel->setEnabled(true);
            m_animationDriver = new AnimationDriver(this);
            m_animationDriver->install();
            m_animationDriver->setStep(animSlider->value());
        } else {
            animSlider->setEnabled(false);
            animLabel->setEnabled(false);
            delete m_animationDriver;
            m_animationDriver = nullptr;
        }
    });
    animSlider->setEnabled(false);
    animLabel->setEnabled(false);
    controlLayout->addWidget(animCheckBox);
    controlLayout->addWidget(animLabel);
    controlLayout->addWidget(animSlider);
//! [anim-slider]

    QCheckBox *mirrorCheckBox = new QCheckBox(tr("Mirror vertically"));
    QObject::connect(mirrorCheckBox, &QCheckBox::stateChanged, mirrorCheckBox, [this, mirrorCheckBox] {
        m_mirrorVertically = mirrorCheckBox->isChecked();
    });
    controlLayout->addWidget(mirrorCheckBox);

    QGridLayout *gridLayout = new QGridLayout;
    vlayout->addLayout(gridLayout);

    QWidget *viewport = new QWidget;
    m_grid = new QGridLayout(viewport);
    m_scrollArea = new QScrollArea;
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setWidget(viewport);
    QObject::connect(m_scrollArea->verticalScrollBar(), &QScrollBar::rangeChanged, this, [this] {
        m_scrollArea->verticalScrollBar()->setSliderPosition(m_scrollArea->verticalScrollBar()->maximum());
    });

    gridLayout->addWidget(m_scrollArea);
    setCentralWidget(centralWidget);

    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(tr("&Open"), this, &MainWindow::openRequested);
    fileMenu->addAction(tr("E&xit"), qApp, &QCoreApplication::quit);

    m_statusMsg = new QLabel;
    statusBar()->addWidget(m_statusMsg);
    m_avgCpuMsg = new QLabel;
    statusBar()->addWidget(m_avgCpuMsg);
    m_avgGpuMsg = new QLabel;
    statusBar()->addWidget(m_avgGpuMsg);
    m_apiMsg = new QLabel;
    statusBar()->addWidget(m_apiMsg);
    m_driverInfoMsg = new QLabel;
    statusBar()->addWidget(m_driverInfoMsg);
}

MainWindow::~MainWindow()
{
    delete m_fullSizeViewerWindow;
}

void MainWindow::openRequested()
{
    QString filename = QFileDialog::getOpenFileName(this, tr("Open file"), QString(), tr("QML files (*.qml);;All files (*.*)"));
    if (!filename.isEmpty())
        load(filename);
}

void MainWindow::reset()
{
    m_rpDesc.reset();
    m_rt.reset();
    m_ds.reset();
    m_texture.reset();

    m_qmlComponent.reset();
    m_qmlEngine.reset();
    m_scene.reset();
    m_renderControl.reset();

    m_apiMsg->setText(QString());
    m_statusMsg->setText(QString());
    m_avgCpuMsg->setText(QString());
    m_avgGpuMsg->setText(QString());
    m_driverInfoMsg->setText(QString());

    m_frameCount = 0;

    qDeleteAll(m_gridWidgets);
    m_gridWidgets.clear();

    delete m_fullSizeViewerWindow;
    m_fullSizeViewerWindow = nullptr;
    m_focus = nullptr;
}

//! [load-1]
void MainWindow::load(const QString &filename)
{
    reset();

    m_renderControl.reset(new QQuickRenderControl);
    m_scene.reset(new QQuickWindow(m_renderControl.get()));

    // enable lastCompletedGpuTime() on QRhiCommandBuffer, if supported by the underlying 3D API
    QQuickGraphicsConfiguration config;
    config.setTimestamps(true);
    m_scene->setGraphicsConfiguration(config);

#if QT_CONFIG(vulkan)
    if (m_scene->graphicsApi() == QSGRendererInterface::Vulkan)
        m_scene->setVulkanInstance(m_vulkanInstance);
#endif

    m_qmlEngine.reset(new QQmlEngine);
    m_qmlComponent.reset(new QQmlComponent(m_qmlEngine.get(), QUrl::fromLocalFile(filename)));
    if (m_qmlComponent->isError()) {
        for (const QQmlError &error : m_qmlComponent->errors())
            qWarning() << error.url() << error.line() << error;
        QMessageBox::critical(this, tr("Cannot load QML scene"), tr("Failed to load %1").arg(filename));
        reset();
        return;
    }
//! [load-1]

//! [load-instantiate]
    QObject *rootObject = m_qmlComponent->create();
    if (m_qmlComponent->isError()) {
        for (const QQmlError &error : m_qmlComponent->errors())
            qWarning() << error.url() << error.line() << error;
        QMessageBox::critical(this, tr("Cannot load QML scene"), tr("Failed to create component"));
        reset();
        return;
    }

    QQuickItem *rootItem = qobject_cast<QQuickItem *>(rootObject);
    if (!rootItem) {
        // Get rid of the on-screen window, if the root object was a Window
        if (QQuickWindow *w = qobject_cast<QQuickWindow *>(rootObject))
            delete w;
        QMessageBox::critical(this,
                              tr("Invalid root item in QML scene"),
                              tr("Root object is not a QQuickItem. If this is a scene with Window in it, note that such scenes are not supported."));
        reset();
        return;
    }

    if (rootItem->size().width() < 16)
        rootItem->setSize(QSizeF(640, 360));

    m_scene->contentItem()->setSize(rootItem->size());
    m_scene->setGeometry(0, 0, rootItem->width(), rootItem->height());

    rootItem->setParentItem(m_scene->contentItem());

    m_statusMsg->setText(tr("QML scene loaded"));
//! [load-instantiate]

//! [load-graphicsinit]
    const bool initSuccess = m_renderControl->initialize();
    if (!initSuccess) {
        QMessageBox::critical(this, tr("Cannot initialize renderer"), tr("QQuickRenderControl::initialize() failed"));
        reset();
        return;
    }

    const QSGRendererInterface::GraphicsApi api = m_scene->rendererInterface()->graphicsApi();
    switch (api) {
    case QSGRendererInterface::OpenGL:
        m_apiMsg->setText(tr("OpenGL"));
        break;
    case QSGRendererInterface::Direct3D11:
        m_apiMsg->setText(tr("D3D11"));
        break;
    case QSGRendererInterface::Direct3D12:
        m_apiMsg->setText(tr("D3D12"));
        break;
    case QSGRendererInterface::Vulkan:
        m_apiMsg->setText(tr("Vulkan"));
        break;
    case QSGRendererInterface::Metal:
        m_apiMsg->setText(tr("Metal"));
        break;
    default:
        m_apiMsg->setText(tr("Unknown 3D API"));
        break;
    }

    QRhi *rhi = m_renderControl->rhi();
    if (!rhi) {
        QMessageBox::critical(this, tr("Cannot render"), tr("No QRhi from QQuickRenderControl"));
        reset();
        return;
    }

    m_driverInfoMsg->setText(QString::fromUtf8(rhi->driverInfo().deviceName));
//! [load-graphicsinit]

//! [texture-setup]
    const QSize pixelSize = rootItem->size().toSize(); // no scaling, i.e. the item size is in pixels

    m_texture.reset(rhi->newTexture(QRhiTexture::RGBA8, pixelSize, 1,
                                    QRhiTexture::RenderTarget | QRhiTexture::UsedAsTransferSource));
    if (!m_texture->create()) {
        QMessageBox::critical(this, tr("Cannot render"), tr("Cannot create texture object"));
        reset();
        return;
    }

    m_ds.reset(rhi->newRenderBuffer(QRhiRenderBuffer::DepthStencil, pixelSize, 1));
    if (!m_ds->create()) {
        QMessageBox::critical(this, tr("Cannot render"), tr("Cannot create depth-stencil buffer"));
        reset();
        return;
    }

    QRhiTextureRenderTargetDescription rtDesc(QRhiColorAttachment(m_texture.get()));
    rtDesc.setDepthStencilBuffer(m_ds.get());
    m_rt.reset(rhi->newTextureRenderTarget(rtDesc));
    m_rpDesc.reset(m_rt->newCompatibleRenderPassDescriptor());
    m_rt->setRenderPassDescriptor(m_rpDesc.get());
    if (!m_rt->create()) {
        QMessageBox::critical(this, tr("Cannot render"), tr("Cannot create render target"));
        reset();
        return;
    }

    m_scene->setRenderTarget(QQuickRenderTarget::fromRhiRenderTarget(m_rt.get()));
//! [texture-setup]

    render();
}

class ImageLabel : public QLabel
{
public:
    ImageLabel(MainWindow *mw, int frameIndex)
        : m_mainWindow(mw),
          m_frameIndex(frameIndex)
    {
    }

    void mousePressEvent(QMouseEvent *) override {
        if (m_mainWindow->m_focus != this) {
            ImageLabel *oldFocus = m_mainWindow->m_focus;
            m_mainWindow->m_focus = this;
            if (oldFocus)
                oldFocus->update();
            update();
        }
    }

    void paintEvent(QPaintEvent *e) override {

        if (m_mainWindow->m_focus == this) {
            QPainter p(this);
            p.fillRect(0, 0, width(), height(), Qt::blue);
            p.end();
            QLabel::paintEvent(e);
            p.begin(this);
            p.setOpacity(0.5);
            p.fillRect(0, 0, width(), height(), Qt::blue);
        } else {
            QLabel::paintEvent(e);
        }
    }

    void mouseDoubleClickEvent(QMouseEvent *) override {
        QLabel *&w(m_mainWindow->m_fullSizeViewerWindow);
        if (!w)
            w = new QLabel;
        const QImage &image(m_mainWindow->m_frames[m_frameIndex]);
        w->resize(image.width(), image.height());
        w->setPixmap(QPixmap::fromImage(image));
        w->show();
        w->activateWindow();
    }

    MainWindow *m_mainWindow;
    int m_frameIndex;
};

void MainWindow::render()
{
    if (!m_renderControl)
        return;

    if (m_frameCount > 0)
        stepAnimations();

    // this is only here to communicate the possibly changed mirrorVertically flag
    QQuickRenderTarget quickRt = QQuickRenderTarget::fromRhiRenderTarget(m_rt.get());
    quickRt.setMirrorVertically(m_mirrorVertically);
    m_scene->setRenderTarget(quickRt);

//! [render-core]
    QElapsedTimer cpuTimer;
    cpuTimer.start();

    m_renderControl->polishItems();

    m_renderControl->beginFrame();

    m_renderControl->sync();
    m_renderControl->render();

    QRhi *rhi = m_renderControl->rhi();
    QRhiReadbackResult readResult;
    QRhiResourceUpdateBatch *readbackBatch = rhi->nextResourceUpdateBatch();
    readbackBatch->readBackTexture(m_texture.get(), &readResult);
    m_renderControl->commandBuffer()->resourceUpdate(readbackBatch);

    m_renderControl->endFrame();

    const double gpuRenderTimeMs = m_renderControl->commandBuffer()->lastCompletedGpuTime() * 1000.0;
    const double cpuRenderTimeMs = cpuTimer.nsecsElapsed() / 1000000.0;

    // m_renderControl->begin/endFrame() is based on QRhi's
    // begin/endOffscreenFrame() under the hood, meaning it does not do
    // pipelining, unlike swapchain-based frames, and therefore the readback is
    // guaranteed to complete once endFrame() returns.
    QImage wrapperImage(reinterpret_cast<const uchar *>(readResult.data.constData()),
                    readResult.pixelSize.width(), readResult.pixelSize.height(),
                    QImage::Format_RGBA8888_Premultiplied);
    QImage result;
    if (rhi->isYUpInFramebuffer())
        result = wrapperImage.mirrored();
    else
        result = wrapperImage.copy();

//! [render-core]

    m_gpuTimes.append(gpuRenderTimeMs);
    m_cpuTimes.append(cpuRenderTimeMs);

    m_frames.append(result);

    if (m_thumbnailSize.isEmpty())
        m_thumbnailSize = size() / 4;

    const QImage thumbnail = result.scaled(m_thumbnailSize, Qt::KeepAspectRatio);

    ImageLabel *image = new ImageLabel(this, m_frames.count() - 1);
    image->setPixmap(QPixmap::fromImage(thumbnail));
    image->setAlignment(Qt::AlignCenter);

    QLabel *label = new QLabel(tr("Frame %1\nCPU: %2 ms GPU: %3 ms").arg(m_frameCount).arg(cpuRenderTimeMs, 0, 'f', 4).arg(gpuRenderTimeMs, 0, 'f', 4));
    label->setAlignment(Qt::AlignCenter);
    QWidget *container = new QWidget;
    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(image);
    layout->addWidget(label);
    container->setLayout(layout);
    m_grid->addWidget(container, m_frameCount / 3, m_frameCount % 3);
    m_gridWidgets.append(container);

    m_scrollArea->verticalScrollBar()->setSliderPosition(m_scrollArea->verticalScrollBar()->maximum());

    m_frameCount += 1;

    double v = 0;
    for (double t : m_cpuTimes)
        v += t;
    v /= m_cpuTimes.count();
    m_avgCpuMsg->setText(tr("Avg. CPU render time: %1 ms").arg(v, 0, 'f', 4));
    if (m_cpuTimes.count() > 64) {
        m_cpuTimes.clear();
        m_cpuTimes.append(v);
    }
    v = 0;
    for (double t : m_gpuTimes)
        v += t;
    v /= m_gpuTimes.count();
    m_avgGpuMsg->setText(tr("Avg. GPU render time: %1 ms").arg(v, 0, 'f', 4));
    if (m_gpuTimes.count() > 64) {
        m_gpuTimes.clear();
        m_gpuTimes.append(v);
    }
}

//! [anim-step]
void MainWindow::stepAnimations()
{
    if (m_animationDriver) {
        // Now the Qt Quick scene will think that <slider value> milliseconds have
        // elapsed and update animations accordingly when doing the next frame.
        m_animationDriver->advance();
    }
}
//! [anim-step]

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

#if QT_CONFIG(vulkan)
    QVulkanInstance vulkanInstance;
#endif

    MainWindow mainWindow;

//! [apiselect]
    QDialog apiSelect;
    QVBoxLayout *selLayout = new QVBoxLayout;
    selLayout->addWidget(new QLabel(QObject::tr("Select graphics API to use")));
    QListWidget *apiList = new QListWidget;
    QVarLengthArray<QSGRendererInterface::GraphicsApi, 5> apiValues;
#ifdef Q_OS_WIN
    apiList->addItem("Direct3D 11");
    apiValues.append(QSGRendererInterface::Direct3D11);
    apiList->addItem("Direct3D 12");
    apiValues.append(QSGRendererInterface::Direct3D12);
#endif
#if defined(Q_OS_MACOS) || defined(Q_OS_IOS)
    apiList->addItem("Metal");
    apiValues.append(QSGRendererInterface::Metal);
#endif
#if QT_CONFIG(vulkan)
    apiList->addItem("Vulkan");
    apiValues.append(QSGRendererInterface::Vulkan);
#endif
#if QT_CONFIG(opengl)
    apiList->addItem("OpenGL / OpenGL ES");
    apiValues.append(QSGRendererInterface::OpenGL);
#endif
    if (apiValues.isEmpty()) {
        QMessageBox::critical(nullptr, QObject::tr("No 3D graphics API"), QObject::tr("No 3D graphics APIs are supported in this Qt build"));
        return 1;
    }
//! [apiselect]
    apiList->setCurrentRow(0);
    selLayout->addWidget(apiList);
    QPushButton *okBtn = new QPushButton("Ok");
    okBtn->setDefault(true);
    selLayout->addWidget(okBtn);
    apiSelect.setLayout(selLayout);
    apiSelect.resize(320, 200);
    apiSelect.show();

    QObject::connect(okBtn, &QPushButton::clicked, okBtn, [apiList, &apiSelect, &apiValues, &mainWindow
#if QT_CONFIG(vulkan)
                                                           , &vulkanInstance
#endif
    ] {
        const QSGRendererInterface::GraphicsApi api = apiValues[apiList->currentRow()];
        QQuickWindow::setGraphicsApi(api);

#if QT_CONFIG(vulkan)
        if (api == QSGRendererInterface::Vulkan) {
            vulkanInstance.setExtensions(QQuickGraphicsConfiguration::preferredInstanceExtensions());
            if (!vulkanInstance.create()) {
                QMessageBox::critical(nullptr, QObject::tr("Cannot initialize Vulkan"), QObject::tr("Failed to create VkInstance"));
                return;
            }
            mainWindow.m_vulkanInstance = &vulkanInstance;
        }
#endif

        mainWindow.resize(1280, 720);
        mainWindow.show();
        mainWindow.load(QLatin1String(":/demo.qml"));
        // load() renders one frame, add 19 more
        for (int i = 1; i <= 19; ++i) {
            mainWindow.render();
            // have to process events, e.g. to get queued metacalls delivered
            QCoreApplication::processEvents();
        }

        apiSelect.close();
    });

    return app.exec();
}

#include "main.moc"
