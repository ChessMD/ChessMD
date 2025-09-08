#ifndef GAMEPLAYVIEWER_H
#define GAMEPLAYVIEWER_H

#include <QWidget>
#include <QTimer>
#include <QSharedPointer>
#include <QComboBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QPushButton>
#include <QButtonGroup>
#include <QLabel>

#include "chessposition.h"
#include "uciengine.h"

class GameplayViewer : public QWidget {
    Q_OBJECT
public:
    explicit GameplayViewer(ChessPosition *positionViewer, QWidget *parent = nullptr);

    void resetPlay();

public slots:
    void onBoardMoveMade(QSharedPointer<NotationMove>& move);

signals:
    void openAnalysisBoard();
    void resetBoard();

private slots:
    void onPlayClicked();
    void onResignClicked();
    void onPlayAgainClicked();
    void onOpenInAnalysisClicked();

    void onEngineBestMove(const QString &uci);
    void onEngineInfo(const QString &line);

    void onClockTick();

private:
    void startEngineProcess();
    void stopEngineProcess();
    bool applyUciMove(const QString &uci);
    void updateClockDisplays();
    void scheduleNextDisplayUpdate();
    void turnFinished();
    bool isPlayersTurn() const;
    void finishGame(const QString &result);

    ChessPosition *m_positionViewer;
    UciEngine *m_engine;

    // UI elements (created in-code to avoid extra UI file)
    QWidget *m_root;
    QWidget *m_controlsWidget;
    QWidget *m_preGameWidget;
    QWidget *m_inGameWidget;
    QWidget *m_moveNotationWidget;

    // controls
    QButtonGroup *m_sideButtonGroup;
    QPushButton *m_whiteBtn;
    QPushButton *m_blackBtn;
    QPushButton *m_randomBtn;
    QSpinBox *m_minutesSpin;
    QSpinBox *m_secondsSpin;
    QSpinBox *m_incrementSpin;
    QSlider *m_eloSlider;
    QSpinBox *m_eloSpin;
    QPushButton *m_playBtn;
    QPushButton *m_resignBtn;
    QPushButton *m_playAgainBtn;
    QPushButton *m_openAnalysisBtn;
    QLabel *m_whitePlayerLabel;
    QLabel *m_blackPlayerLabel;
    QLabel *m_whiteClock;
    QLabel *m_blackClock;

    // clocks in ms
    int m_whiteMs;
    int m_blackMs;
    int m_incMs;

    int m_humanSide; // 0 = white, 1 = black
    bool m_active;

    QTimer m_updateTimer;
    QElapsedTimer m_clockTimer;
};

#endif // GAMEPLAYVIEWER_H
