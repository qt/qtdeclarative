// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickitemgenerator_p.h"
#include "utils_p.h"
#include "qquicknodeinfo_p.h"

#include <private/qsgcurveprocessor_p.h>
#include <private/qquickshape_p.h>
#include <private/qquadpath_p.h>
#include <private/qquickitem_p.h>
#include <private/qquickimagebase_p_p.h>
#include <private/qquickanimation_p.h>
#include <private/qquicktext_p.h>
#include <private/qquicktranslate_p.h>
#include <private/qquickimage_p.h>

#include <QtCore/qloggingcategory.h>
#include <QtCore/qstandardpaths.h>
#include <QtQml/qqmlcomponent.h>

QT_BEGIN_NAMESPACE

QQuickItemGenerator::QQuickItemGenerator(const QString fileName,
                                         QQuickVectorImageGenerator::GeneratorFlags flags,
                                         QQuickItem *parentItem,
                                         QQmlContext *ctx)
    : QQuickQmlGenerator(fileName, flags, QString{})
    , m_parentItem(parentItem)
    , m_qmlContext(ctx)
{
    setRetainFilePaths(true);

    QString tmpDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    setAssetFileDirectory(tmpDir);
    setAssetFilePrefix(QStringLiteral("_qt_vectorimage_"));
    setUrlPrefix(QStringLiteral("file:"));
}

QQuickItemGenerator::~QQuickItemGenerator()
{
}

bool QQuickItemGenerator::generateRootNode(const StructureNodeInfo &info)
{
    if (Q_UNLIKELY(errorState()))
        return false;

    bool cont = QQuickQmlGenerator::generateRootNode(info);
    if (info.stage == StructureNodeStage::End) {
        if (m_qmlContext == nullptr) {
            qCWarning(lcQuickVectorImage) << "QQuickItemGenerator::generateRootItem: Requires QML context";
            return false;
        }

        QQmlEngine *engine = m_qmlContext->engine();
        if (engine == nullptr) {
            qCWarning(lcQuickVectorImage) << "QQuickItemGenerator::generateRootItem: Requires QML engine";
            return false;
        }

        QQmlComponent component(engine);
        component.setData(m_result.data(), QUrl{});
        QObject *obj = component.create(m_qmlContext);
        QQuickItem *rootItem = qobject_cast<QQuickItem *>(obj);
        if (rootItem == nullptr) {
            delete obj;

            qCWarning(lcQuickVectorImage) << "QQuickItemGenerator::generateRootItem: Root item not a QQuickItem: " << component.errorString();
            return false;
        }

        m_parentItem->setImplicitWidth(rootItem->width());
        m_parentItem->setImplicitHeight(rootItem->height());

        rootItem->setParent(m_parentItem);
        rootItem->setParentItem(m_parentItem);
    }

    return cont;
}

QT_END_NAMESPACE
