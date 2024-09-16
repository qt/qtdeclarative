// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtCore/qcoreapplication.h>
#include <QtCore/qlibraryinfo.h>
#include <QtCore/qdebug.h>
#include <QtQml/qqmldebug.h>
#include <QtQml/qqmlengine.h>

int main(int argc, char *argv[])
{
      QQmlDebuggingEnabler::StartMode block = QQmlDebuggingEnabler::DoNotWaitForClient;
      int portFrom = 0;
      int portTo = 0;

      QCoreApplication app(argc, argv);
      QStringList arguments = app.arguments();
      arguments.removeFirst();
      QString connector = QLatin1String("QQmlDebugServer");

      if (arguments.size() && arguments.first() == QLatin1String("-block")) {
          block = QQmlDebuggingEnabler::WaitForClient;
          arguments.removeFirst();
      }

      if (arguments.size() >= 2 && arguments.first() == QLatin1String("-connector")) {
          arguments.removeFirst();
          connector = arguments.takeFirst();
      }

      if (arguments.size() >= 2) {
          portFrom = arguments.takeFirst().toInt();
          portTo = arguments.takeFirst().toInt();
      }

      if (arguments.size() && arguments.takeFirst() == QLatin1String("-services"))
          QQmlDebuggingEnabler::setServices(arguments);

      if (connector == QLatin1String("QQmlDebugServer")) {
          if (!portFrom || !portTo)
              qFatal("Port range has to be specified.");

          while (portFrom <= portTo)
              QQmlDebuggingEnabler::startTcpDebugServer(portFrom++, block);
      } else if (connector == QLatin1String("QQmlNativeDebugConnector")) {
          QVariantHash configuration;
          configuration[QLatin1String("block")] = (block == QQmlDebuggingEnabler::WaitForClient);
          QQmlDebuggingEnabler::startDebugConnector(connector, configuration);
      }

      QQmlEngine engine;
      qDebug() << "QQmlEngine created\n";
      Q_UNUSED(engine);
      return app.exec();
}

