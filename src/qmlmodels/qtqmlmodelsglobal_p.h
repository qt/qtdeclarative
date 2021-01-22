/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
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
**
**
**
****************************************************************************/

#ifndef QTQMLMODELSGLOBAL_P_H
#define QTQMLMODELSGLOBAL_P_H

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

#include <QtQml/private/qtqmlglobal_p.h>
#include <QtQmlModels/qtqmlmodelsglobal.h>
#include <QtQmlModels/private/qtqmlmodels-config_p.h>

#define Q_QMLMODELS_PRIVATE_EXPORT Q_QMLMODELS_EXPORT
#define Q_QMLMODELS_AUTOTEST_EXPORT Q_AUTOTEST_EXPORT

void Q_QMLMODELS_PRIVATE_EXPORT qml_register_types_QtQml_Models();
GHS_KEEP_REFERENCE(qml_register_types_QtQml_Models);

#endif // QTQMLMODELSGLOBAL_P_H
