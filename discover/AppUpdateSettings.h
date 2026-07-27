/*
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <KConfigWatcher>
#include <KSharedConfig>
#include <QObject>

class AppUpdateSettings : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool automaticUpdates READ automaticUpdates WRITE setAutomaticUpdates NOTIFY automaticUpdatesChanged)
    Q_PROPERTY(int updateInterval READ updateInterval WRITE setUpdateInterval NOTIFY updateIntervalChanged)

public:
    explicit AppUpdateSettings(QObject *parent = nullptr);

    bool automaticUpdates() const;
    void setAutomaticUpdates(bool enabled);

    int updateInterval() const;
    void setUpdateInterval(int seconds);

Q_SIGNALS:
    void automaticUpdatesChanged();
    void updateIntervalChanged();

private:
    void reload();

    KSharedConfig::Ptr m_config;
    KConfigWatcher::Ptr m_watcher;
    bool m_automaticUpdates = true;
    int m_updateInterval = 7 * 24 * 60 * 60;
};
