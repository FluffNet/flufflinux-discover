/*
 * SPDX-FileCopyrightText: 2026 FluffNet LLC
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "UpdateConfig.h"

#include <KConfigGroup>
#include <KSharedConfig>

#include <algorithm>
#include <array>
#include <cstdlib>

namespace
{
constexpr auto configFile = "PlasmaDiscoverUpdates";
constexpr auto configGroup = "Global";
constexpr auto automaticUpdatesKey = "AutomaticUpdates";
constexpr auto updateIntervalKey = "UpdateInterval";
constexpr auto legacyAutomaticUpdatesKey = "UseUnattendedUpdates";
constexpr auto legacyUpdateIntervalKey = "RequiredNotificationInterval";
constexpr int dailyInterval = 24 * 60 * 60;
constexpr int weeklyInterval = 7 * dailyInterval;
constexpr int monthlyInterval = 30 * dailyInterval;

KSharedConfig::Ptr updateConfig()
{
    return KSharedConfig::openConfig(QLatin1String(configFile));
}

int closestSupportedInterval(int seconds)
{
    const std::array supported = {dailyInterval, weeklyInterval, monthlyInterval};
    return *std::min_element(supported.cbegin(), supported.cend(), [seconds](int left, int right) {
        return std::abs(left - seconds) < std::abs(right - seconds);
    });
}
}

QString UpdateConfig::intervalName(int intervalSeconds)
{
    switch (closestSupportedInterval(intervalSeconds)) {
    case dailyInterval:
        return QStringLiteral("daily");
    case monthlyInterval:
        return QStringLiteral("monthly");
    default:
        return QStringLiteral("weekly");
    }
}

int UpdateConfig::intervalSeconds(const QString &name)
{
    if (name.compare(QStringLiteral("daily"), Qt::CaseInsensitive) == 0) {
        return dailyInterval;
    }
    if (name.compare(QStringLiteral("monthly"), Qt::CaseInsensitive) == 0) {
        return monthlyInterval;
    }
    return weeklyInterval;
}

UpdateConfig::Settings UpdateConfig::read()
{
    const KSharedConfig::Ptr config = updateConfig();
    const KConfigGroup group(config, QLatin1String(configGroup));
    bool automatic = group.hasKey(automaticUpdatesKey)
        ? group.readEntry(automaticUpdatesKey, true)
        : group.readEntry(legacyAutomaticUpdatesKey, true);

    int interval = weeklyInterval;
    if (group.hasKey(updateIntervalKey)) {
        interval = intervalSeconds(group.readEntry(updateIntervalKey, QStringLiteral("weekly")));
    } else if (group.hasKey(legacyUpdateIntervalKey)) {
        const int legacyInterval = group.readEntry(legacyUpdateIntervalKey, weeklyInterval);
        if (legacyInterval <= 0) {
            automatic = false;
        } else {
            interval = closestSupportedInterval(legacyInterval);
        }
    }
    return {.automaticUpdates = automatic, .intervalSeconds = interval};
}

UpdateConfig::Settings UpdateConfig::ensureAndMigrate()
{
    const Settings settings = read();
    const KSharedConfig::Ptr config = updateConfig();
    KConfigGroup group(config, QLatin1String(configGroup));

    group.writeEntry(automaticUpdatesKey, settings.automaticUpdates);
    group.deleteEntry(legacyAutomaticUpdatesKey);
    group.deleteEntry(legacyUpdateIntervalKey);
    if (settings.automaticUpdates) {
        group.writeEntry(updateIntervalKey, intervalName(settings.intervalSeconds));
    } else {
        group.deleteEntry(updateIntervalKey);
    }
    config->sync();
    return settings;
}

void UpdateConfig::writeAutomaticUpdates(bool enabled, int intervalSeconds)
{
    const KSharedConfig::Ptr config = updateConfig();
    KConfigGroup group(config, QLatin1String(configGroup));
    group.writeEntry(automaticUpdatesKey, enabled);
    group.deleteEntry(legacyAutomaticUpdatesKey);
    group.deleteEntry(legacyUpdateIntervalKey);
    if (enabled) {
        group.writeEntry(updateIntervalKey, intervalName(intervalSeconds));
    } else {
        group.deleteEntry(updateIntervalKey);
    }
    config->sync();
}

void UpdateConfig::writeUpdateInterval(int intervalSeconds)
{
    const KSharedConfig::Ptr config = updateConfig();
    KConfigGroup group(config, QLatin1String(configGroup));
    if (group.readEntry(automaticUpdatesKey, true)) {
        group.writeEntry(updateIntervalKey, intervalName(intervalSeconds));
    }
    group.deleteEntry(legacyUpdateIntervalKey);
    config->sync();
}
