/*
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <KConfigWatcher>
#include <KSharedConfig>
#include <QDateTime>
#include <QObject>

class AppUpdateSettings : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool automaticUpdates READ automaticUpdates WRITE setAutomaticUpdates NOTIFY automaticUpdatesChanged)
    Q_PROPERTY(int updateInterval READ updateInterval WRITE setUpdateInterval NOTIFY updateIntervalChanged)
    Q_PROPERTY(bool hasLastSuccessfulUpdate READ hasLastSuccessfulUpdate NOTIFY updateHistoryChanged)
    Q_PROPERTY(QString lastSuccessfulUpdate READ lastSuccessfulUpdate NOTIFY updateHistoryChanged)
    Q_PROPERTY(QString relativeLastSuccessfulUpdate READ relativeLastSuccessfulUpdate NOTIFY updateHistoryChanged)
    Q_PROPERTY(bool lastUpdateOlderThanWeek READ lastUpdateOlderThanWeek NOTIFY updateHistoryChanged)
    Q_PROPERTY(bool checkedThisSession READ checkedThisSession NOTIFY updateHistoryChanged)

public:
    explicit AppUpdateSettings(QObject *parent = nullptr);

    static void ensureDefaultsExist();

    bool automaticUpdates() const;
    void setAutomaticUpdates(bool enabled);

    int updateInterval() const;
    void setUpdateInterval(int seconds);

    bool hasLastSuccessfulUpdate() const;
    QString lastSuccessfulUpdate() const;
    QString relativeLastSuccessfulUpdate() const;
    bool lastUpdateOlderThanWeek() const;
    bool checkedThisSession() const;

    Q_INVOKABLE void recordUpdateCheck();
    Q_INVOKABLE void recordUpdateSuccess();
    Q_INVOKABLE void recordUpdateFailure(const QString &error);
    Q_INVOKABLE void reloadUpdateHistory();

Q_SIGNALS:
    void automaticUpdatesChanged();
    void updateIntervalChanged();
    void updateHistoryChanged();

private:
    void reload();

    KSharedConfig::Ptr m_config;
    KConfigWatcher::Ptr m_watcher;
    bool m_automaticUpdates = true;
    int m_updateInterval = 7 * 24 * 60 * 60;
    bool m_checkedThisSession = false;
    QDateTime m_sessionStarted;
};
