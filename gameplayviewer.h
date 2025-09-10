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
    bool isEngineIdle(){ return m_engineIdle; };

public slots:
    void onBoardMoveMade(QSharedPointer<NotationMove>& move);
    void updateClockDisplays();

signals:
    void openAnalysisBoard();
    void resetBoard();
    void matchBoardFlip(QChar side);
    void selectLastMove();
    void requestTakeback(QChar side);

private slots:
    void onPlayClicked(int selectedSide);
    void onResignClicked();
    void onReturnClicked();
    void onRematchClicked();
    void onOpenInAnalysisClicked();
    void onTakebackClicked();

    void onEngineBestMove(const QString &uci);
    void onEngineInfo(const QString &line);

    void onClockTick();

private:
    void startEngineProcess();
    void stopEngineProcess();
    void resetPlay();
    bool applyUciMove(const QString &uci);
    void scheduleNextDisplayUpdate();
    void turnFinished();
    void updateTakebackEnabled();
    bool isPlayersTurn() const;
    void finishGame(const QString &result, const QString &description);

    ChessPosition *m_positionViewer;
    UciEngine *m_engine;
    QMetaObject::Connection m_engineReadyConn;
    ChessPosition *m_lastPosition;
    ChessPosition *m_startPosition;

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
    QLabel *m_engineLabel;
    QPushButton *m_selectEngineBtn;
    QCheckBox *m_timeCheck;
    QSpinBox *m_minutesSpin;
    QSpinBox *m_secondsSpin;
    QSpinBox *m_incrementSpin;
    QWidget *m_timeWidget;
    QSlider *m_eloSlider;
    QSpinBox *m_eloSpin;
    QPushButton *m_playBtn;
    QPushButton *m_resignBtn;
    QPushButton *m_returnBtn;
    QPushButton *m_openAnalysisBtn;
    QPushButton *m_takebackBtn;
    QLabel *m_whitePlayerLabel;
    QLabel *m_blackPlayerLabel;
    QLabel *m_whiteClock;
    QLabel *m_blackClock;

    // clocks in ms
    int m_whiteMs;
    int m_blackMs;
    int m_incMs;
    int m_engineDepth;

    int m_moveCount;
    int m_humanSide; // 0 = white, 1 = black
    bool m_engineIdle;
    bool m_active;

    QTimer m_updateTimer;
    QElapsedTimer m_clockTimer;
};

#endif // GAMEPLAYVIEWER_H
