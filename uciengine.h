/*
April 11, 2025: File Creation
*/

#ifndef UCIENGINE_H
#define UCIENGINE_H

#include <QObject>
#include <QProcess>

// Connection to UCI engine
class UciEngine : public QObject {
    Q_OBJECT
public:
    explicit UciEngine(QObject *parent = nullptr);
    ~UciEngine();

    void startEngine(const QString &binaryPath = QStringLiteral("/usr/bin/stockfish"));

    void setPosition(const QString &fen);

    void goDepth(int depth);

    void stop();

signals:
    void infoReceived(const QString &info);

    void bestMove(const QString &move);

private slots:
    void handleReadyRead();

private:
    QProcess *m_proc;
};

#endif // UCIENGINE_H
