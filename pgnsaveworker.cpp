#include "pgnsaveworker.h"

#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QMessageBox>

PGNSaveWorker::PGNSaveWorker(QObject *parent)
    : QObject{parent}
{
    m_cancel = false;
}

void PGNSaveWorker::requestSave(const QString &filePath, const QVector<PGNGame> &database)
{
    m_cancel.store(true);
    QMutexLocker lock(&m_mutex);
    m_pendingPath = filePath;
    m_pendingGames = database;

    QMetaObject::invokeMethod(this, "doSave", Qt::QueuedConnection);
}

void PGNSaveWorker::requestCancel()
{
    m_cancel.store(true);
}

void PGNSaveWorker::doSave()
{
    m_cancel.store(false);

    QString tempPath;
    QVector<PGNGame> dbCopy;

    QMutexLocker lock(&m_mutex);
    tempPath = m_pendingPath + ".tmp";
    dbCopy = m_pendingGames;

    QFile tmpFile(tempPath);
    if (!tmpFile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        return;
    }

    QTextStream out(&tmpFile);
    for (int i = 0; i < dbCopy.size(); ++i) {
        if (m_cancel.load()) {
            tmpFile.close();
            tmpFile.remove();
            return;
        }
        out << dbCopy[i].serializePGN() << "\n\n";
        out.flush();
    }
    tmpFile.close();

    QFile::remove(m_pendingPath);
    tmpFile.rename(m_pendingPath);
}
