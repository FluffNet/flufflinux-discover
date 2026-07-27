/*
 *   SPDX-FileCopyrightText: 2018 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick
import org.kde.coreaddons as Core
import org.kde.kirigamiaddons.formcard as FormCard

FormCard.AboutPage {
    readonly property bool isHome: true

    // Preserve the application metadata while keeping licensing details in the
    // repository and packaged license files instead of displaying them here.
    aboutData: ({
        "displayName": Core.AboutData.displayName,
        "productName": Core.AboutData.productName,
        "componentName": Core.AboutData.componentName,
        "shortDescription": Core.AboutData.shortDescription,
        "homepage": Core.AboutData.homepage,
        "bugAddress": Core.AboutData.bugAddress,
        "version": Core.AboutData.version,
        "otherText": Core.AboutData.otherText,
        "authors": Core.AboutData.authors,
        "credits": Core.AboutData.credits,
        "translators": Core.AboutData.translators,
        "licenses": [],
        "copyrightStatement": Core.AboutData.copyrightStatement,
        "desktopFileName": Core.AboutData.desktopFileName,
        "programLogo": Core.AboutData.programLogo
    })

    donateUrl: ""
    getInvolvedUrl: ""
}
