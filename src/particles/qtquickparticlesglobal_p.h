/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQuickParticles module of the Qt Toolkit.
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

#ifndef QTQUICKPARTICLESGLOBAL_P_H
#define QTQUICKPARTICLESGLOBAL_P_H

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

// We only have private exports from this library

#ifndef QT_STATIC
#  if defined(QT_BUILD_QUICKPARTICLES_LIB)
#    define Q_QUICKPARTICLES_PRIVATE_EXPORT Q_DECL_EXPORT
#  else
#    define Q_QUICKPARTICLES_PRIVATE_EXPORT Q_DECL_IMPORT
#  endif
#else
#  define Q_QUICKPARTICLES_PRIVATE_EXPORT
#endif

void Q_QUICKPARTICLES_PRIVATE_EXPORT qml_register_types_QtQuick_Particles();
GHS_KEEP_REFERENCE(qml_register_types_QtQuick_Particles);

#endif // QTQUICKPARTICLESGLOBAL_P_H
