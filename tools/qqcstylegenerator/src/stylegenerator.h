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

#include "jsontools.h"

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

class StyleGenerator
{

public:
    StyleGenerator(const QString &fileId, const QString &token
        , const QString &targetPath, bool verbose, bool silent, const QString &generate)
        : m_fileId(fileId)
        , m_token(token)
        , m_targetPath(targetPath)
        , m_verbose(verbose)
        , m_silent(silent)
        , m_controlToGenerate(generate)
    {
    }

    void generateStyle()
    {
        debug("Generate style: " + m_targetPath);

        readInputConfig();
        downloadFigmaDocument();
        generateControls();
        downloadImages();
        debugHeader("Generating config files");
        generateQmlDir();
        generateConfiguration();
    }

private:

    void downloadFigmaDocument()
    {
        debug("downloading figma file");
        newProgress("downloading figma file");
        QJsonDocument figmaResponsDoc;
        const QUrl url("https://api.figma.com/v1/files/" + m_fileId);
        debug("requesting: " + url.toString());

        QNetworkRequest request(url);
        request.setRawHeader(QByteArray("X-FIGMA-TOKEN"), m_token.toUtf8());
        debug("using token: " + m_token.toUtf8());

        QScopedPointer<QNetworkAccessManager> manager(new QNetworkAccessManager);
        manager->setAutoDeleteReplies(true);

        QNetworkReply *reply = manager->get(request);

        QObject::connect(reply, &QNetworkReply::finished, [this, &reply]{
            if (reply->error() == QNetworkReply::NoError)
                m_document = QJsonDocument::fromJson(reply->readAll());

            if (qgetenv("QSTYLEGENERATOR_SAVEDOC") == "true")
                saveForDebug(m_document.object(), "figmastyle.json");
        });

        QObject::connect(reply, &QNetworkReply::downloadProgress, [this]{
            progress();
        });

        while (reply->isRunning())
            qApp->processEvents();

        if (reply->error() != QNetworkReply::NoError)
            throw RestCallException(QStringLiteral("Could not download design file from Figma: ")
                + networkErrorString(reply));
    }

    QJsonDocument generateImageUrls(const ImageFormat &imageFormat)
    {
        newProgress("downloading image urls with format " + imageFormat.name);
        QJsonDocument figmaResponsDoc;

        const auto figmaIdToFileNameMap = m_imagesToDownload[imageFormat.name];
        const QString ids = QStringList(figmaIdToFileNameMap.keys()).join(",");

        const QUrl url("https://api.figma.com/v1/images/"
                       + m_fileId
                       + "?ids=" + ids
                       + "&format=" + imageFormat.format
                       + "&scale=" + imageFormat.scale);

        debug("request: " + url.toString());
        QNetworkRequest request(url);
        request.setRawHeader(QByteArray("X-FIGMA-TOKEN"), m_token.toUtf8());

        QScopedPointer<QNetworkAccessManager> manager(new QNetworkAccessManager);
        manager->setAutoDeleteReplies(true);

        QNetworkReply *reply = manager->get(request);

        QObject::connect(reply, &QNetworkReply::finished, [reply, &figmaResponsDoc]{
            if (reply->error() == QNetworkReply::NoError)
                figmaResponsDoc = QJsonDocument::fromJson(reply->readAll());
        });

        QObject::connect(reply, &QNetworkReply::downloadProgress, [this]{
            progress();
        });

        while (reply->isRunning())
            qApp->processEvents();

        if (reply->error() != QNetworkReply::NoError)
            throw RestCallException(QStringLiteral("Could not download images from Figma: ")
                + networkErrorString(reply));

        return figmaResponsDoc;
    }

    void downloadImages(const ImageFormat &imageFormat, const QJsonDocument &figmaImagesResponsDoc)
    {
        debug("downloading requested images with format " + imageFormat.name);
        newProgress("downloading images with format " + imageFormat.name);

        const auto imageUrls = getObject("images", figmaImagesResponsDoc.object());
        const auto figmaIdToFileNameMap = m_imagesToDownload[imageFormat.name];

        QScopedPointer<QNetworkAccessManager> manager(new QNetworkAccessManager);
        manager->setAutoDeleteReplies(true);
        int requestCount = imageUrls.keys().count();

        for (const QString &figmaId : imageUrls.keys()) {
            const QString imageUrl = imageUrls.value(figmaId).toString();
            if (imageUrl.isEmpty()) {
                qWarning().noquote().nospace() << "No image URL generated for id: " << figmaId << ", " << figmaIdToFileNameMap.value(figmaId);
                requestCount--;
                continue;
            }

            QNetworkReply *reply = manager->get(QNetworkRequest(imageUrl));

            QObject::connect(reply, &QNetworkReply::finished,
                             [this, figmaId, imageUrl, imageFormat,
                             reply, &figmaIdToFileNameMap, &requestCount] {
                requestCount--;
                if (reply->error() != QNetworkReply::NoError) {
                    qWarning().nospace().noquote() << "Failed to download "
                        << imageUrl << " (id: " << figmaId << "). "
                        << "Error code:" << networkErrorString(reply);
                    return;
                }

                const QString filePath = m_targetPath + "/" + figmaIdToFileNameMap.value(figmaId);
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
                        qWarning().nospace().noquote() << "Failed to create pixmap: "
                            << filePath << " from " << imageUrl;
                        return;
                    }
                    if (!pixmap.save(filePath)) {
                        qWarning().nospace().noquote() << "Failed to save pixmap: "
                            << filePath << " from " << imageUrl;
                        return;
                    }
                }
                debug("downloaded image: " + filePath + " from " + imageUrl);
                progress();
            });
        }

        while (requestCount > 0)
            qApp->processEvents();
    }

    void downloadImages()
    {
        debugHeader("Downloading images from Figma");
        if (m_imagesToDownload.isEmpty()) {
            debug("No images to download!");
            return;
        }

        for (const ImageFormat imageFormat : std::as_const(m_imagesToDownload).keys()) {
            try {
                const QJsonDocument imageUrlDoc = generateImageUrls(imageFormat);
                downloadImages(imageFormat, imageUrlDoc);
            } catch (std::exception &e) {
                qWarning().nospace().noquote() << "Could not generate images: " << e.what();
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

    void generateControls()
    {
        QJsonArray controlsArray = getArray("controls", m_inputConfig);

        const QJsonArray themesArray = m_inputConfig.value("themes").toArray();
        if (themesArray.isEmpty())
            throw std::runtime_error("The input config needs to list at least one theme!");
        for (const QJsonValue &themeValue : themesArray)
            m_themes.append(themeValue.toString());

        const QJsonArray imageFormats = m_inputConfig.value("image formats").toArray();
        for (const QJsonValue &formatValue : imageFormats)
            m_imageFormats.append(formatValue.toString());

        const QJsonArray exportArray = m_inputConfig.value("default export").toArray();
        for (const QJsonValue &exportValue : exportArray)
            m_defaultExport.append(exportValue.toString());

        for (const auto controlValue : controlsArray) {
            const auto controlObj = controlValue.toObject();
            const QString name = getString("name", controlObj);
            if (!m_controlToGenerate.isEmpty() && name != m_controlToGenerate)
                continue;

            try {
                generateControl(controlObj);
            } catch (std::exception &e) {
                qWarning().nospace().noquote() << "Warning, could not generate " << name << ": " << e.what();
            }
        }
    }

    void generateControl(const QJsonObject &controlObj)
    {
        const QString controlName = getString("name", controlObj);
        debugHeader(controlName);

        // Copy files (typically the QML control) into the style folder
        QStringList files = getStringList("copy", controlObj, false);
        for (const QString &file : files)
            copyFileToStyleFolder(file, false);

        // Add this control to the list of controls that goes into the qmldir file
        m_qmlDirControls.append(controlName);

        // Export the requested atoms. We do that once for each theme, and
        // put the exported assets into a dedicated theme folder.
        for (const QString &theme : m_themes) try {
            m_currentTheme = theme;
            m_themeVars.clear();
            m_themeVars.insert("Theme", theme);
            exportAssets(controlObj);
        } catch (std::exception &e) {
            qWarning().nospace().noquote() << "Warning, failed exporting assets for theme: "
                                           << m_themeVars["Theme"] << "; " << e.what();
        }
    }

    void exportAssets(const QJsonObject &controlObj)
    {
        debug("\nexporting assets for '" + m_currentTheme + "' theme");
        QJsonObject outputControlConfig;

        // Get the description about the control from the input config document
        const auto configAtoms = getArray("atoms", controlObj);

        // Get the json object that describes the control in the Figma file
        const auto controlName = getString("name", controlObj);
        const auto componentSetName = getThemeString("component set", controlObj);
        const auto documentRoot = getObject("document", m_document.object());
        const auto componentSet = findChild({"type", "COMPONENT_SET", "name", componentSetName}, documentRoot);
        const auto configStatesArray = getArray("states", controlObj);

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
                QStringList customExportList = getStringList("export", atomConfigObj, false);
                const auto atomExportList = customExportList.isEmpty() ? m_defaultExport : customExportList;

                for (const QString &atomExport : atomExportList) try {
                    if (atomExport == "geometry")
                        exportGeometry(figmaAtomObj, outputAtomConfig);
                    else if (atomExport == "layout")
                        exportLayout(figmaAtomObj, outputAtomConfig);
                    else if (atomExport == "image")
                        exportImage(figmaAtomObj, m_imageFormats, outputAtomConfig);
                    else if (atomExport.startsWith("png") || atomExport.startsWith("svg"))
                        exportImage(figmaAtomObj, {atomExport}, outputAtomConfig);
                    else if (atomExport == "json")
                        exportJson(figmaAtomObj, outputAtomConfig);
                    else if (atomExport == "text")
                        exportText(figmaAtomObj, outputAtomConfig);
                    else
                        throw std::runtime_error("Unknown option: '" + atomExport.toStdString() + "'");
                } catch (std::exception &e) {
                    qWarning().nospace().noquote() << "Warning, export atom: " << e.what() << "; " << m_currentAtomInfo;
                }

                // Add the exported atom configuration to the state configuration
                outputStateConfig.insert(atomConfigName, outputAtomConfig);
            } catch (std::exception &e) {
                qWarning().nospace().noquote() << "Warning, generate atom: " << e.what() << "; " << m_currentAtomInfo;
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

                const auto stateComponent = findChild({"type", "COMPONENT", "name", "state=" + figmaState}, componentSet);
                generateTransitions(stateComponent, outputStateConfig, configStatesArray);

            } catch (std::exception &e) {
                qWarning().nospace().noquote() << "Warning, generate control: " << e.what() << "; " << m_currentAtomInfo;
            }

            // Add the exported atom configuration to the state configuration
            outputControlConfig.insert(controlState, outputStateConfig);
        } catch (std::exception &e) {
            qWarning().nospace().noquote() << "Warning, generate control: " << e.what() << " " << m_currentAtomInfo;
        }

        // Add the control configuration to the global configuration document
        m_outputConfig[m_currentTheme].insert(controlName.toLower(), outputControlConfig);
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

        return findNamedChild(jsonPath, componentSet);
    }

    void exportGeometry(const QJsonObject &atom, QJsonObject &outputConfig)
    {
        const auto geometry = getGeometry(atom);
        const auto stretch = getStretch(atom);

        outputConfig.insert("x", geometry.x());
        outputConfig.insert("y", geometry.y());
        outputConfig.insert("width", geometry.width());
        outputConfig.insert("height", geometry.height());

        const int halfWidth = geometry.width() / 2;
        const int halfHeight = geometry.height() / 2;

        auto leftOffset = qMin(stretch.left(), halfWidth);
        auto rightOffset = qMin(stretch.right(), halfWidth);
        auto topOffset = qMin(stretch.top(), halfHeight);
        auto bottomOffset = qMin(stretch.bottom(), halfHeight);

        // workaround to make sure that there is at least a 1px
        // middle area to stretch in case of fully-rounded corners
        if ((bottomOffset + topOffset) == geometry.height())
            bottomOffset--;
        if ((rightOffset + leftOffset) == geometry.width())
            rightOffset--;

        outputConfig.insert("leftOffset", leftOffset);
        outputConfig.insert("topOffset", topOffset);
        outputConfig.insert("rightOffset", rightOffset);
        outputConfig.insert("bottomOffset", bottomOffset);

        // Todo: resolve insets for rectangles with drop shadow
        // config.insert("leftInset", 0);
        // config.insert("topInset", 0);
        // config.insert("rightInset", 0);
        // config.insert("bottomInset", 0);
    }

    void exportImage(const QJsonObject &atom, const QStringList &imageFormats, QJsonObject &outputConfig)
    {
        const QString figmaId = getString("figmaId", outputConfig);
        const QString imageName = getString("name", outputConfig);

        if (!atom.value("qt_visibleRecursive").toBool()) {
            outputConfig.insert("filePath", "");
            debug("skipping hidden image: " + imageName);
            return;
        }

        const QString imageFolder = m_currentTheme.toLower() + "/images/";
        for (const ImageFormat imageFormat : imageFormats) {
            const QString fileNameForReading = imageFolder + imageName + "." + imageFormat.format;
            const QString fileNameForWriting =  imageFolder
                    + (imageFormat.hasScale
                    ? imageName + '@' + imageFormat.scale + "x." + imageFormat.format
                    : imageName + '.' + imageFormat.format);

            auto &figmaIdToFileNameMap = m_imagesToDownload[imageFormat.name];
            figmaIdToFileNameMap.insert(figmaId, fileNameForWriting);

            outputConfig.insert("export", "image");
            outputConfig.insert("filePath", fileNameForReading);
            debug("exporting image: " + fileNameForWriting);
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
        const auto spacing = atom["spacing"];

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

    void generateTransitions(const QJsonObject &component, QJsonObject &outputConfig, const QJsonArray &statesArray)
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
            qWarning().noquote().nospace() << "No corresponding state found for figma state: " << figmaStateName.toString();
            return;
        }

        QJsonObject transitionObject;
        transitionObject.insert("duration", duration);
        transitionObject.insert("type", easingType);
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
        for (const QString &theme : std::as_const(m_outputConfig).keys()) {
            QJsonObject root;
            root.insert("version", "1.0");
            root.insert("controls", m_outputConfig[theme]);
            const QString fileName = theme.toLower() + "/config.json";
            debug("generating " + fileName);
            createTextFileInStylefolder(fileName, QJsonDocument(root).toJson());
        }
    }

    void generateQmlDir()
    {
        const QString styleName = QFileInfo(m_targetPath).fileName();
        const QString version(" 1.0 ");

        QString qmldir;
        qmldir += "module " + styleName + "\n";
        for (const QString &control : m_qmlDirControls)
            qmldir += control + version + control + ".qml\n";

        debug("generating qmldir");
        createTextFileInStylefolder("qmldir", qmldir);
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

        QString targetPath = m_targetPath + "/" + destPath;
        mkTargetPath(targetPath);
        if (!overwrite && QFileInfo(targetPath).exists()) {
            debug(targetPath + " exists, skipping overwrite");
            return;
        }

        debug("copying " + QFileInfo(srcPath).fileName() + " to " + targetPath);
        srcFile.copy(targetPath);

        // Files we copy from resources are read-only, so change target permission
        // so that the user can modify generated QML files etc.
        QFile::setPermissions(targetPath, QFileDevice::ReadOwner | QFileDevice::ReadUser
            | QFileDevice::ReadGroup | QFileDevice::ReadOther | QFileDevice::WriteOwner);
    }

    void createTextFileInStylefolder(const QString &fileName, const QString &contents) const
    {
        const QString targetPath = m_targetPath + "/" + fileName;
        mkTargetPath(targetPath);
        QFile file(targetPath);
        if (!file.open(QIODevice::WriteOnly))
            throw std::runtime_error("Could not open file for writing: " + targetPath.toStdString());

        QTextStream out(&file);
        out << contents;
    }

    QRectF getGeometry(const QJsonObject figmaObject) const
    {
        const auto bb = getObject("absoluteBoundingBox", figmaObject);
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

    QMargins getStretch(const QJsonObject rectangle) const
    {
        // Determine the stretch factor of the rectangle based on its corner radii
        qreal topLeftRadius = 0;
        qreal topRightRadius = 0;
        qreal bottomRightRadius = 0;
        qreal bottomLeftRadius = 0;

        const QJsonValue radiusValue = rectangle.value("cornerRadius");
        if (radiusValue != QJsonValue::Undefined) {
            const qreal r = radiusValue.toDouble();
            topLeftRadius = r;
            topRightRadius = r;
            bottomRightRadius = r;
            bottomLeftRadius = r;
        } else {
            const QJsonValue radiiValue = rectangle.value("rectangleCornerRadii");
            if (radiiValue == QJsonValue::Undefined)
                return {};
            const QJsonArray r = radiiValue.toArray();
            Q_ASSERT(r.count() == 4);
            topLeftRadius= r[0].toDouble();
            topRightRadius= r[1].toDouble();
            bottomRightRadius = r[2].toDouble();
            bottomLeftRadius = r[3].toDouble();
        }

        const qreal left = qMax(topLeftRadius, bottomLeftRadius);
        const qreal right = qMax(topRightRadius, bottomRightRadius);
        const qreal top = qMax(topLeftRadius, topRightRadius);
        const qreal bottom = qMax(bottomLeftRadius, bottomRightRadius);

        return QMargins(left, top, right, bottom);
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
            qWarning().nospace().noquote() << "Theme variable not set: '" << themeVar << "' (" << str << ")";
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

    void newProgress(const QString &label)
    {
        if (m_verbose || m_silent)
            return;
        m_progressLabel = label;
        m_progress = 0;
        printf("\33[2K%s: 0\r", qPrintable(m_progressLabel));
        fflush(stdout);
    }

    void progress()
    {
        if (m_verbose || m_silent)
            return;
        printf("\33[2K%s: %i\r", qPrintable(m_progressLabel), ++m_progress);
        fflush(stdout);
    }

    void debug(const QString &msg) const
    {
        if (!m_verbose)
            return;
        qDebug().noquote() << msg;
    }

    void debugHeader(const QString &msg) const
    {
        debug("");
        debug("*** " + msg + " ***");
    }

    void saveForDebug(const QJsonObject &object, const QString &name = "debug.json") const
    {
        debug("saving json for debug: " + name);
        createTextFileInStylefolder(name, QJsonDocument(object).toJson());
    }

private:
    QString m_fileId;
    QString m_token;
    QString m_targetPath;
    bool m_verbose = false;
    bool m_silent = false;

    QStringList m_imageFormats;
    QStringList m_defaultExport;
    QStringList m_themes;

    QJsonDocument m_document;
    QJsonObject m_inputConfig;
    QMap<QString, QJsonObject> m_outputConfig;
    QStringList m_qmlDirControls;
    QString m_controlToGenerate;

    // m_imagesToDownload contains the images to be downloaded.
    // The outer map maps the image format (e.g "svg@2x") to the
    // figma children that should be generated as such images.
    // The inner map maps from figma child id to the file name
    // that the image should be saved to once downloaded.
    QMap<QString, QMap<QString, QString>> m_imagesToDownload;

    QString m_currentTheme;
    QMap<QString, QString> m_themeVars;

    QString m_currentAtomInfo;
    QString m_progressLabel;
    int m_progress = 0;
};

#endif // QSTYLEREADER_H
