#ifndef PGNSAVEWORKER_H
#define PGNSAVEWORKER_H

#include "pgngamedata.h"

#include <QObject>
#include <QVector>
#include <QMutex>
#include <QAtomicInteger>
#include <atomic>

class PGNSaveWorker : public QObject
{
    Q_OBJECT
public:
    explicit PGNSaveWorker(QObject *parent = nullptr);

public slots:
    void requestSave(const QString &filePath, const QVector<PGNGame> &games);
    void requestCancel();
    void doSave();

private:
    QMutex m_mutex;
    QString m_pendingPath;
    QVector<PGNGame> m_pendingGames;
    std::atomic<bool> m_cancel;
};

#endif // PGNSAVEWORKER_H
