/*
April 11, 2025: File Creation
*/

#include "uciengine.h"
#include <QTextStream>
#include <QFileInfo>

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
        sendCommand("quit");
        m_proc->waitForFinished(500);
    }
}

void UciEngine::startEngine(const QString &binaryPath) {
    QFileInfo fileInfo(binaryPath);
    if (!fileInfo.exists()) return;
    m_proc->start(binaryPath);
    sendCommand("uci", false);
    uciNewGame();
}

void UciEngine::sendCommand(const QString &cmd, bool requireReady) {
    if (!m_proc || m_proc->state() == QProcess::NotRunning || (requireReady && !m_ready)) return;
    m_proc->write((cmd+'\n').toUtf8());
    emit commandSent(cmd.trimmed());

    qDebug() << cmd << "sent";
}

void UciEngine::requestReady() {
    sendCommand("isready");
}

void UciEngine::quitEngine() {
    if (m_proc->state() != QProcess::NotRunning) {
        sendCommand("quit");
        m_proc->waitForFinished(500);
    }
}

void UciEngine::setOption(const QString &name, const QString &value) {
    sendCommand(QString("setoption name %1 value %2").arg(name, value));
}

void UciEngine::setPosition(const QString &fen) {
    if (fen == "startpos") sendCommand("position startpos");
    else sendCommand(QString("position fen %1").arg(fen));
}

void UciEngine::startInfiniteSearch(int maxMultiPV) {
    stopSearch();
    setOption("MultiPV", QString::number(maxMultiPV));
    sendCommand("go infinite");
}

void UciEngine::stopSearch() {
    sendCommand("stop");
}

void UciEngine::goMovetime(int milliseconds) {
    stopSearch();
    sendCommand(QString("go movetime %1").arg(milliseconds));
}

void UciEngine::setSkillLevel(int level) {
    setOption("Skill Level", QString::number(level));
}

void UciEngine::setLimitStrength(bool enabled) {
    setOption("UCI_LimitStrength", enabled ? "true" : "false");
}

void UciEngine::goDepthWithClocks(int depth, int whiteMs, int blackMs, int whiteIncMs, int blackIncMs) {
    stopSearch();
    sendCommand(QString("go depth %1 wtime %2 btime %3 winc %4 binc %5").arg(depth).arg(whiteMs).arg(blackMs).arg(whiteIncMs).arg(blackIncMs));
}

void UciEngine::goDepth(int depth) {
    stopSearch();
    sendCommand(QString("go depth %1").arg(depth));
}

void UciEngine::uciNewGame() {
    sendCommand("ucinewgame");
    m_ready = false;
    sendCommand("isready", false);
}

void UciEngine::handleReadyRead() {
    while (m_proc->canReadLine()) {
        QString line = QString::fromUtf8(m_proc->readLine()).trimmed();
        emit infoReceived(line);
        if (line == "readyok"){
            m_ready = true;
            emit engineReady();
            emit engineReady();
            continue;
        }

        if (line.startsWith("id name ")) {
            // everything after "id name " is the engine's name
            QString name = line.mid(QStringLiteral("id name ").length());
            emit nameReceived(name);
        }

        // bestmove
        if (line.startsWith("bestmove ")) {
            auto parts = line.split(' ', Qt::SkipEmptyParts);
            if (parts.size() >= 2)
                emit bestMove(parts[1]);
            continue;
        }

        // tokenize by whitespace
        QStringList toks = line.simplified().split(' ', Qt::SkipEmptyParts);

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
            info.depth = depth;
            info.multipv = multipv;
            info.isMate = isMate;
            info.score = score;
            info.pvLine = pvLine;
            emit pvUpdate(info);
        }
    }
}
