/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Quick Templates 2 module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
******************************************************************************/

#ifndef QTQUICKTEMPLATES2GLOBAL_P_H
#define QTQUICKTEMPLATES2GLOBAL_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qglobal.h>
#include <QtQml/private/qqmlglobal_p.h>
#include <QtQuickTemplates2/private/qtquicktemplates2-config_p.h>

QT_BEGIN_NAMESPACE

#ifndef QT_STATIC
#  if defined(QT_BUILD_QUICKTEMPLATES2_LIB)
#    define Q_QUICKTEMPLATES2_PRIVATE_EXPORT Q_DECL_EXPORT
#  else
#    define Q_QUICKTEMPLATES2_PRIVATE_EXPORT Q_DECL_IMPORT
#  endif
#else
#  define Q_QUICKTEMPLATES2_PRIVATE_EXPORT
#endif

Q_QUICKTEMPLATES2_PRIVATE_EXPORT void QQuickTemplates_initializeModule();

QT_END_NAMESPACE

Q_QUICKTEMPLATES2_PRIVATE_EXPORT void qml_register_types_QtQuick_Templates();

#endif // QTQUICKTEMPLATES2GLOBAL_P_H
