/*
 *   SPDX-FileCopyrightText: 2017 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *   SPDX-FileCopyrightText: 2019 Carl Schwan <carl@carlschwan.eu>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick
import QtQuick.Controls
import org.kde.kirigami as Kirigami

Kirigami.SearchField {
    id: root

    // for appium tests
    objectName: "searchField"

    // The Flatpak catalog is already local, so update results as the user
    // types instead of requiring Enter or imposing a delayed search.
    autoAccept: true
    delaySearch: false

    property QtObject page
    property string currentSearchText

    placeholderText: (!enabled || !page || page.hasOwnProperty("isHome") || window.leftPage.name.length === 0) ? i18n("Search…") : i18n("Search in '%1'…", window.leftPage.name)

    onAccepted: {
        // Keep the user's text untouched while typing. Mutating the field to
        // its trimmed form here made every trailing space disappear before
        // the next word could be entered.
        currentSearchText = text.trim();
    }

    function clearText() {
        text = "";
        accepted();
    }

    Connections {
        ignoreUnknownSignals: true
        target: root.page

        function onClearSearch() {
            root.clearText();
        }
    }

    Connections {
        target: applicationWindow()
        function onCurrentTopLevelChanged() {
            if (applicationWindow().currentTopLevel.length > 0) {
                root.clearText();
            }
        }
    }
}
