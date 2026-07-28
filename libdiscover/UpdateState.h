/*
 * SPDX-FileCopyrightText: 2026 FluffNet LLC
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include "discovercommon_export.h"

#include <QDateTime>
#include <QString>

namespace UpdateState
{
struct DISCOVERCOMMON_EXPORT State {
    QDateTime lastCheck;
    QDateTime lastAttempt;
    QDateTime lastSuccess;
    QDateTime lastFailure;
    QDateTime nextScheduledUpdate;
    QString lastError;
    int retryCount = 0;
    bool pending = false;
};

DISCOVERCOMMON_EXPORT QString path();
DISCOVERCOMMON_EXPORT State read();
DISCOVERCOMMON_EXPORT bool write(const State &state);
DISCOVERCOMMON_EXPORT void recordCheck();
DISCOVERCOMMON_EXPORT void recordAttempt();
DISCOVERCOMMON_EXPORT void recordSuccess(int intervalSeconds);
DISCOVERCOMMON_EXPORT void recordNoUpdates(int intervalSeconds);
DISCOVERCOMMON_EXPORT void recordFailure(const QString &error, int retryDelaySeconds);
DISCOVERCOMMON_EXPORT void postponeUntil(const QDateTime &when);
}
