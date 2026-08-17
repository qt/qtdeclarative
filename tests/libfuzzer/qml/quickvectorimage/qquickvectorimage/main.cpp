// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QGuiApplication>
#include <QtGlobal>
#include <QQmlEngine>
#include <QFont>
#include <QQuickWindow>
#include <QtQuickVectorImage/private/qquickvectorimage_p.h>
#include <QtQuickVectorImage/private/qquickvectorimage_p_p.h>
#include <QtGui/private/qfont_p.h>
#include <QFile>

#ifdef QT_ASAN_ENABLED
#  include <sanitizer/lsan_interface.h>
#else
#  define __lsan_disable()
#  define __lsan_enable()
#endif

// silence warnings
static QtMessageHandler mh = qInstallMessageHandler([](QtMsgType, const QMessageLogContext &,
                                                       const QString &) {});

// Driver may allocate caches or other persistent state, which we need to suppress.
// Add any false positive leaks here.
extern "C" const char *__lsan_default_suppressions()
{
    return "leak:libgallium\n"
           "leak:swrast\n"
           "leak:libGLX\n"
           "leak:libEGL\n"
           "leak:libfontconfig\n"
           "leak:swiftshader\n"
           "leak:libvulkan\n";
}

Q_GLOBAL_STATIC(std::shared_ptr<QGuiApplication>, guiApp)
Q_GLOBAL_STATIC(QQuickWindow, window)
Q_GLOBAL_STATIC(QQuickVectorImage, vectorImage)
Q_GLOBAL_STATIC(QQmlEngine, engine)

extern "C" int LLVMFuzzerInitialize(int *_argc, char ***_argv)
{
    Q_UNUSED(_argc);
    Q_UNUSED(_argv);

    // One-time setup allocations referenced only from the mmap'ed V4 GC heap
    // are invisible to LeakSanitizer, so ignore everything allocated here.
    __lsan_disable();

    static int argc = 1;
    static char arg1[] = "fuzzer";
    static char *argv[] = {arg1, nullptr};

    QHashSeed::setDeterministicGlobalSeed();
    (*guiApp()) = std::make_shared<QGuiApplication>(argc, argv);

    window()->resize(512, 512);
    window()->show();

    QQmlEngine::setContextForObject(vectorImage(), engine()->rootContext());
    vectorImage()->setParentItem(window()->contentItem());
    vectorImage()->setSize(window()->size());

    __lsan_enable();

    return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const char *Data, size_t Size)
{
    QByteArray data = QByteArray::fromRawData(Data, Size);

    QQuickVectorImagePrivate *d = QQuickVectorImagePrivate::get(vectorImage());
    d->setSourceData(data);
    window()->grabWindow(); // renders one frame
    d->setSourceData(QByteArray{});

    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QCoreApplication::processEvents();
    QFontCache::instance()->clear();
    return 0;
}
