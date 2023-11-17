// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSTYLEREADER_H
#define QSTYLEREADER_H

#include <QtCore>
#include <QMargins>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QDir>
#include <QPixmap>

#include <set>

#include "jsontools.h"
#include "bridge.h"

using namespace JsonTools;

struct ImageFormat
{
    ImageFormat(const QString &name)
        : name(name)
    {
        // Example names: png@2x, svg
        const int alphaIndex = name.indexOf('@');
        hasScale = alphaIndex != -1;
        if (hasScale) {
            format = name.first(alphaIndex);
            scale = name.sliced(alphaIndex + 1).chopped(1);
        } else {
            format = name;
            scale = "1";
        }
    }

    bool hasScale;
    QString name;
    QString format;
    QString scale;
};

struct Radii
{
    qreal topLeft = 0;
    qreal topRight = 0;
    qreal bottomLeft = 0;
    qreal bottomRight = 0;
};

struct BorderImageOffset
{
    int left = 0;
    int right = 0;
    int top = 0;
    int bottom = 0;
};

class StyleGenerator : public QObject
{
    Q_OBJECT

public:
    StyleGenerator(Bridge *bridge)
        : QObject(nullptr)
        , m_bridge(bridge)
    {
        try {
            readInputConfig();
            resolveGlobalConfig();
        } catch (std::exception &e) {
            error(e.what());
        }
    }

    Q_INVOKABLE void generateStyle()
    {
        try {
            progressTo(0);
            JsonTools::clearCache();
            if (!m_abort)
                downloadFigmaDocument();
            if (!m_abort)
                copyFiles();
            if (!m_abort)
                generateControls();
            if (!m_abort)
                generateIcons();
            if (!m_abort)
                downloadImages();
            progressTo(4);
            progressLabel("Generating configuration files");
            if (!m_abort)
                generateConfiguration();
            if (!m_abort)
                generateQmlDir();
            if (!m_abort)
                generateIndexThemeFile();
            if (!m_abort)
                generateQrcFile();
        } catch (std::exception &e) {
            error(e.what());
        }

        QThread::currentThread()->quit();
    }

    QStringList availableControls()
    {
        // This function returns a list over all the controls (and possibly other
        // items in the config file) that the user can choose from in order to tweak
        // what should be generated. m_bridge->m_selectedControls should be populated
        // with a subset of this list.
        QStringList controls;
        controls << m_controls;
        controls << m_defaultControls;
        controls << "Icons";
        std::sort(controls.begin(), controls.end());
        return controls;
    }

private:

    void downloadFigmaDocument()
    {
        progressLabel("Downloading figma file: " + m_bridge->m_fileId);
        QJsonDocument figmaResponsDoc;
        const QUrl url("https://api.figma.com/v1/files/" + m_bridge->m_fileId);
        debug("requesting: " + url.toString());

        QNetworkRequest request(url);
        request.setRawHeader(QByteArray("X-FIGMA-TOKEN"), m_bridge->m_figmaToken.toUtf8());
        debug("using token: " + m_bridge->m_figmaToken.toUtf8());

        QScopedPointer<QNetworkAccessManager> manager(new QNetworkAccessManager);
        manager->setAutoDeleteReplies(true);

        QNetworkReply *reply = manager->get(request);

        QObject::connect(reply, &QNetworkReply::finished, [this, &reply]{
            if (reply->error() != QNetworkReply::NoError)
                return;

            m_document = QJsonDocument::fromJson(reply->readAll());

            if (qgetenv("QSTYLEGENERATOR_SAVEDOC") == "true")
                saveForDebug(m_document.object(), "figmastyle.json");

            try {
                setFigmaFileName(getString("name", m_document.object()));
            } catch (std::exception &e) {
                warning("could not resolve name of design file: " + QString(e.what()));
            }
        });

        QObject::connect(reply, &QNetworkReply::downloadProgress, [this]{
            progress();
        });

        auto dispatcher = QThread::currentThread()->eventDispatcher();
        while (reply->isRunning() && !m_abort)
            dispatcher->processEvents(QEventLoop::AllEvents);

        if (reply->error() != QNetworkReply::NoError) {
            throw RestCallException(QStringLiteral("Could not download design file from Figma! ")
                + "Please check that your Figma token is valid and that the file ID '" + m_bridge->m_fileId
                + "' is correct! (error message: " + networkErrorString(reply) + ")");
        }
    }

    void resolveGlobalConfig()
    {
        const QJsonArray themesArray = m_inputConfig.value("themes").toArray();
        if (themesArray.isEmpty())
            throw std::runtime_error("The input config needs to list at least one theme!");
        for (const QJsonValue &themeValue : themesArray)
            m_themes.append(themeValue.toString());

        const QJsonArray imageFormats = m_inputConfig.value("image formats").toArray();
        for (const QJsonValue &formatValue : imageFormats)
            m_imageFormats.append(formatValue.toString());

        const QJsonArray exportArray = m_inputConfig.value("default export").toArray();
        for (const QJsonValue &exportValue : exportArray) {
            const QJsonObject exportObj = exportValue.toObject();
            const QString atom = getString("atom", exportObj);
            const QStringList exportList = getStringList("export", exportObj, true);
            m_defaultExport[atom] = exportList;
        }
        const QJsonArray controlsArray = m_inputConfig.value("controls").toArray();
        for (const QJsonValue &controlValue : controlsArray) {
            const QJsonObject controlObj = controlValue.toObject();
            const QString control = getString("name", controlObj);
            m_controls << control;
        }
        const QJsonArray defaulControlsArray = m_inputConfig.value("default controls").toArray();
        for (const QJsonValue &controlValue : defaulControlsArray) {
            const QJsonObject controlObj = controlValue.toObject();
            const QString control = getString("name", controlObj);
            m_defaultControls << control;
        }
    }

    QList<QJsonDocument> generateImageUrls(const ImageFormat &imageFormat)
    {
        const auto figmaIdToFileNameMap = m_imagesToDownload[imageFormat.name];
        const QStringList idsList = figmaIdToFileNameMap.keys();
        const QString format = imageFormat.format;
        const QString scale = imageFormat.scale;

        progressLabel("Downloading image urls with format " + imageFormat.name);

        QScopedPointer<QNetworkAccessManager> manager(new QNetworkAccessManager);
        manager->setAutoDeleteReplies(true);

        QNetworkRequest request;
        request.setRawHeader(QByteArray("X-FIGMA-TOKEN"), m_bridge->m_figmaToken.toUtf8());

        QList<QString> requestIds;
        QString currentIds;
        QString partialUrl = "https://api.figma.com/v1/images/" + m_bridge->m_fileId +
                             "?format=" + format + "&scale=" + scale + "&ids=";

        // REST API supports urls of up to 6000 chars so we need to split
        // into multiple requests if the total character length exceeds it
        const int maxIdsLength = 6000 - partialUrl.length();
        for (const QString& id : idsList) {
            if (currentIds.isEmpty()) {
                currentIds = id;
            } else if (currentIds.length() + id.length() + 1 <= maxIdsLength) {
                currentIds += "," + id;
            } else {
                requestIds.append(currentIds);
                currentIds = id;
            }
        }

        if (!currentIds.isEmpty())
            requestIds.append(currentIds);

        QList<QJsonDocument> responseDocuments;

        // Send network requests for each set of IDs
        for (const QString& ids : requestIds) {
            QUrl url(partialUrl + ids);
            debug("request: " + url.toString());
            request.setUrl(url);

            QNetworkReply* reply = manager->get(request);

            QObject::connect(reply, &QNetworkReply::finished, [reply, &responseDocuments] {
                if (reply->error() == QNetworkReply::NoError) {
                    QJsonDocument responseDoc = QJsonDocument::fromJson(reply->readAll());
                    responseDocuments.append(responseDoc);
                }
            });

            QObject::connect(reply, &QNetworkReply::downloadProgress, [this] {
                progress();
            });

            auto dispatcher = QThread::currentThread()->eventDispatcher();
            while (reply->isRunning() && !m_abort)
                dispatcher->processEvents(QEventLoop::AllEvents);

            if (reply->error() != QNetworkReply::NoError)
                throw RestCallException(QStringLiteral("Could not download images from Figma: ") + networkErrorString(reply));

            reply->deleteLater();
        }

        return responseDocuments;
    }

    void downloadImages(const ImageFormat &imageFormat, const QJsonDocument &figmaImagesResponsDoc)
    {
        const auto imageUrls = getObject("images", figmaImagesResponsDoc.object());
        const auto figmaIdToFileNameMap = m_imagesToDownload[imageFormat.name];

        progressLabel("Downloading images with format " + imageFormat.name);

        QScopedPointer<QNetworkAccessManager> manager(new QNetworkAccessManager);
        manager->setAutoDeleteReplies(true);
        int requestCount = imageUrls.keys().count();

        for (const QString &figmaId : imageUrls.keys()) {
            const QString imageUrl = imageUrls.value(figmaId).toString();
            if (imageUrl.isEmpty()) {
                // Figma doesn't create imageUrls for empty images (where nothing
                // was drawn). For those cases we need to clear the filePath in the
                // output config as well, so that QML doesn't complain about a missing images.
                const QString filePath = figmaIdToFileNameMap.value(figmaId);
                debug("no image URL generated for image: " + filePath + " (image probably empty)");

                const QString fileTheme = filePath.split('/').first();
                for (const QString &theme : std::as_const(m_outputConfig).keys()) {
                    if (theme.compare(fileTheme, Qt::CaseInsensitive) == 0) {
                        auto &config = m_outputConfig[theme];
                        const bool modified = JsonTools::modifyValue(figmaId, "filePath", "", config);
                        if (!modified)
                            warning("Could not clear filePath: " + filePath);
                        m_outputConfig[theme] = config;
                    }
                }

                requestCount--;
                continue;
            }

            QNetworkReply *reply = manager->get(QNetworkRequest(imageUrl));

            QObject::connect(reply, &QNetworkReply::finished,
                             [this, figmaId, imageUrl, imageFormat,
                             reply, &figmaIdToFileNameMap, &requestCount] {
                requestCount--;
                if (reply->error() != QNetworkReply::NoError) {
                    warning("Failed to download "
                        + imageUrl + " (id: " + figmaId + "). "
                        + "Error code:" + networkErrorString(reply));
                    return;
                }

                const QString filePath = m_bridge->m_targetDirectory + "/" + figmaIdToFileNameMap.value(figmaId);
                const QString targetDir = QFileInfo(filePath).absoluteDir().path();
                if (!QDir().mkpath(targetDir))
                    throw std::runtime_error("Could not create image directory: " + targetDir.toStdString());

                if (imageFormat.format == "svg") {
                    QFile file(filePath);
                    file.open(QIODevice::WriteOnly);
                    file.write(reply->readAll());
                } else {
                    QPixmap pixmap;
                    if (!pixmap.loadFromData(reply->readAll(), imageFormat.format.toUtf8().constData())) {
                        warning("Failed to create pixmap: "
                            + filePath + " from " + imageUrl);
                        return;
                    }
                    if (!pixmap.save(filePath)) {
                        warning("Failed to save pixmap: "
                            + filePath + " from " + imageUrl);
                        return;
                    }
                }
                debug("downloaded image: " + filePath + " from " + imageUrl);
                progress();
            });
        }

        auto dispatcher = QThread::currentThread()->eventDispatcher();
        while (requestCount > 0 && !m_abort)
            dispatcher->processEvents(QEventLoop::AllEvents);
    }

    void downloadImages()
    {
        if (m_imagesToDownload.isEmpty()) {
            debug("No images to download!");
            return;
        }

        progressTo(m_imageCount);
        progressLabel("Downloading images");

        for (const ImageFormat imageFormat : std::as_const(m_imagesToDownload).keys()) {
            try {
                const QList<QJsonDocument> imageUrlDocs = generateImageUrls(imageFormat);
                for (const QJsonDocument &imageUrlDoc : imageUrlDocs)
                    downloadImages(imageFormat, imageUrlDoc);
            } catch (std::exception &e) {
                warning("Could not generate images: " + QString(e.what()));
            }
        }
    }

    void readInputConfig()
    {
        QFile file(":/config.json");
        if (!file.open(QIODevice::ReadOnly))
            throw std::runtime_error("Could not open file for reading: " + file.fileName().toStdString());

        QJsonParseError parseError;
        QJsonDocument configDoc = QJsonDocument::fromJson(file.readAll(), &parseError);
        if (parseError.error != QJsonParseError::NoError)
            throw std::runtime_error(QString("Could not parse " + file.fileName()
                + ": " + parseError.errorString()).toStdString());

        m_inputConfig = configDoc.object();
    }

    void copyFiles()
    {
        QJsonObject qmlConfig = getObject("qml", m_inputConfig);

        const QStringList controls = availableControls();
        const QStringList filesToCopy = getStringList("copy", qmlConfig, false);
        progressLabel("Copying QML files");
        for (const QString &file : filesToCopy) {
            const auto re = QRegularExpression::fromWildcard(file);
            QDirIterator it(":", QDirIterator::Subdirectories);
            while (it.hasNext()) {
                const QString filePath = it.next();
                if (re.match(filePath).hasMatch()) {
                    const QString baseName = QFileInfo(filePath).baseName();
                    if (controls.contains(baseName)) {
                        // This QML file is a control. Only copy the file
                        // if we're supposed to generate it.
                        if (!m_bridge->m_selectedControls.contains(baseName))
                            continue;
                    }
                    copyFileToStyleFolder(filePath, false);
                }
            }
        }
    }

    void generateControls()
    {
        QJsonArray controlsArray = getArray("controls", m_inputConfig);
        const QJsonArray defaultControls = getArray("default controls", m_inputConfig);
        progressTo(controlsArray.count() + defaultControls.count());

        QRegularExpression re(m_bridge->m_controlToGenerate);
        for (const auto controlValue: defaultControls) {
            progress();
            tryGenerateControl(controlValue.toObject(), re, true);
        }

        for (const auto controlValue : controlsArray) {
            progress();
            tryGenerateControl(controlValue.toObject(), re);
        }
    }

    void tryGenerateControl(const QJsonObject &controlObj, const QRegularExpression &re, bool isDefault = false) {
        const QString name = getString("name", controlObj);
        if (!re.match(name).hasMatch())
            return;

        // If an empty pattern was given from the command line, we
        // respect the controls selected in the UI
        if (re.pattern().isEmpty() && !m_bridge->m_selectedControls.contains(name))
            return;

        try {
            generateControl(controlObj, isDefault);
        } catch (std::exception &e) {
            warning("could not generate " + name + ": " + e.what());
        }
    }

    void generateControl(const QJsonObject &controlObj, const bool isDefault = false)
    {
        const QString controlName = getString("name", controlObj);
        progressLabel("Generating " + controlName);

        // Copy files (typically the QML control) into the style folder
        QStringList files = getStringList("copy", controlObj, false);
        for (const QString &file : files)
            copyFileToStyleFolder(file, m_bridge->m_overwriteQml);

        // Add this control to the list of controls that goes into the qmldir file
        m_qmlDirControls.append(controlName);

        // Export the requested atoms. We do that once for each theme, and
        // put the exported assets into a dedicated theme folder.
        for (const QString &theme : m_themes) try {
            m_currentTheme = theme;
            m_themeVars.clear();
            m_themeVars.insert("Theme", theme);
            if (!isDefault)
                exportAssets(controlObj);
        } catch (std::exception &e) {
            warning("failed exporting assets for theme: "
                + m_themeVars["Theme"] + "; " + e.what());
        }
    }

    void exportAssets(const QJsonObject &controlObj)
    {
        debug("exporting assets for '" + m_currentTheme + "' theme");
        QJsonObject outputControlConfig;

        // Get the description about the control from the input config document
        const auto configAtoms = getArray("atoms", controlObj);

        // Get the json object that describes the control in the Figma file
        const auto controlName = getString("name", controlObj);
        const auto componentSetName = getThemeString("component set", controlObj);
        const auto configStatesArray = getArray("states", controlObj);

        const QJsonObject searchRoot = getComponentSearchRoot(controlObj);
        const QJsonObject componentSet = getComponentSet(searchRoot, componentSetName);
        const QString componentSetId = JsonTools::getString("id", componentSet);
        const QString componentSetPath = JsonTools::resolvedPath(componentSetId);
        debug("using component set: " + componentSetPath);

        for (const QJsonValue &configStateValue : configStatesArray) try {
            QJsonObject outputStateConfig;

            // Resolve all atoms for the given state
            m_currentAtomInfo = "control: " + controlName + "; theme: " + m_currentTheme;
            const QJsonObject configStateObj = configStateValue.toObject();
            const QString controlState = getThemeString("state", configStateObj);
            const QString figmaState = getThemeString("figmaState", configStateObj);

            for (const QJsonValue &atomConfigValue : configAtoms) try {
                QJsonObject outputAtomConfig;
                m_currentAtomInfo = "control: " + controlName
                        + "; theme: " + m_currentTheme + "; state: " + controlState;
                const QJsonObject atomConfigObj = atomConfigValue.toObject();

                // Resolve the atom name. The atomConfigName cannot contain any
                // '-', since it will also be used as property name from QML.
                const QString atomName = getThemeString("atom", atomConfigObj);
                m_currentAtomInfo += "; atom: " + atomName;
                QString atomConfigName = atomName;
                atomConfigName.replace('-', '_');

                // Resolve the path to the node in Figma
                const auto figmaPath = getThemeString("figmaPath", atomConfigObj);
                m_currentAtomInfo += "; figma path: " + figmaPath;

                // Find the json object in the Figma document that describes the atom
                const auto figmaAtomObj = findAtomObject(figmaPath, figmaState, componentSet);

                // Add some convenience values into the config
                const auto figmaId = getString("id", figmaAtomObj);
                outputAtomConfig.insert("figmaId", figmaId);
                m_currentAtomInfo += "; id: " + figmaId;
                const QString atomCombinedName = controlName.toLower() + "-" + atomName
                    + (controlState == "normal" ? "" : "-" + controlState);
                outputAtomConfig.insert("name", atomCombinedName);

                // Export the atom
                QStringList atomExportList = getStringList("export", atomConfigObj, false);
                if (atomExportList.isEmpty()) {
                    atomExportList = m_defaultExport.contains(atomName)
                            ? m_defaultExport[atomName] : m_defaultExport["default"];
                }
                exportFigmaObject(figmaAtomObj, atomExportList, outputAtomConfig);

                // Add the exported atom configuration to the state configuration
                outputStateConfig.insert(atomConfigName, outputAtomConfig);
            } catch (std::exception &e) {
                warning("generate atom: " + QString(e.what()) + "; " + m_currentAtomInfo);
            }

            try {
                // Generate output configuration for the control as a whole. This involves
                // reading the output configuration from the already exported atoms.
                m_currentAtomInfo = "control: " + controlName + ", " + controlState;
                QStringList contentAtoms;
                const auto contentAtomsArray = controlObj["contents"].toArray();
                for (const QJsonValue &atomValue : contentAtomsArray) {
                    QString atomConfigName = themeVarsResolved(atomValue.toString());
                    atomConfigName.replace('-', '_');
                    contentAtoms.append(atomConfigName);
                }

                generateMirrored(contentAtoms, outputStateConfig);
                generateSpacing(contentAtoms, outputStateConfig);
                generatePadding(outputStateConfig);

                const auto stateVariant = findVariantInstance(componentSet, figmaState);
                generateTransitions(stateVariant, outputStateConfig, controlState, configStatesArray);

            } catch (std::exception &e) {
                warning("generate control: " + QString(e.what()) + "; " + m_currentAtomInfo);
            }

            // Add the exported atom configuration to the state configuration
            outputControlConfig.insert(controlState, outputStateConfig);
        } catch (std::exception &e) {
            warning("generate control: " + QString(e.what()) + " " + m_currentAtomInfo);
        }

        auto controlNameModified = controlName.toLower();
        // switch is a keyword in QML so add a "_"
        // before placing it into the config object
        if (controlNameModified == "switch")
            controlNameModified.append("_");

        // Add the control configuration to the global configuration document
        m_outputConfig[m_currentTheme].insert(controlNameModified, outputControlConfig);
    }

    void generateIcons()
    {
        // Note that we don't generate different icons per theme, since
        // they will be colored with a shader in QML to follow the
        // button icon color.
        if (!m_bridge->m_selectedControls.contains("Icons"))
            return;

        try {
            QJsonArray iconGroupsArray = getArray("icons", m_inputConfig);
            for (const auto iconGroupValue : iconGroupsArray) {
                const QJsonObject iconGroupConfig = iconGroupValue.toObject();
                const auto name = getString("name", iconGroupConfig);
                progressLabel("Generating " + name);

                QStringList exportList = getStringList("export", iconGroupConfig);
                if (exportList.contains("image")) {
                    exportList.removeAll("image");
                    exportList += m_imageFormats;
                }

                const auto componentSetName = getThemeString("component set", iconGroupConfig);
                const QJsonObject searchRoot = getComponentSearchRoot(iconGroupConfig);
                const QJsonObject componentSet = getComponentSet(searchRoot, componentSetName);
                const QString componentSetId = JsonTools::getString("id", componentSet);
                const QString componentSetPath = JsonTools::resolvedPath(componentSetId);
                debug("using component set: " + componentSetPath);

                // All the children of the component represents an icon
                const auto children = componentSet.value("children").toArray();
                progressTo(children.count());

                for (auto it = children.constBegin(); it != children.constEnd(); ++it) {
                    exportIcon(it->toObject(), exportList);
                    progress();
                }
            }
        } catch (std::exception &e) {
            warning("failed exporting icons: " + QString(e.what()));
        }
    }

    QString generateQMLForJsonObject(const QJsonObject &object, const QString &objectName, QString &indent) {
        QString qml;

        if (!objectName.isEmpty()) {
            qml += indent + "readonly property QtObject " + objectName + ": QtObject {\n";
            indent += "\t";
        }

        for (auto it = object.begin(); it != object.end(); ++it) {
            QString key = it.key();
            key.replace('-', '_');
            const QJsonValue& value = it.value();

            if (value.isObject()) {
                qml += generateQMLForJsonObject(value.toObject(), key, indent) + "\n";
            } else if (value.isString()) {
                qml += indent + "readonly property string " + key + ": \"" + value.toString() + "\"\n";
            } else if (value.isDouble()) {
                qml += indent + "readonly property real " + key + ": " + QString::number(value.toDouble()) + "\n";
            } else if (value.isBool()) {
                qml += indent + "readonly property bool " + key + ": " + (value.toBool() ? "true" : "false") + "\n";
            } else if (value.isNull()) {
                qml += indent + "readonly property var " + key + ": null\n";
            }
        }

        if (!objectName.isEmpty()) {
            indent.chop(1);
            qml += indent + "}\n";
        }

        return qml;
    }


    QJsonObject findAtomObject(const QString &path, const QString &figmaState, const QJsonObject &componentSet)
    {
        // Construct the json search path to the atom
        // inside the component set, and fetch it.
        QStringList jsonPath;
        QStringList atomPath;
        if (!path.isEmpty())
            atomPath = path.split(",");

        jsonPath.append("state=" + figmaState);
        for (const QString &child : atomPath)
            jsonPath += child.trimmed();

        return findNamedChild(jsonPath, componentSet, false);
    }

    QJsonObject findVariantInstance(const QJsonObject &parent, const QString &figmaState) {
        const auto stateComponent = JsonTools::findChild({"type", "COMPONENT", "name", "state=" + figmaState}, parent, m_bridge->m_sanity);
        const auto stateInstance = JsonTools::findChildWithKey("componentProperties", stateComponent);
        const auto stateObject = getObject("componentProperties", stateInstance);
        if (!stateObject.isEmpty()) {
            const auto value = getObject("state", stateObject);
            if (value.value("type").toString() == "VARIANT"
                && value.value("value").toString() == figmaState) {
                    return stateInstance;
            }
        }
        return {};
    }

    void exportFigmaObject(const QJsonObject &obj, const QStringList &atomExportList,
        QJsonObject &atomConfig)
    {
        for (const QString &atomExport : atomExportList) try {
            if (atomExport == "geometry")
                exportGeometry(obj, atomConfig);
            else if (atomExport == "layout")
                exportLayout(obj, atomConfig);
            else if (atomExport == "image")
                exportImage(obj, m_imageFormats, atomConfig);
            else if (atomExport.startsWith("png") || atomExport.startsWith("svg"))
                exportImage(obj, {atomExport}, atomConfig);
            else if (atomExport == "json")
                exportJson(obj, atomConfig);
            else if (atomExport == "text")
                exportText(obj, atomConfig);
            else if (atomExport == "borderImageOffset")
                exportBorderImageOffset(obj, atomConfig);
            else
                throw std::runtime_error("Unknown option: '" + atomExport.toStdString() + "'");
        } catch (std::exception &e) {
            warning("export atom: " + QString(e.what()) + "; " + m_currentAtomInfo);
        }
    }

    void exportGeometry(const QJsonObject &atom, QJsonObject &outputConfig)
    {
        const QRectF geometry = getFigmaBoundingBox(atom);
        QRectF geometryIncludingShadow = getFigmaRenderBounds(atom);
        if (geometryIncludingShadow.isEmpty())
            geometryIncludingShadow = geometry;

        // Note that the geometry we insert into the config file is
        // the geometry of atom/shape without shadows. This means that
        // if we export an image of the atom, the image size would be
        // equal to geometry + shadows.
        outputConfig.insert("x", geometry.x());
        outputConfig.insert("y", geometry.y());
        outputConfig.insert("width", geometry.width());
        outputConfig.insert("height", geometry.height());

        // Report the margins around the image that contains drop shadows (and
        // possibly other effects). This is quite similar to insets, except that
        // insets is already a term in Quick Controls with a slightly different
        // meaning (it tells the offset of a control's background delegate, which
        // can also be negative).
        // For unknown reasons, Figma sometimes report that the render bounds
        // (geometryIncludingShadow) is smaller than the bouding box (geometry).
        // Hence the need for qMin and qMax until this weirdness is resolved
        // (which might also be a bug in Figma).
        const qreal leftShadow = geometry.x() - geometryIncludingShadow.x();
        const qreal topShadow =  geometry.y() - geometryIncludingShadow.y();
        const qreal rightShadow = geometryIncludingShadow.width() - geometry.width() - leftShadow;
        const qreal bottomShadow = geometryIncludingShadow.height() - geometry.height() - topShadow;
        outputConfig.insert("leftShadow", qMax(0., leftShadow));
        outputConfig.insert("topShadow", qMax(0., topShadow));
        outputConfig.insert("rightShadow", qMax(0., rightShadow));
        outputConfig.insert("bottomShadow", qMax(0., bottomShadow));
    }

    void exportBorderImageOffset(const QJsonObject &atom, QJsonObject &outputConfig)
    {
        const auto geometry = getFigmaBoundingBox(atom);
        const int halfWidth = geometry.width() / 2;
        const int halfHeight = geometry.height() / 2;

        // Get the image offsets from the design. But ensure that the offset ends up
        // smaller than the image itself (since Figma doesn't care if the designer e.g
        // uses a radius that is far bigger than the rectangle).
        BorderImageOffset offset = resolveBorderImageOffset(atom);
        offset.left = qMin(offset.left, halfWidth);
        offset.right = qMin(offset.right, halfWidth);
        offset.top = qMin(offset.top, halfHeight);
        offset.bottom = qMin(offset.bottom, halfHeight);

        // Workaround to make sure that there is at least a 1px
        // middle area to stretch in case of fully-rounded corners
        if ((offset.bottom + offset.top) == geometry.height())
            offset.bottom--;
        if ((offset.right + offset.left) == geometry.width())
            offset.right--;

        outputConfig.insert("leftOffset", offset.left);
        outputConfig.insert("topOffset", offset.top);
        outputConfig.insert("rightOffset", offset.right);
        outputConfig.insert("bottomOffset", offset.bottom);
    }

    void exportImage(const QJsonObject &atom, const QStringList &imageFormats, QJsonObject &outputConfig)
    {
        const QString figmaId = getString("figmaId", outputConfig);
        const QString imageName = getString("name", outputConfig);
        const bool atomVisible = !resolvedHidden(figmaId);

        if (atomVisible) {
            const QString imageFolder = m_currentTheme.toLower() + "/images/";
            for (const ImageFormat imageFormat : imageFormats) {
                const QString fileNameForReading = imageFolder + imageName + "." + imageFormat.format;
                const QString fileNameForWriting =  imageFolder
                        + (imageFormat.hasScale
                           ? imageName + '@' + imageFormat.scale + "x." + imageFormat.format
                           : imageName + '.' + imageFormat.format);

                auto &figmaIdToFileNameMap = m_imagesToDownload[imageFormat.name];
                if (figmaIdToFileNameMap.contains(figmaId))
                    warning("'" + figmaIdToFileNameMap[figmaId] + "' has the same figmaId '" + figmaId
                            + "' as '" + fileNameForWriting + "', and will be overwritten");
                figmaIdToFileNameMap.insert(figmaId, fileNameForWriting);
                m_imageCount++;

                outputConfig.insert("exportType", "image");
                outputConfig.insert("filePath", fileNameForReading);
                debug("exporting image: " + fileNameForWriting);
            }
        } else {
            outputConfig.insert("filePath", "");
            debug("skipping hidden image: " + imageName + (m_bridge->m_sanity ? ", path: " + resolvedPath(figmaId) : ""));
        }

        // Exporting an image will also automatically export related
        // properties, such as geometry, shadows and border offsets
        // (even for hidden / not generated images, otherwise QML bindings will fail)
        exportGeometry(atom, outputConfig);
        exportBorderImageOffset(atom, outputConfig);
    }

    void exportIcon(const QJsonObject &iconObj, const QStringList &imageFormats)
    {
        const QString figmaId = getString("id", iconObj);
        const QString figmaName = getString("name", iconObj);

        QString imageName;
        static QRegularExpression re(R"(Property.*=(.*))");
        QRegularExpressionMatch match = re.match(figmaName);
        if (match.hasMatch()) {
            // The name might be a combination of many properties
            QStringList propertyNames;
            const auto properties = figmaName.split(',');
            for (const auto &propertyName : properties) {
                QRegularExpressionMatch match = re.match(propertyName);
                propertyNames << match.captured(1);
            }
            imageName = propertyNames.join('_').toLower();
        } else {
            imageName = figmaName;
        }
        imageName.replace(' ', '_');
        imageName.replace('-', '_');

        for (const ImageFormat imageFormat : imageFormats) {
            const QString imageFolder = "icons/icons"
                    + (imageFormat.hasScale ? + "@" + imageFormat.scale + "x" : "") + "/";
            const QString fileName = imageFolder + imageName + "." + imageFormat.format;

            auto &figmaIdToFileNameMap = m_imagesToDownload[imageFormat.name];
            if (figmaIdToFileNameMap.contains(figmaId))
                warning("'" + figmaIdToFileNameMap[figmaId] + "' has the same figmaId '" + figmaId
                        + "' as '" + fileName + "', and will be overwritten");
            figmaIdToFileNameMap.insert(figmaId, fileName);
            m_icons.insert(fileName);
            m_imageCount++;

            debug("exporting icon: " + fileName);
        }
    }

    void exportJson(const QJsonObject &atom, QJsonObject &outputConfig)
    {
        const QString name = getString("name", outputConfig);
        const QString fileName = m_currentTheme.toLower() + "/json/" + name + ".json";
        debug("export json: " + m_currentAtomInfo + "; filename: " + fileName);
        createTextFileInStylefolder(fileName, QJsonDocument(atom).toJson());
    }

    void exportText(const QJsonObject &atom, QJsonObject &outputConfig)
    {
        const QJsonObject style = getObject("style", atom);
        const QString figmaAlignmentH = style["textAlignHorizontal"].toString();
        const QString figmaAlignmentV = style["textAlignVertical"].toString();

        Qt::Alignment verticalAlignment = Qt::AlignVCenter;
        Qt::Alignment horizontalAlignment = Qt::AlignHCenter;

        if (figmaAlignmentH == "LEFT")
            horizontalAlignment = Qt::AlignLeft;
        else if (figmaAlignmentH == "RIGHT")
            horizontalAlignment = Qt::AlignRight;

        if (figmaAlignmentV == "TOP")
            verticalAlignment = Qt::AlignTop;
        else if (figmaAlignmentV == "BOTTOM")
            verticalAlignment = Qt::AlignBottom;

        outputConfig.insert("textHAlignment", int(horizontalAlignment));
        outputConfig.insert("textVAlignment", int(verticalAlignment));
        outputConfig.insert("fontFamily", style["fontFamily"]);
        outputConfig.insert("fontSize", style["fontSize"]);
    }

    void exportLayout(const QJsonObject &atom, QJsonObject &outputConfig)
    {
        const auto leftPadding = atom["paddingLeft"];
        const auto topPadding = atom["paddingTop"];
        const auto rightPadding = atom["paddingRight"];
        const auto bottomPadding = atom["paddingBottom"];
        const auto spacing = atom["itemSpacing"];

        // If padding are left unmodified in Figma (all values are zero)
        // the the following keys will be missing in the atom. When that's
        // the case, we just set them to zero.
        outputConfig.insert("leftPadding", leftPadding.isUndefined() ? 0 : leftPadding);
        outputConfig.insert("topPadding", topPadding.isUndefined() ? 0 : topPadding);
        outputConfig.insert("rightPadding", rightPadding.isUndefined() ? 0 : rightPadding);
        outputConfig.insert("bottomPadding", bottomPadding.isUndefined() ? 0 : bottomPadding);
        outputConfig.insert("spacing", spacing.isUndefined() ? 0 : spacing);

        outputConfig.insert("layoutMode", atom["layoutMode"]);
        outputConfig.insert("alignItems", atom["primaryAxisAlignItems"]);
    }

    const QJsonObject getComponentSearchRoot(const QJsonObject &configObj) const
    {
        // Each control in the config file can optionally specify a page
        // where the component set should be found. If not set, we
        // return the document root.
        auto self = const_cast<StyleGenerator *>(this);
        const auto pageName = configObj["page"].toString();
        if (!pageName.isEmpty() && pageName == m_cachedPageName) {
            return m_cachedPage;
        } else if (!pageName.isEmpty()) {
            try {
                const auto documentRoot = getObject("document", m_document.object());
                self->m_cachedPage = JsonTools::findChild({"type", "CANVAS", "name", pageName}, documentRoot, m_bridge->m_sanity);
            } catch (std::exception &e) {
                Q_UNUSED(e);
                throw std::runtime_error("Could not find page in Figma: " + pageName.toStdString());
            }
            self->m_cachedPageName = pageName;
            return m_cachedPage;
        }

        const auto documentRoot = getObject("document", m_document.object());
        return documentRoot;
    }

    const QJsonObject getComponentSet(const QJsonObject &searchRoot, const QString &componentSetName)
    {
        return JsonTools::findChild({"type", "COMPONENT_SET", "name", componentSetName}, searchRoot, m_bridge->m_sanity);
    }

    BorderImageOffset getBorderImageOffset(const QJsonObject &obj)
    {
        // Use radii and border width of the obj to determine the offsets.
        // The biggest of them wins.
        const qreal borderWidth = obj["strokeWeight"].toDouble(0);
        const Radii radii = getRadii(obj);

        BorderImageOffset offset;
        offset.left = qCeil(qMax(borderWidth, qMax(radii.topLeft, radii.bottomLeft)));
        offset.right = qCeil(qMax(borderWidth, qMax(radii.topRight, radii.bottomRight)));
        offset.top = qCeil(qMax(borderWidth, qMax(radii.topLeft, radii.topRight)));
        offset.bottom = qCeil(qMax(borderWidth, qMax(radii.bottomLeft, radii.bottomRight)));

        return offset;
    }

    BorderImageOffset resolveBorderImageOffset(const QJsonObject &atom)
    {
        // If the atom has a child "borderImageOffset", and it's visible, we use it's
        // layout padding to determine the border image offsets. But this is mostly meant
        // as a fall back solution for the designer if our attempt to resolve the offset
        // ends up wrong. Because ideally we try to determine the offset by looking
        // at the radii and border width of the "fillAndStroke" child, or if it's missing, the
        // atom itself. By using an offset that is bigger than the radii and border, we
        // ensure that those parts of the image will not get scaled.
        try {
            const auto child = JsonTools::findNamedChild({"borderImageOffset"}, atom, m_bridge->m_sanity);
            const bool hidden = JsonTools::resolvedHidden(child["id"].toString());
            if (!hidden) {
                BorderImageOffset offset;
                offset.left = child["paddingLeft"].toInt();
                offset.right = child["paddingRight"].toInt();
                offset.top = child["paddingTop"].toInt();
                offset.bottom = child["paddingBottom"].toInt();
                return offset;
            }
        } catch (std::exception &e) {
            Q_UNUSED(e);
        }

        try {
            const auto child = JsonTools::findNamedChild({"fillAndStroke"}, atom, m_bridge->m_sanity);
            return getBorderImageOffset(child);
        } catch (std::exception &e) {
            Q_UNUSED(e);
        }

        return getBorderImageOffset(atom);
    }

    void generateTransitions(const QJsonObject &component, QJsonObject &outputConfig, const QString &controlState, const QJsonArray &statesArray)
    {
        // Due to a limitation in Figma's REST API we are only able
        // to get one transition per component, no matter how many
        // transitions(interactions) the component might have defined in Figma.
        const auto duration = component.value("transitionDuration");
        const auto easingType = component.value("transitionEasing");
        const auto nodeId = component.value("transitionNodeID");

        if (duration == QJsonValue::Undefined
            || easingType == QJsonValue::Undefined
            || nodeId == QJsonValue::Undefined)
            return;

        // Find state for nodeId
        //Get the component name with the given figma id from the "components" key
        const auto components = getObject("components", m_document.object());
        const auto stateComponent = getObject(nodeId.toString(), components);
        const auto figmaStateName = getValue("name", stateComponent);

        // Iterate through the states array in the config.json to find the
        // corresponding control state for the given figma state
        QString stateValue;
        for (auto it = statesArray.begin(); it != statesArray.end(); ++it) {
            const auto stateObject = it->toObject();
            if (QString("state=" + stateObject.value("figmaState").toString()) == figmaStateName.toString()) {
                stateValue = stateObject.value("state").toString();
                break;
            }
        }

        if (stateValue.isEmpty()) {
            warning("No corresponding config state found for figma state: " + figmaStateName.toString() + "; " + m_currentAtomInfo);
            return;
        }

        QJsonObject transitionObject;
        transitionObject.insert("duration", duration);
        transitionObject.insert("type", easingType);
        transitionObject.insert("from", controlState);
        transitionObject.insert("to", stateValue);

        QJsonArray transitionsArray;
        transitionsArray.append(transitionObject);
        outputConfig.insert("transitions", transitionsArray);
    }

    void generateMirrored(const QStringList &contentAtoms, QJsonObject &outputConfig)
    {
        if (contentAtoms.size() < 2)
            return;

        const QRectF atom0Geo = getConfigGeometry(contentAtoms[0].trimmed(), outputConfig);
        const QRectF atom1Geo = getConfigGeometry(contentAtoms[1].trimmed(), outputConfig);

        // Note that the order in which the content atoms are listed in
        // the config file matters when we now try to calculate if the
        // control is mirrored in the design.
        const bool mirrored = !atom0Geo.isEmpty() && !atom1Geo.isEmpty() && atom0Geo.x() > atom1Geo.x();
        outputConfig.insert("mirrored", mirrored);
    }

    void generateSpacing(const QStringList &contentAtoms, QJsonObject &outputConfig)
    {
        // 'spacing' for a Control tells the exact distance between the items inside
        // the contentItem (typically the label and indicator). Since Figma implements
        // spacing a bit differently (and supports modes such as SPACE_BETWEEN, which
        // Controls don't support), we calculate the spacing ourselves based on the
        // geometry of the content atoms.
        if (contentAtoms.size() < 2)
            return;

        const bool mirrored = outputConfig["mirrored"].toBool();
        const QRectF atom0Geo = getConfigGeometry(contentAtoms[0].trimmed(), outputConfig);
        const QRectF atom1Geo = getConfigGeometry(contentAtoms[1].trimmed(), outputConfig);
        const qreal spacing = mirrored ?
            atom0Geo.x() - atom1Geo.x() - atom1Geo.width() :
            atom1Geo.x() - atom0Geo.x() - atom0Geo.width();

        outputConfig.insert("spacing", spacing);
    }

    void generatePadding(QJsonObject &outputConfig)
    {
        // To be able to generate padding, we require that the layout
        // of an atom 'contentItem' has been exported
        const QJsonValue contentItemValue = outputConfig.value("contentItem");
        if (contentItemValue.isUndefined())
            return;

        const QJsonObject contentItem = contentItemValue.toObject();
        outputConfig.insert("leftPadding", contentItem["leftPadding"]);
        outputConfig.insert("topPadding", contentItem["topPadding"]);
        outputConfig.insert("rightPadding", contentItem["rightPadding"]);
        outputConfig.insert("bottomPadding", contentItem["bottomPadding"]);
    }

    void generateConfiguration()
    {
        debug("Generating Config.qml");
        const QString fileName = "/Config.qml";
        QString result;
        result = "pragma Singleton\n"
                "import QtQml\n"
                "\n"
                "QtObject {\n"
                "\treadonly property QtObject controls: Qt.styleHints.colorScheme === Qt.Light ? light.controls : dark.controls\n\n";
        for (const QString &theme : std::as_const(m_outputConfig).keys()) {
            result.append("\treadonly property QtObject " + theme.toLower() + ": QtObject {\n");
            QString indent = "\t\t";
            QString qml = generateQMLForJsonObject(m_outputConfig[theme], "controls", indent);
            indent.chop(1);
            result.append(qml + indent + "}\n");
        }
        result.append("}\n");
        createTextFileInStylefolder(fileName, result);
        progress();
    }

    void generateQmlDir()
    {
        const QString fileName("qmldir");

        debug("Generating qmldir");
        const QString styleName = QFileInfo(m_bridge->m_targetDirectory).fileName();
        const QString version(" 1.0 ");

        QString qmldir;
        qmldir += "module " + styleName + "\n";
        for (const QString &control : m_qmlDirControls)
            qmldir += control + version + control + ".qml\n";

        qmldir += "singleton Config" + version + "Config.qml" + "\n";

        createTextFileInStylefolder(fileName, qmldir);
        progress();
    }

    QString generateQrcSection(const QString &subDir, const QString &prefix, const QString &filter = {})
    {
        const QRegularExpression filterRegExp(filter);
        const QString styleTargetDir = QFileInfo(m_bridge->m_targetDirectory).absoluteFilePath();
        const QString absoluteSubDirPath = styleTargetDir + (subDir.isEmpty() ? "" : QDir::separator() + subDir);
        QDirIterator it(absoluteSubDirPath, QDirIterator::Subdirectories);

        QString resourceString = "\t<qresource prefix=\"" + prefix + "\">\n";

        while (it.hasNext()) {
            const QString file = it.next();
            if (QFileInfo(file).isDir())
                continue;
            const QString relativeFilePath = QDir(absoluteSubDirPath).relativeFilePath(file);
            if (!filter.isEmpty() && filterRegExp.match(relativeFilePath).hasMatch())
                continue;
            if (subDir.isEmpty())
                resourceString += "\t\t<file>" + relativeFilePath + "</file>\n";
            else
                resourceString += "\t\t<file alias=\"" + relativeFilePath + "\">"
                        + subDir + QDir::separator() + relativeFilePath + "</file>\n";
        }

        resourceString += "\t</qresource>\n";
        return resourceString;
    }

    void generateQrcFile()
    {
        debug("Generating Qt resource file");
        const QString styleName = QFileInfo(m_bridge->m_targetDirectory).fileName();

        QString resources
                = QStringLiteral("<RCC>\n")
                += generateQrcSection("", "/qt/qml/" + styleName, "^icons/")
                += generateQrcSection("icons", "/icons/" + styleName)
                += "</RCC>\n";

        createTextFileInStylefolder(styleName + ".qrc", resources);
        progress();
    }

    void generateIndexThemeFile()
    {
        if (!m_bridge->m_selectedControls.contains("Icons"))
            return;

        debug("Generating icons/index.theme");
        const QString styleName = QFileInfo(m_bridge->m_targetDirectory).fileName();
        const QString targetPath = QFileInfo(m_bridge->m_targetDirectory).absolutePath() + QDir::separator();

        QString scaleDirectoriesConfig;
        QStringList scaleDirectories;
        QDirIterator it(targetPath + QDir::separator() + styleName + QDir::separator() + "icons");
        QRegularExpression reGetScale(R"(@(.*)x)");

        while (it.hasNext()) {
            const QString file = it.next();
            const QFileInfo fileInfo(file);
            if (file.endsWith('.') || !fileInfo.isDir())
                continue;

            const QString directoryName = fileInfo.fileName();
            scaleDirectories += directoryName;

            auto scale = reGetScale.match(directoryName).captured(1);
            if (scale.isEmpty())
                scale = "1";

            scaleDirectoriesConfig += "[" + directoryName + "]\n"
                    + "Scale=" + scale + "\n"
                    + "Size=32\n"
                    + "Type=Fixed\n\n";
        }

        const QString contents = QStringLiteral("[Icon Theme]\n")
                + "Name=" + styleName + "\n"
                + "Comment=Generated by Qt StyleGenerator\n\n"
                + "Directories=" + scaleDirectories.join(',') + "\n\n"
                + scaleDirectoriesConfig;

        createTextFileInStylefolder("icons/index.theme", contents);
        progress();
    }

    void mkTargetPath(const QString &path) const
    {
        const QFileInfo fileInfo(path);
        if (fileInfo.exists())
            return;

        const QDir dir = fileInfo.absoluteDir();
        if (!QDir().mkpath(dir.path()))
            throw std::runtime_error("Could not create target path: " + dir.path().toStdString());
    }

    /**
     * Copies the given file into the style folder.
     * If destFileName is empty, the file name of the src will be used.
     */
    void copyFileToStyleFolder(const QString &srcPath, bool overwrite = true, QString destPath = "") const
    {
        QFile srcFile = QFile(srcPath);
        if (!srcFile.exists())
            throw std::runtime_error("File doesn't exist: " + srcPath.toStdString());
        if (destPath.isEmpty())
            destPath = QFileInfo(srcFile).fileName();

        QString targetPath = m_bridge->m_targetDirectory + "/" + destPath;
        mkTargetPath(targetPath);

        if (QFileInfo(targetPath).exists()) {
            if (!overwrite) {
                debug(targetPath + " exists, skipping overwrite");
                return;
            } else if (!QFile(targetPath).remove()) {
                warning("Could not remove existing file: " + targetPath);
                return;
            }
        }

        if (srcFile.copy(targetPath)) {
            debug("copying " + QFileInfo(srcPath).fileName() + " to " + targetPath);
        } else {
            warning("Could not copy " + QFileInfo(srcPath).fileName() + " to " + targetPath);
            return;
        }

        // Files we copy from resources are read-only, so change target permission
        // so that the user can modify generated QML files etc.
        QFile::setPermissions(targetPath, QFileDevice::ReadOwner | QFileDevice::ReadUser
            | QFileDevice::ReadGroup | QFileDevice::ReadOther | QFileDevice::WriteOwner);
    }

    void createTextFileInStylefolder(const QString &fileName, const QString &contents) const
    {
        const QString targetPath = m_bridge->m_targetDirectory + "/" + fileName;
        mkTargetPath(targetPath);
        QFile file(targetPath);
        if (!file.open(QIODevice::WriteOnly))
            throw std::runtime_error("Could not open file for writing: " + targetPath.toStdString());

        QTextStream out(&file);
        out << contents;
    }

    bool fileExists(const QString &destPath) const
    {
        QString targetPath = m_bridge->m_targetDirectory + "/" + destPath;
        return QFileInfo(targetPath).exists();
    }

    QRectF getFigmaBoundingBox(const QJsonObject figmaObject) const
    {
        // Figma bounding box is the bounds of the item / image
        // in scene coordinates, excluding drop shadow and other effects.
        const auto bb = getObject("absoluteBoundingBox", figmaObject);
        const auto x = getValue("x", bb).toDouble();
        const auto y = getValue("y", bb).toDouble();
        const auto width = getValue("width", bb).toDouble();
        const auto height = getValue("height", bb).toDouble();
        return QRectF(x, y, width, height);
    }

    QRectF getFigmaRenderBounds(const QJsonObject figmaObject) const
    {
        // Figma render bounds is the bounds of the whole item / image
        // in scene coordinates, including drop shadow and other effects.
        // Note: 'absoluteRenderBounds' can sometimes be 'null'.
        const auto foundValue = figmaObject.value("absoluteRenderBounds");
        if (foundValue.isNull())
            return {};
        if (!foundValue.isObject())
            throw std::runtime_error("'absoluteRenderBounds' is not an object!");

        const auto bb = foundValue.toObject();
        const auto x = getValue("x", bb).toDouble();
        const auto y = getValue("y", bb).toDouble();
        const auto width = getValue("width", bb).toDouble();
        const auto height = getValue("height", bb).toDouble();
        return QRectF(x, y, width, height);
    }

    QRectF getConfigGeometry(const QString &atomName, const QJsonObject &outputConfig) const
    {
        // Read back the geometry we have already generated in the config object
        const QJsonObject atomObj = getObject(atomName, outputConfig);
        const auto x = getValue("x", atomObj).toDouble();
        const auto y = getValue("y", atomObj).toDouble();
        const auto width = getValue("width", atomObj).toDouble();
        const auto height = getValue("height", atomObj).toDouble();
        return QRectF(x, y, width, height);
    }

    Radii getRadii(const QJsonObject rectangle) const
    {
        const QJsonValue radiusValue = rectangle.value("cornerRadius");
        if (radiusValue != QJsonValue::Undefined) {
            const qreal r = radiusValue.toDouble();
            return {r, r, r, r};
        }

        const QJsonValue radiiValue = rectangle.value("rectangleCornerRadii");
        if (radiiValue == QJsonValue::Undefined)
            return {0, 0, 0, 0};
        const QJsonArray r = radiiValue.toArray();
        Q_ASSERT(r.count() == 4);
        return {r[0].toDouble(), r[1].toDouble(), r[2].toDouble(), r[3].toDouble()};
    }

    QString themeVarsResolved(const QString &str)
    {
        const int first = str.indexOf("${");
        if (first == -1)
            return str;
        const int last = str.indexOf('}', first);
        if (last == -1)
            return str;
        const int themeVarFirst = first + 2;
        const QString themeVar = str.sliced(themeVarFirst, last - themeVarFirst);
        if (!m_themeVars.contains(themeVar)) {
            warning("Theme variable not set: '" + themeVar + "' (" + str + ")");
            return str;
        }
        const QString substitute = m_themeVars[themeVar];
        return str.first(first) + substitute + str.mid(last + 1);
    }

    QString getThemeString(const QString &key, const QJsonObject object)
    {
        // This function is the same as JsonTools::getString(), except that
        // ${Theme} strings inside the return value are substituted with the
        // name of the theme that is currently being processed.
        return themeVarsResolved(getString(key, object));
    }

    QString networkErrorString(QNetworkReply *reply)
    {
        return QMetaEnum::fromType<QNetworkReply::NetworkError>().valueToKey(reply->error());
    }

    void saveForDebug(const QJsonObject &object, const QString &name = "debug.json") const
    {
        debug("saving json for debug: " + name);
        createTextFileInStylefolder(name, QJsonDocument(object).toJson());
    }

    void debug(const QString &msg) const { emit m_bridge->debug(msg); }
    void warning(const QString &msg) const { emit m_bridge->warning(msg); }
    void error(const QString &msg) const { emit m_bridge->error(msg); }
    void progressTo(int to) const { emit m_bridge->progressToChanged(to); }
    void progressLabel(const QString &label) const { emit m_bridge->progressLabelChanged(label); }
    void progress() const { emit m_bridge->progress(); }
    void setFigmaFileName(const QString &name) const { emit m_bridge->figmaFileNameChanged(name); }

public:
    bool m_abort = false;

private:
    Bridge *m_bridge = nullptr;

    QStringList m_imageFormats;
    QMap<QString, QStringList> m_defaultExport;
    QStringList m_themes;
    QStringList m_controls;
    QStringList m_defaultControls;

    QJsonDocument m_document;
    QJsonObject m_inputConfig;
    QMap<QString, QJsonObject> m_outputConfig;
    std::set<QString> m_icons;
    QStringList m_qmlDirControls;

    QString m_cachedPageName;
    QJsonObject m_cachedPage;

    // m_imagesToDownload contains the images to be downloaded.
    // The outer map maps the image format (e.g "svg@2x") to the
    // figma children that should be generated as such images.
    // The inner map maps from figma child id to the file name
    // that the image should be saved to once downloaded.
    QMap<QString, QMap<QString, QString>> m_imagesToDownload;
    int m_imageCount = 0;

    QString m_currentTheme;
    QMap<QString, QString> m_themeVars;

    QString m_currentAtomInfo;
};

#endif // QSTYLEREADER_H
