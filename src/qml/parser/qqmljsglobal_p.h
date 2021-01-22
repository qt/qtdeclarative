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
#ifndef QQMLJSGLOBAL_P_H
#define QQMLJSGLOBAL_P_H

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

#ifdef QT_CREATOR

#  ifdef QDECLARATIVEJS_BUILD_DIR
#    define QML_PARSER_EXPORT Q_DECL_EXPORT
#  elif QML_BUILD_STATIC_LIB
#    define QML_PARSER_EXPORT
#  else
#    define QML_PARSER_EXPORT Q_DECL_IMPORT
#  endif // QQMLJS_BUILD_DIR

#else // !QT_CREATOR
#  ifndef QT_STATIC
#    if defined(QT_BUILD_QMLDEVTOOLS_LIB) || defined(QT_QMLDEVTOOLS_LIB)
       // QmlDevTools is a static library
#      define QML_PARSER_EXPORT
#    elif defined(QT_BUILD_QML_LIB)
#      define QML_PARSER_EXPORT Q_DECL_EXPORT
#    else
#      define QML_PARSER_EXPORT Q_DECL_IMPORT
#    endif
#  else
#      define QML_PARSER_EXPORT
#  endif
#endif // QT_CREATOR

#endif // QQMLJSGLOBAL_P_H
