// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtCore/qurl.h>
#include "qquickvectorimage_p.h"
#include "qquickvectorimage_p_p.h"
#include <QtQuickVectorImageGenerator/private/qquickitemgenerator_p.h>
#include <QtQuickVectorImageGenerator/private/qquickvectorimageglobal_p.h>
#include <QtQuickVectorImageGenerator/private/qquickvectorimageplugin_p.h>
#include <QtCore/qloggingcategory.h>

#include <private/qquicktranslate_p.h>

#include <private/qfactoryloader_p.h>

QT_BEGIN_NAMESPACE

Q_GLOBAL_STATIC_WITH_ARGS(QFactoryLoader, vectorImagePluginLoader,
                          (QQuickVectorImageFormatsPluginFactory_iid,
                           QLatin1String("/vectorimageformats"),
                           Qt::CaseInsensitive))


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

    It currently supports the \c SVG file format. In addition, Lottie support can be enabled by
    setting the \l{assumeTrustedSource} property to true and including the plugin from the
    \l{Qt Lottie Animation} module.

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

    QUrl resolvedUrl = qmlContext(q)->resolvedUrl(sourceFile);
    QString localFile = QQmlFile::urlToLocalFileOrQrc(resolvedUrl);

    if (localFile.isEmpty())
        return;

    if (rootItem)
        rootItem->deleteLater();

    rootItem = new QQuickItem(q);
    rootItem->setParentItem(q);

    QQuickVectorImageGenerator::GeneratorFlags flags;
    if (preferredRendererType == QQuickVectorImage::CurveRenderer)
        flags.setFlag(QQuickVectorImageGenerator::CurveRenderer);
    if (assumeTrustedSource)
        flags.setFlag(QQuickVectorImageGenerator::AssumeTrustedSource);

    if (!m_qmlContext || m_qmlContext->engine() != qmlContext(q)->engine())
        m_qmlContext.reset(new QQmlContext(qmlContext(q)->engine()));

    QQuickItemGenerator generator(localFile, flags, rootItem, m_qmlContext.get());

    // If we assume trusted source, we try plugins first
    bool generatedWithPlugin = false;
    if (assumeTrustedSource) {
        QFactoryLoader *loader = vectorImagePluginLoader();

        const qsizetype count = loader->keyMap().size();
        for (qsizetype i = 0; i <= count && !generatedWithPlugin; ++i) {
            QQuickVectorImagePlugin *plugin = qobject_cast<QQuickVectorImagePlugin *>(loader->instance(i));
            if (plugin != nullptr)
                generatedWithPlugin = plugin->generate(localFile, &generator);
        }
    }

    if (!generatedWithPlugin)
        generator.generate();

    q->setImplicitWidth(rootItem->width());
    q->setImplicitHeight(rootItem->height());

    q->updateAnimationProperties();
    q->updateRootItemScale();
    q->update();
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
        childItem->setProperty("loops", d->animations->loops());
        childItem->setProperty("paused", d->animations->paused());
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
