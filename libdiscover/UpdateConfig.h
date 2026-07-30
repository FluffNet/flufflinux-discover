/*
 * SPDX-FileCopyrightText: 2026 FluffNet LLC
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include "discovercommon_export.h"

#include <QString>

namespace UpdateConfig
{
struct DISCOVERCOMMON_EXPORT Settings {
    bool automaticUpdates = true;
    int intervalSeconds = 7 * 24 * 60 * 60;
};

DISCOVERCOMMON_EXPORT Settings read();
DISCOVERCOMMON_EXPORT Settings ensureAndMigrate();
DISCOVERCOMMON_EXPORT void writeAutomaticUpdates(bool enabled, int intervalSeconds);
DISCOVERCOMMON_EXPORT void writeUpdateInterval(int intervalSeconds);
DISCOVERCOMMON_EXPORT QString intervalName(int intervalSeconds);
DISCOVERCOMMON_EXPORT int intervalSeconds(const QString &name);
}
