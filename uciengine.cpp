/*
April 11, 2025: File Creation
*/

#include "uciengine.h"
#include <QTextStream>

UciEngine::UciEngine(QObject *parent)
    : QObject(parent)
    , m_proc(new QProcess(this))
    , m_rePv(R"(^info\s+depth\s+(\d+).*?multipv\s+(\d+).*?score\s+(cp|mate)\s+(-?\d+).*?pv\s+(.+)$)")
{
    m_proc->setProcessChannelMode(QProcess::MergedChannels);
    connect(m_proc, &QProcess::readyReadStandardOutput,
            this,   &UciEngine::handleReadyRead);
}

UciEngine::~UciEngine() {
    UciEngine();
}

void UciEngine::startEngine(const QString &binaryPath) {
    m_proc->start(binaryPath);
    sendCommand("uci\n");
    sendCommand("isready\n");
}

void UciEngine::quitEngine() {
    if (m_proc->state() != QProcess::NotRunning) {
        sendCommand("quit\n");
        m_proc->waitForFinished(500);
    }
}

void UciEngine::setOption(const QString &name, const QString &value) {
    sendCommand(QString("setoption name %1 value %2\n").arg(name, value));
}

void UciEngine::setPosition(const QString &fen) {
    sendCommand(QString("position fen %1\n").arg(fen));
}

void UciEngine::startInfiniteSearch(int maxMultiPV) {
    setOption("MultiPV", QString::number(maxMultiPV));
    sendCommand("go infinite\n");
}

void UciEngine::stopSearch() {
    sendCommand("stop\n");
}

void UciEngine::handleReadyRead() {
    while (m_proc->canReadLine()) {
        QString line = QString::fromUtf8(m_proc->readLine()).trimmed();
        emit infoReceived(line);

        if (line.startsWith("bestmove ")) {
            auto parts = line.split(' ');
            if (parts.size() >= 2)
                emit bestMove(parts[1]);
        }

        auto m = m_rePv.match(line);
        if (m.hasMatch()) {
            PvInfo info;
            info.depth   = m.captured(1).toInt();
            info.multipv = m.captured(2).toInt();
            info.isMate  = (m.captured(3) == "mate");
            int raw      = m.captured(4).toInt();
            info.score   = info.isMate ? raw : raw / 100.0;
            info.pvLine  = m.captured(5);
            emit pvUpdate(info);
        }
    }
}

