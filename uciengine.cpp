/*
April 11, 2025: File Creation
*/

#include "uciengine.h"
#include <QTextStream>

UciEngine::UciEngine(QObject *parent) : QObject(parent), m_proc(new QProcess(this))
{
    m_proc->setProcessChannelMode(QProcess::MergedChannels);
    connect(m_proc, &QProcess::readyReadStandardOutput, this, &UciEngine::handleReadyRead);
}

UciEngine::~UciEngine() {
    if (m_proc->state() != QProcess::NotRunning) {
        m_proc->write("quit\n");
        m_proc->waitForFinished(500);
    }
}

void UciEngine::startEngine(const QString &binaryPath) {
    m_proc->start(binaryPath);
    m_proc->write("uci\n");
    m_proc->write("isready\n");
}

void UciEngine::setPosition(const QString &fen) {
    QByteArray cmd = "position fen " + fen.toUtf8() + "\n";
    m_proc->write(cmd);
}

void UciEngine::goDepth(int depth) {
    QByteArray cmd = "go depth " + QByteArray::number(depth) + "\n";
    m_proc->write(cmd);
}

void UciEngine::stop() {
    m_proc->write("stop\n");
}

void UciEngine::handleReadyRead() {
    while (m_proc->canReadLine()) {
        QString line = QString::fromUtf8(m_proc->readLine()).trimmed();
        emit infoReceived(line);
        if (line.startsWith("bestmove ")) {
            auto parts = line.split(' ');
            if (parts.size() >= 2)
                emit bestMove(parts.at(1));
        }
    }
}
