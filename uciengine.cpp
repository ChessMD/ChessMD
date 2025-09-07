/*
April 11, 2025: File Creation
*/

#include "uciengine.h"
#include <QTextStream>
#include <qregularexpression.h>

UciEngine::UciEngine(QObject *parent)
    : QObject(parent)
    , m_proc(new QProcess(this))
{
    m_proc->setProcessChannelMode(QProcess::MergedChannels);
    connect(m_proc, &QProcess::readyReadStandardOutput, this, &UciEngine::handleReadyRead);
}

UciEngine::~UciEngine() {
    blockSignals(true);
    disconnect(this, nullptr, nullptr, nullptr);
    if (m_proc && m_proc->state() != QProcess::NotRunning) {
        m_proc->blockSignals(true);
        sendCommand("quit\n");
        m_proc->waitForFinished(500);
    }
}

void UciEngine::startEngine(const QString &binaryPath) {
    m_proc->start(binaryPath);
    sendCommand("uci\n");
    sendCommand("isready\n");
}


void UciEngine::requestReady() {
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
    if (!m_ready) return;
    setOption("MultiPV", QString::number(maxMultiPV));
    sendCommand("go infinite\n");
    m_ready = false;
}

void UciEngine::stopSearch() {
    sendCommand("stop\n");
}


void UciEngine::goMovetime(int milliseconds) {
    requestReady();
    sendCommand(QString("go movetime %1\n").arg(milliseconds));
}

void UciEngine::setSkillLevel(int level) {
    setOption("Skill Level", QString::number(level));
}

void UciEngine::setLimitStrength(bool enabled) {
    setOption("UCI_LimitStrength", enabled ? "true" : "false");
}

void UciEngine::goWithClocks(int wtime_ms, int btime_ms, int winc_ms, int binc_ms) {
    if (!m_ready) return;
    sendCommand(QString("go wtime %1 btime %2 winc %3 binc %4\n").arg(wtime_ms).arg(btime_ms).arg(winc_ms).arg(binc_ms));
    m_ready = false;
}

void UciEngine::sendRawCommand(const QString &cmd) {
    sendCommand(cmd + "\n");
}

void UciEngine::handleReadyRead() {
    while (m_proc->canReadLine()) {
        QString line = QString::fromUtf8(m_proc->readLine()).trimmed();
        emit infoReceived(line);

        if (line == "readyok"){
            m_ready = true;
            continue;
        }

        // bestmove
        if (line.startsWith("bestmove ")) {
            auto parts = line.split(' ', Qt::SkipEmptyParts);
            if (parts.size() >= 2)
                emit bestMove(parts[1]);
            continue;
        }

        // tokenize by whitespace
        QStringList toks = line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);

        int depth = -1, multipv = 1;
        bool isMate = false;
        double score = 0.0;
        QString pvLine;
        for (int i = 0; i < toks.size(); ++i) {
            const QString &tk = toks[i];
            if (tk == "depth" && i+1 < toks.size()) {
                depth = toks[i+1].toInt();
                ++i;
            }
            else if (tk == "multipv" && i+1 < toks.size()) {
                multipv = toks[i+1].toInt();
                ++i;
            }
            else if (tk == "score" && i+2 < toks.size()) {
                const QString &typ = toks[i+1];
                const QString &num = toks[i+2];
                if (typ == "cp") {
                    isMate = false;
                    score = num.toInt() / 100.0;
                } else if (typ == "mate") {
                    isMate = true;
                    score = num.toInt();
                }
                i += 2;
            }
            else if (tk == "pv" && i+1 < toks.size()) {
                // rest of tokens form the PV
                pvLine = toks.mid(i+1).join(' ');
                break;
            }
        }

        // emit an update if we found both PV and depth
        if (depth >= 0 && multipv >= 1 && !pvLine.isEmpty()) {
            PvInfo info;
            info.depth   = depth;
            info.multipv = multipv;
            info.isMate  = isMate;
            info.score   = score;
            info.pvLine  = pvLine;
            emit pvUpdate(info);
        }
    }
}
