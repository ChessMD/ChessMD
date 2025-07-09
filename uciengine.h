/*
April 11, 2025: File Creation
*/

#ifndef UCIENGINE_H
#define UCIENGINE_H

#include <QObject>
#include <QProcess>

struct PvInfo {
    int    depth;
    int    multipv;
    bool   isMate;
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
        emit commandSent(cmd.trimmed());
    }
};

#endif // UCIENGINE_H

