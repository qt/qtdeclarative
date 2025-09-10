// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickvectorimage_p.h"
#include "qquickvectorimage_p_p.h"
#include <QtQuickVectorImageGenerator/private/qquickitemgenerator_p.h>
#include <QtQuickVectorImageGenerator/private/qquickvectorimageglobal_p.h>
#include <QtCore/qloggingcategory.h>

#include <private/qquicktranslate_p.h>

QT_BEGIN_NAMESPACE

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

    It currently supports the \c SVG file format.

    Qt supports multiple options for displaying SVG files. For an overview and comparison of the
    different ones, see the documentation of the \l{svgtoqml} tool.

    \section1 QML Types
*/

Q_LOGGING_CATEGORY(lcQuickVectorImage, "qt.quick.vectorimage", QtWarningMsg)

void QQuickVectorImagePrivate::setSource(const QUrl &source)
{
    Q_Q(QQuickVectorImage);
    if (sourceFile == source)
        return;

    sourceFile = source;
    loadSvg();
    emit q->sourceChanged();
}

void QQuickVectorImagePrivate::loadSvg()
{
    Q_Q(QQuickVectorImage);

    QUrl resolvedUrl = qmlContext(q)->resolvedUrl(sourceFile);
    QString localFile = QQmlFile::urlToLocalFileOrQrc(resolvedUrl);

    if (localFile.isEmpty())
        return;

    QQuickVectorImagePrivate::Format fileFormat = formatFromFilePath(localFile);

    if (fileFormat != QQuickVectorImagePrivate::Format::Svg) {
        qCWarning(lcQuickVectorImage) << "Unsupported file format";
        return;
    }

    if (svgItem)
        svgItem->deleteLater();

    svgItem = new QQuickItem(q);

    QQuickVectorImageGenerator::GeneratorFlags flags;
    if (preferredRendererType == QQuickVectorImage::CurveRenderer)
        flags.setFlag(QQuickVectorImageGenerator::CurveRenderer);
    QQuickItemGenerator generator(localFile, flags, svgItem);
    generator.generate();

    svgItem->setParentItem(q);
    q->setImplicitWidth(svgItem->width());
    q->setImplicitHeight(svgItem->height());

    q->updateSvgItemScale();

    q->update();
}

QQuickVectorImagePrivate::Format QQuickVectorImagePrivate::formatFromFilePath(const QString &filePath)
{
    Q_UNUSED(filePath)

    QQuickVectorImagePrivate::Format res = QQuickVectorImagePrivate::Format::Unknown;

    if (filePath.endsWith(QLatin1String(".svg")) || filePath.endsWith(QLatin1String(".svgz"))
        || filePath.endsWith(QLatin1String(".svg.gz"))) {
        res = QQuickVectorImagePrivate::Format::Svg;
    }

    return res;
}

/*!
    \qmltype VectorImage
    \inqmlmodule QtQuick.VectorImage
    \inherits Item
    \brief Loads a vector image file and displays it in a Qt Quick scene.
    \since 6.8

    The VectorImage can be used to load a vector image file and display this as an item in a Qt
    Quick scene. It currently supports the \c SVG file format.

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

    QObject::connect(this, &QQuickItem::widthChanged, this, &QQuickVectorImage::updateSvgItemScale);
    QObject::connect(this, &QQuickItem::heightChanged, this, &QQuickVectorImage::updateSvgItemScale);
    QObject::connect(this, &QQuickVectorImage::fillModeChanged, this, &QQuickVectorImage::updateSvgItemScale);
}

/*!
    \qmlproperty url QtQuick.VectorImage::VectorImage::source

    This property holds the URL of the vector image file to load.

    VectorImage currently only supports the \c SVG file format.
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

void QQuickVectorImage::updateSvgItemScale()
{
    Q_D(QQuickVectorImage);

    if (d->svgItem == nullptr
        || qFuzzyIsNull(d->svgItem->width())
        || qFuzzyIsNull(d->svgItem->height())) {
        return;
    }

    auto xformProp = d->svgItem->transform();
    QQuickScale *scaleTransform = nullptr;
    if (xformProp.count(&xformProp) == 0) {
        scaleTransform = new QQuickScale;
        scaleTransform->setParent(d->svgItem);
        xformProp.append(&xformProp, scaleTransform);
    } else {
        scaleTransform = qobject_cast<QQuickScale *>(xformProp.at(&xformProp, 0));
    }

    if (scaleTransform != nullptr) {
        qreal xScale = width() / d->svgItem->width();
        qreal yScale = height() / d->svgItem->height();

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
    d->loadSvg();
    emit preferredRendererTypeChanged();
}

QT_END_NAMESPACE
