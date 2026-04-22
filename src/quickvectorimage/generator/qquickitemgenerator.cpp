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
                                         QQuickVectorImageGenerator::GeneratorFlags flags)
    : QQuickQmlGenerator(fileName, flags, QString{})
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

QT_END_NAMESPACE
