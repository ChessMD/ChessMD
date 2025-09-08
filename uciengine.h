/*
April 11, 2025: File Creation
*/

#ifndef UCIENGINE_H
#define UCIENGINE_H

#include <QObject>
#include <QProcess>
#include <QDebug>

struct PvInfo {
    int depth;
    int multipv;
    bool isMate;
    bool positive;
    double score;
    QString pvLine;
};

class UciEngine : public QObject {
    Q_OBJECT
public:
    explicit UciEngine(QObject *parent = nullptr);
    ~UciEngine();

    void startEngine(const QString &binaryPath = QStringLiteral("/usr/bin/stockfish"));
    void quitEngine();
    void requestReady();

    void setOption(const QString &name, const QString &value);
    void setPosition(const QString &fen);

    void startInfiniteSearch(int maxMultiPV = 1);
    void stopSearch();

    void goMovetime(int milliseconds);

    void setSkillLevel(int level);
    void setLimitStrength(bool enabled);
    void goWithClocks(int wtime_ms, int btime_ms, int winc_ms = 0, int binc_ms = 0);
    void sendRawCommand(const QString &cmd);

signals:
    void commandSent(const QString &cmd);
    void infoReceived(const QString &rawInfo);
    void bestMove(const QString &move);
    void pvUpdate(PvInfo &info);

private slots:
    void handleReadyRead();

private:
    QProcess *m_proc;
    bool m_ready = false;

    // helper to write+emit
    void sendCommand(const QString &cmd) {
        m_proc->write(cmd.toUtf8());
        qDebug() << cmd << "sent";
        emit commandSent(cmd.trimmed());
    }

    bool m_hasPendingGo = false;
    int m_pending_wtime = 0;
    int m_pending_btime = 0;
    int m_pending_winc = 0;
    int m_pending_binc = 0;
};

#endif // UCIENGINE_H

