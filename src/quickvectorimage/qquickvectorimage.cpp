// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtCore/qurl.h>
#include <QtCore/QScopeGuard>
#include "qquickvectorimage_p.h"
#include "qquickvectorimage_p_p.h"
#include "qquickvectorimageincubator_p.h"
#include "qquickvectorimageplugin_p.h"
#include <QtQuickVectorImageGenerator/private/qquickitemgenerator_p.h>
#include <QtQuickVectorImageGenerator/private/qquickvectorimageglobal_p.h>
#include <QtQuickVectorImageGenerator/private/qquickanimationrootitem_p.h>
#include <QtCore/private/qfactoryloader_p.h>
#include <QtCore/qloggingcategory.h>

#include <private/qquicktranslate_p.h>
#include <private/qquickanimation_p.h>

QT_BEGIN_NAMESPACE

Q_GLOBAL_STATIC_WITH_ARGS(QFactoryLoader, itemGenPluginLoader,
                          (QQuickVectorImageFormatsPluginFactory_iid,
                           QLatin1String("/vectorimageformats"), Qt::CaseInsensitive))

static bool useQmlGenerator()
{
    static const bool val = !qEnvironmentVariableIsSet("QT_QUICKVECTORIMAGE_USE_ITEM_GENERATOR");
    return val;
}

/*!
    \qmlmodule QtQuick.VectorImage
    \title Qt Quick Vector Image QML Types
    \ingroup qmlmodules
    \brief Provides QML types for displaying vector image files.
    \since 6.8

    To use the types in this module, import the module with the following line:

    \qml
    import QtQuick.VectorImage
    \endqml

    Qt Quick Vector Image provides support for displaying vector image files in a Qt Quick
    scene.

    It currently supports the \c SVG file format. In addition, Lottie support
    can be enabled by setting the
    \l{QtQuick.VectorImage::VectorImage::}{assumeTrustedSource} property to true
    and including the plugin from the \l{Qt Lottie Animation} module.

    Qt supports multiple options for displaying SVG files. For an overview and comparison of
    the different ones, see the documentation of the \l{svgtoqml} tool.

    \section1 QML Types
*/

void QQuickVectorImagePrivate::setSource(const QUrl &source)
{
    Q_Q(QQuickVectorImage);
    if (sourceFile == source)
        return;

    sourceFile = source;
    loadFile();
    emit q->sourceChanged();
}

void QQuickVectorImagePrivate::loadFile()
{
    Q_Q(QQuickVectorImage);

    if (!q->isComponentComplete())
        return;

    QQmlContext *ctx = qmlContext(q);
    QUrl resolvedUrl = ctx->resolvedUrl(sourceFile);
    QString localFile = QQmlFile::urlToLocalFileOrQrc(resolvedUrl);

    if (localFile.isEmpty())
        return;

    if (rootItem && !retainWhileLoading) {
        rootItem->deleteLater();
        rootItem = nullptr;
        emit q->generatedItemChanged();
    }

    if (incubator != nullptr) {
        // If the incubator is still alive, it means it was interrupted before we could add the
        // object to the parent item.
        QObject *obj = incubator->object();
        delete obj;

        incubator->disconnect(q);
        incubator->deleteLater();
    }

    if (pendingRootItem != nullptr) {
        delete pendingRootItem;
        pendingRootItem = nullptr;
    }

    QQuickVectorImageGenerator::GeneratorFlags flags;
    if (preferredRendererType == QQuickVectorImage::CurveRenderer)
        flags.setFlag(QQuickVectorImageGenerator::CurveRenderer);
    if (assumeTrustedSource)
        flags.setFlag(QQuickVectorImageGenerator::AssumeTrustedSource);
    if (m_asyncShapes)
        flags.setFlag(QQuickVectorImageGenerator::AsyncShapes);
    if (asynchronous)
        flags.setFlag(QQuickVectorImageGenerator::AsynchronousLoading);

    if (useQmlGenerator()) {
        QQmlIncubator::IncubationMode mode =
                asynchronous ? QQmlIncubator::Asynchronous : QQmlIncubator::Synchronous;

        if (!context || context->engine() != qmlContext(q)->engine())
            context.reset(new QQmlContext(qmlContext(q)->engine()));

        incubator = new QQuickVectorImageIncubator(mode, context.get(), q);
        QObject::connect(incubator, &QQuickVectorImageIncubator::statusUpdated, q,
                         &QQuickVectorImage::statusChanged);
        QObject::connect(incubator, &QQuickVectorImageIncubator::statusUpdated, q,
                         &QQuickVectorImage::updateItem);
        incubator->start(localFile, flags);
    } else {
        QQuickItemGenerator gen(localFile, flags, qmlContext(q));

        bool generatedWithPlugin = false;
        if (flags.testFlag(QQuickVectorImageGenerator::AssumeTrustedSource)) {
            QFactoryLoader *loader = itemGenPluginLoader();
            const qsizetype count = loader->keyMap().size();
            for (qsizetype i = 0; i < count && !generatedWithPlugin; ++i) {
                QQuickVectorImagePlugin *plugin =
                        qobject_cast<QQuickVectorImagePlugin *>(loader->instance(i));
                if (plugin != nullptr) {
                    std::unique_ptr<QQuickVectorImagePluginGenerator> pluginGen(
                            plugin->createGenerator(localFile));
                    if (pluginGen != nullptr)
                        generatedWithPlugin = pluginGen->generate(localFile, &gen);
                }
            }
        }

        if (!generatedWithPlugin)
            gen.generate();

        if (gen.errorState() == QQuickVectorImageGenerator::NoError) {
            pendingRootItem = gen.takeRootItem();
        } else {
            qCWarning(lcQuickVectorImage) << "QQuickItemGenerator: failed to generate" << localFile
                                          << "(errorState:" << gen.errorState() << ")";
        }
        q->updateItem();
        emit q->statusChanged();
    }
}

void QQuickVectorImage::updateItem()
{
    Q_D(QQuickVectorImage);

    QQuickItem *item = nullptr;
    if (d->incubator != nullptr) {
        if (d->incubator->object() == nullptr || !d->incubator->isReady())
            return;
        item = qobject_cast<QQuickItem *>(d->incubator->object());
        if (item == nullptr) {
            qCWarning(lcQuickVectorImage)
                    << "QQuickVectorImage::updateItem: Root item not a QQuickItem:"
                    << d->incubator->errors();
            return;
        }
    } else {
        item = d->pendingRootItem;
        d->pendingRootItem = nullptr;
        if (item == nullptr)
            return;
    }

    if (d->rootItem != nullptr)
        d->rootItem->deleteLater();

    d->rootItem = new QQuickItem(this);
    d->rootItem->setParentItem(this);
    auto emitter = qScopeGuard([&] { emit generatedItemChanged(); }); // emit at any function exit

    if (item->width() == 0 || item->height() == 0)
        return;

    d->rootItem->setImplicitWidth(item->width());
    d->rootItem->setImplicitHeight(item->height());

    item->setParent(d->rootItem);
    item->setParentItem(d->rootItem);

    setImplicitWidth(d->rootItem->width());
    setImplicitHeight(d->rootItem->height());

    updateAnimationProperties();
    updateRootItemScale();
    update();

    static int freezeTime = qEnvironmentVariableIntValue("QT_QUICKVECTORIMAGE_FREEZE");
    if (freezeTime != 0) {
        if (freezeTime < 0)
            freezeTime = 400; // TBD: calculate better default, e.g. midtime of total anim duration
        animations()->setPaused(true);
        const QList<QQuickAbstractAnimation *> anims = d->rootItem->findChildren<QQuickAbstractAnimation *>();
        for (QQuickAbstractAnimation *anim : anims) {
            if (anim->group() == nullptr)
                anim->setCurrentTime(freezeTime);
        }
    }

    if (d->incubator) {
        d->incubator->disconnect(this);
        d->incubator->deleteLater();
        d->incubator = nullptr;
    }
}

/*!
    \qmltype VectorImage
    \inqmlmodule QtQuick.VectorImage
    \inherits Item
    \brief Loads a vector image file and displays it in a Qt Quick scene.
    \since 6.8

    The VectorImage can be used to load a vector image file and display this as an item in a Qt
    Quick scene.

    It currently supports the \c SVG file format. In addition, Lottie support can be enabled by
    setting the \l{assumeTrustedSource} property to true and including the plugin from the
    \l{Qt Lottie Animation} module.

    \note This complements the approach of loading the vector image file through an \l Image
    element: \l Image creates a raster version of the image at the requested size. VectorImage
    builds a Qt Quick scene that represents the image. This means the resulting item can be scaled
    and rotated without losing quality, and it will typically consume less memory than the
    rasterized version.
*/
QQuickVectorImage::QQuickVectorImage(QQuickItem *parent)
    : QQuickItem(*(new QQuickVectorImagePrivate), parent)
{
    setFlag(QQuickItem::ItemHasContents, true);

    QObject::connect(this, &QQuickItem::widthChanged, this, &QQuickVectorImage::updateRootItemScale);
    QObject::connect(this, &QQuickItem::heightChanged, this, &QQuickVectorImage::updateRootItemScale);
    QObject::connect(this, &QQuickVectorImage::fillModeChanged, this, &QQuickVectorImage::updateRootItemScale);
}

QQuickVectorImage::~QQuickVectorImage()
{
    Q_D(QQuickVectorImage);
    // This may have a running thread, so we need to delete it before we start deleting children
    delete d->incubator;
    d->incubator = nullptr;
}

/*!
    \qmlproperty url QtQuick.VectorImage::VectorImage::source

    This property holds the URL of the vector image file to load.

    VectorImage currently supports the \c SVG file format. In addition, Lottie support can be
    enabled by setting the \l{assumeTrustedSource} property to true and including the plugin from
    the \l{Qt Lottie Animation} module.
*/
QUrl QQuickVectorImage::source() const
{
    Q_D(const QQuickVectorImage);
    return d->sourceFile;
}

void QQuickVectorImage::setSource(const QUrl &source)
{
    Q_D(QQuickVectorImage);
    d->setSource(source);
}

void QQuickVectorImage::updateRootItemScale()
{
    Q_D(QQuickVectorImage);

    if (d->rootItem == nullptr
        || qFuzzyIsNull(d->rootItem->width())
        || qFuzzyIsNull(d->rootItem->height())) {
        return;
    }

    auto xformProp = d->rootItem->transform();
    QQuickScale *scaleTransform = nullptr;
    if (xformProp.count(&xformProp) == 0) {
        scaleTransform = new QQuickScale;
        scaleTransform->setParent(d->rootItem);
        xformProp.append(&xformProp, scaleTransform);
    } else {
        scaleTransform = qobject_cast<QQuickScale *>(xformProp.at(&xformProp, 0));
    }

    if (scaleTransform != nullptr) {
        qreal xScale = width() / d->rootItem->width();
        qreal yScale = height() / d->rootItem->height();

        switch (d->fillMode) {
        case QQuickVectorImage::NoResize:
            xScale = yScale = 1.0;
            break;
        case QQuickVectorImage::PreserveAspectFit:
            xScale = yScale = qMin(xScale, yScale);
            break;
        case QQuickVectorImage::PreserveAspectCrop:
            xScale = yScale = qMax(xScale, yScale);
            break;
        case QQuickVectorImage::Stretch:
            // Already correct
            break;
        };

        scaleTransform->setXScale(xScale);
        scaleTransform->setYScale(yScale);
    }
}

void QQuickVectorImage::updateAnimationProperties()
{
    Q_D(QQuickVectorImage);
    if (Q_UNLIKELY(d->rootItem == nullptr || d->rootItem->childItems().isEmpty()))
        return;

    QQuickItem *childItem = d->rootItem->childItems().first();
    if (Q_LIKELY(d->animations != nullptr)) {
        if (auto *root = qobject_cast<QQuickAnimationRootItem *>(childItem)) {
            root->setLoops(d->animations->loops());
            root->setPaused(d->animations->paused());
        } else {
            childItem->setProperty("loops", d->animations->loops());
            childItem->setProperty("paused", d->animations->paused());
        }
    }
}

QQuickVectorImageAnimations *QQuickVectorImage::animations()
{
    Q_D(QQuickVectorImage);
    if (d->animations == nullptr) {
        d->animations = new QQuickVectorImageAnimations;
        QQml_setParent_noEvent(d->animations, this);
        QObject::connect(d->animations, &QQuickVectorImageAnimations::loopsChanged, this, &QQuickVectorImage::updateAnimationProperties);
        QObject::connect(d->animations, &QQuickVectorImageAnimations::pausedChanged, this, &QQuickVectorImage::updateAnimationProperties);
    }

    return d->animations;
}

/*!
    \qmlproperty enumeration QtQuick.VectorImage::VectorImage::fillMode

    This property defines what happens if the width and height of the VectorImage differs from
    the implicit size of its contents.

    \value VectorImage.NoResize             The contents are still rendered at the size provided by
                                            the input.
    \value VectorImage.Stretch              The contents are scaled to match the width and height of
                                            the \c{VectorImage}. (This is the default.)
    \value VectorImage.PreserveAspectFit    The contents are scaled to fit inside the bounds of the
                                            \c VectorImage, while preserving aspect ratio. The
                                            actual bounding rect of the contents will sometimes be
                                            smaller than the \c VectorImage item.
    \value VectorImage.PreserveAspectCrop   The contents are scaled to fill the \c VectorImage item,
                                            while preserving the aspect ratio. The actual bounds of
                                            the contents will sometimes be larger than the
                                            \c VectorImage item.
*/

QQuickVectorImage::FillMode QQuickVectorImage::fillMode() const
{
    Q_D(const QQuickVectorImage);
    return d->fillMode;
}

void QQuickVectorImage::setFillMode(FillMode newFillMode)
{
    Q_D(QQuickVectorImage);
    if (d->fillMode == newFillMode)
        return;
    d->fillMode = newFillMode;
    emit fillModeChanged();
}

/*!
    \qmlproperty enumeration QtQuick.VectorImage::VectorImage::preferredRendererType

    Requests a specific backend to use for rendering shapes in the \c VectorImage.

    \value VectorImage.GeometryRenderer Equivalent to Shape.GeometryRenderer. This backend flattens
    curves and triangulates the result. It will give aliased results unless multi-sampling is
    enabled, and curve flattening may be visible when the item is scaled.
    \value VectorImage.CurveRenderer Equivalent to Shape.CurveRenderer. With this backend, curves
    are rendered on the GPU and anti-aliasing is built in. Will typically give better visual
    results, but at some extra cost to performance.

    The default is \c{VectorImage.GeometryRenderer}.
*/

QQuickVectorImage::RendererType QQuickVectorImage::preferredRendererType() const
{
    Q_D(const QQuickVectorImage);
    return d->preferredRendererType;
}

void QQuickVectorImage::setPreferredRendererType(RendererType newPreferredRendererType)
{
    Q_D(QQuickVectorImage);
    if (d->preferredRendererType == newPreferredRendererType)
        return;
    d->preferredRendererType = newPreferredRendererType;
    d->loadFile();
    emit preferredRendererTypeChanged();
}

/*!
    \qmlproperty bool QtQuick.VectorImage::VectorImage::asynchronousShapes
    \since 6.11

    This property controls the {QtQuick.Shapes::Shape::asynchronous}{asynchronous} property of the
    \l Shape items in the Quick scene that VectorImage builds to represent the image.

    Setting this property to \c true will offload the CPU part of the rendering processing of the
    shapes to separate worker threads. This can improve CPU utilization and user interface
    responsiveness.

    By default this property is \c false.

    \sa asynchronous
*/

bool QQuickVectorImage::asynchronousShapes() const
{
    Q_D(const QQuickVectorImage);
    return d->m_asyncShapes;
}

void QQuickVectorImage::setAsynchronousShapes(bool asynchronous)
{
    Q_D(QQuickVectorImage);
    if (d->m_asyncShapes == asynchronous)
        return;
    d->m_asyncShapes = asynchronous;
    emit asynchronousShapesChanged();
}

/*!
    \qmlproperty bool QtQuick.VectorImage::VectorImage::asynchronous
    \since 6.12

    This property holds whether the image will be loaded asynchronously. When set to to \c true,
    the UI will remain reactive while the image is loading. The \l status property can be used
    to check the current progress.

    By default this property is \c false.

    \sa asynchronousShapes, status
*/

bool QQuickVectorImage::asynchronous() const
{
    Q_D(const QQuickVectorImage);
    return d->asynchronous;
}

void QQuickVectorImage::setAsynchronous(bool asynchronous)
{
    Q_D(QQuickVectorImage);
    if (d->asynchronous == asynchronous)
        return;
    d->asynchronous = asynchronous;
    emit asynchronousChanged();
}

/*!
    \qmlproperty bool QtQuick.VectorImage::VectorImage::retainWhileLoading
    \since 6.12

    This property defines the behavior when the \l source property is changed and loading happens
    asynchronously. This is the case when the \l asynchronous property is set to \c true.

    If \c retainWhileLoading is \c false (the default), the old image is discarded immediately, and
    the component is cleared while the new image is being loaded. If set to \c true, the old image
    is retained and remains visible until the new one is ready.

    Enabling this property can avoid flickering in cases where loading the new image takes a long
    time. It comes at the cost of some extra memory use while the new image is being loaded.

    \sa asynchronous
*/
bool QQuickVectorImage::retainWhileLoading() const
{
    Q_D(const QQuickVectorImage);
    return d->retainWhileLoading;
}

void QQuickVectorImage::setRetainWhileLoading(bool retainWhileLoading)
{
    Q_D(QQuickVectorImage);
    if (d->retainWhileLoading == retainWhileLoading)
        return;
    d->retainWhileLoading = retainWhileLoading;
    emit retainWhileLoadingChanged();
}

/*!
    \qmlproperty enumeration QtQuick.VectorImage::VectorImage::status
    \since 6.12

    This property holds the status of vector image loading. It can be one of:

    \value VectorImage.Null       No vector image has been set
    \value VectorImage.Ready      The vector image has been loaded
    \value VectorImage.Loading    The vector image is currently being loaded
    \value VectorImage.Error      An error occurred while loading the vector image
*/
QQuickVectorImage::Status QQuickVectorImage::status() const
{
    Q_D(const QQuickVectorImage);
    if (d->incubator == nullptr) {
        if (d->rootItem == nullptr)
            return Status::Null;
        else
            return Status::Ready;
    }

    switch (d->incubator->status()) {
    case QQmlIncubator::Null:
        return Status::Null;
    case QQmlIncubator::Loading:
        return Status::Loading;
    case QQmlIncubator::Error:
        return Status::Error;
    case QQmlIncubator::Ready:
        return Status::Ready;
    };

    return Status::Error;
}

/*!
    \qmlproperty bool QtQuick.VectorImage::VectorImage::assumeTrustedSource
    \since 6.10

    Setting this to true when loading trusted source files expands support for some features that
    may be unsafe in an uncontrolled setting. For SVG in particular, this maps to the
    \l{QtSvg::Option}{AssumeTrustedSource option}.

    When this is set to true, VectorImage will also try to load the image using the Lottie format
    plugin if this is available. See \l{Qt Lottie Animation} for additional information.

    By default this property is \c false.

    \sa svgtoqml, lottietoqml
 */

bool QQuickVectorImage::assumeTrustedSource() const
{
    Q_D(const QQuickVectorImage);
    return d->assumeTrustedSource;
}

void QQuickVectorImage::setAssumeTrustedSource(bool assumeTrustedSource)
{
    Q_D(QQuickVectorImage);
    if (d->assumeTrustedSource == assumeTrustedSource)
        return;
    d->assumeTrustedSource = assumeTrustedSource;
    d->loadFile();
    emit assumeTrustedSourceChanged();
}

/*!
    \qmlproperty Item QtQuick.VectorImage::VectorImage::generatedItem
    \since 6.12
    \readonly

    When a vector image file is loaded, this property holds the top level Item of the generated Qt
    Quick scene. When no file is loaded, this property is \c null.
*/

QQuickItem *QQuickVectorImage::generatedItem() const
{
    Q_D(const QQuickVectorImage);
    return d->rootItem ? d->rootItem->childItems().value(0) : nullptr;
}

void QQuickVectorImage::componentComplete()
{
    Q_D(QQuickVectorImage);
    QQuickItem::componentComplete();

    d->loadFile();
}

/*!
    \qmlpropertygroup QtQuick.VectorImage::VectorImage::animations
    \qmlproperty bool QtQuick.VectorImage::VectorImage::animations.paused
    \qmlproperty int QtQuick.VectorImage::VectorImage::animations.loops
    \since 6.10

    These properties can be used to control animations in the image, if it contains any.

    The \c paused property can be set to true to temporarily pause all animations. When the
    property is reset to \c false, the animations will resume where they were. By default this
    property is \c false.

    The \c loops property defines the number of times the animations in the document will repeat.
    By default this property is 1. Any animations that is set to loop indefinitely in the source
    image will be unaffected by this property. To make all animations in the document repeat
    indefinitely, the \c loops property can be set to \c{Animation.Infinite}.
*/
int QQuickVectorImageAnimations::loops() const
{
    return m_loops;
}

void QQuickVectorImageAnimations::setLoops(int loops)
{
    if (m_loops == loops)
        return;
    m_loops = loops;
    emit loopsChanged();
}

bool QQuickVectorImageAnimations::paused() const
{
    return m_paused;
}

void QQuickVectorImageAnimations::setPaused(bool paused)
{
    if (m_paused == paused)
        return;
    m_paused = paused;
    emit pausedChanged();
}

void QQuickVectorImageAnimations::restart()
{
    QQuickVectorImage *parentVectorImage = qobject_cast<QQuickVectorImage *>(parent());
    if (Q_UNLIKELY(parentVectorImage == nullptr)) {
        qCWarning(lcQuickVectorImage) << Q_FUNC_INFO << "Parent is not a VectorImage";
        return;
    }

    QQuickVectorImagePrivate *d = QQuickVectorImagePrivate::get(parentVectorImage);

    if (Q_UNLIKELY(d->rootItem == nullptr || d->rootItem->childItems().isEmpty()))
        return;

    QQuickItem *childItem = d->rootItem->childItems().first();
    QMetaObject::invokeMethod(childItem, "restart");
}

QT_END_NAMESPACE

#include <moc_qquickvectorimage_p.cpp>
