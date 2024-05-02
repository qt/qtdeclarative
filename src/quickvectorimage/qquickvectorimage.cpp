// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickvectorimage_p.h"
#include "qquickvectorimage_p_p.h"
#include <QtQuickVectorImageGenerator/private/qquickitemgenerator_p.h>
#include <QtQuickVectorImageGenerator/private/qquickvectorimageglobal_p.h>
#include <QtCore/qloggingcategory.h>

QT_BEGIN_NAMESPACE

/*!
    \qmlmodule QtQuick.VectorImage
    \title Qt Quick Vector Image QML Types
    \ingroup qmlmodules
    \brief Provides QML types for displaying vector image files.

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
    flags.setFlag(QQuickVectorImageGenerator::CurveRenderer);
    QQuickItemGenerator generator(localFile, flags, svgItem);
    generator.generate();

    svgItem->setParentItem(q);
    q->setImplicitWidth(svgItem->width());
    q->setImplicitHeight(svgItem->height());
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

QT_END_NAMESPACE
