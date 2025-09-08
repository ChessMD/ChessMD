import QtQuick 2.15
import QtQuick.Controls 2.15

pragma ComponentBehavior: Bound

Rectangle {
    id: recRoot
    anchors.fill: parent
    color: "#3f538a" /*hcc*/

    property int padding: 16

    property real evalRatio: {
        if (!chessPosition) return 0.5;
        return Math.min( Math.max(chessPosition.evalScore * ((0.95 - 0.05) / 8.0) + 0.5, 0.05), 0.95)
    }

    // dark background track
    Rectangle {
        id: evalBarBg
        anchors {
            top: parent.top
            bottom: parent.bottom
            left: parent.left
            margins: padding
        }
        width: 20
        color: "#000000" /*hcc*/
        radius: width / 2
    }

    // white fill bar
    Rectangle {
        id: evalBarFill
        anchors {
            bottom: evalBarBg.bottom
        }
        width: evalBarBg.width - 4
        x: evalBarBg.x + 2
        color: "#ffffff" /*hcc*/
        radius: width / 2

        height: {
            if (!chessPosition) return 0.5;
            return evalBarBg.height * Math.min(Math.max(chessPosition.evalScore * 0.1125 + 0.5, 0.05), 0.95);
        }

        Behavior on height {
            NumberAnimation { duration: 300 }
        }
    }

    // a full‑screen mouse area to catch “cancel” taps
    MouseArea {
        id: promoCatcher
        anchors.fill: parent
        onClicked: {
            promoPopup.close()
        }
    }

    property int promoSr; property int promoSc
    property int promoDr; property int promoDc

    Connections {
        target: chessPosition
        function onRequestPromotion(sr, sc, dr, dc) {
            promoSr = sr
            promoSc = sc
            promoDr = dr
            promoDc = dc
            promoPopup.open()
        }
    }
    Connections {
        target: chessPosition
        function onBoardDataChanged() {
            promoPopup.close()
        }
    }

    Popup {
        id: promoPopup
        modal: true
        focus: true
        padding: 0
        x: board.x + promoDc*board.cellSize
        y: {
            if (promoDr == 0)
                return board.y
            else if (promoDr == 7)
                return board.y + 4*board.cellSize
        }
        width: board.cellSize
        height: 4*board.cellSize
        background: Rectangle {
            color: "#FFFFFF" /*hcc*/
            border.color: "#000000" /*hcc*/
            border.width: 4
        }

        property var promoPiece: ["Q", "N", "R", "B"]

        Item {
            anchors.fill: parent
            Repeater {
                id: promoContainer
                model: 4
                Rectangle {
                    id: promoSquare
                    x: 0
                    y: index*board.cellSize
                    width: board.cellSize
                    height: board.cellSize

                    required property int index

                    Image {
                        anchors.fill: parent
                        // Svg Rasterisation
                        sourceSize.width: parent.width
                        sourceSize.height: parent.height
                        property int index
                        source: {
                            if (promoDr == 0)
                                return "img/piece/alpha/" + "w" + promoPopup.promoPiece[promoSquare.index] + ".svg"
                            else
                                return "img/piece/alpha/" + "b" + promoPopup.promoPiece[3-promoSquare.index] + ".svg"
                        }
                        fillMode: Image.PreserveAspectFit
                    }

                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            chessPosition.promote(promoSr, promoSc, promoDr, promoDc, promoPopup.promoPiece[(promoDr == 0 ? promoSquare.index : 3 - promoSquare.index)]);
                            promoPopup.close();
                        }
                    }
                }
            }
        }
        Rectangle {
            anchors.fill: parent
            color: "transparent"
            border.width: 2
            border.color: "#000000" /*hcc*/
            z: 100
        }
    }

    Item {
        id: board
        width: {
            return Math.min(parent.width - (padding + evalBarBg.width + padding)*2, parent.height - (padding)*2)
        }
        height: width
        anchors.centerIn: parent

        property real cellSize: width / 8
        property int dragOrigin: -1
        property var boardData: chessPosition ? chessPosition.boardData : [["", "", "", "", "", "", "", ""],["", "", "", "", "", "", "", ""],["", "", "", "", "", "", "", ""],["", "", "", "", "", "", "", ""],["", "", "", "", "", "", "", ""],["", "", "", "", "", "", "", ""],["", "", "", "", "", "", "", ""],["", "", "", "", "", "", "", ""]]

        Repeater {
            id: squareContainer
            model: 64

            Rectangle {
                id: square
                width: board.cellSize
                height: board.cellSize

                required property int index
                property int row: Math.floor(index / 8)
                property int col: index % 8

                color: ((Math.floor(index / 8) + (index % 8)) % 2 === 0) ? "#EEE" : "#999999" /*hcc*/
                x: col * board.cellSize
                y: row * board.cellSize

                // file coordinates bottom
                Text {
                    anchors.bottom: parent.bottom
                    anchors.right: parent.right
                    anchors.margins: 2
                    text: String.fromCharCode(97 + col) // a-h
                    font.pixelSize: board.cellSize * 0.2
                    font.weight: Font.Bold
                    color: ((Math.floor(index / 8) + (index % 8)) % 2 === 0) ? "#999999" : "#EEE" /*hcc*/
                    opacity: 0.8
                    visible: row === 7 // Only show on bottom rank
                }

                // rank coordinates left
                Text {
                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.margins: 2
                    text: String(8 - row) // 8-1
                    font.pixelSize: board.cellSize * 0.2
                    font.weight: Font.Bold
                    color: ((Math.floor(index / 8) + (index % 8)) % 2 === 0) ? "#999999" : "#EEE" /*hcc*/
                    opacity: 0.8
                    visible: col === 0 // Only show on left file
                }

                Rectangle {  // highlight
                    anchors.fill: parent
                    color: {
                        if (!chessPosition) return "00000000";
                        return chessPosition.isPreview ? "yellow" : "blue" //hcc
                    }
                    opacity: 0.2

                    // decode the one property:
                    visible: {
                        if (!chessPosition) return false;
                        var lm = chessPosition.lastMove;
                        if (lm < 0) return false;
                        var fromIdx = lm >> 8;
                        var toIdx   = lm & 0xFF;
                        var thisIdx = row*8 + col;
                        return thisIdx === fromIdx || thisIdx === toIdx;
                    }
                    z: 5
                }
            }
        }

        Repeater {
            id: pieceContainer
            model: 64

            Rectangle {
                id: piece
                width: board.cellSize
                height: board.cellSize
                color: "transparent"

                required property int index
                property int row: Math.floor(index / 8)
                property int col: index % 8

                x: col * board.cellSize
                y: row * board.cellSize
                z: pieceTouch.pressed ? 100 : 1

                Image {
                    // Svg Rasterisation
                    sourceSize.width: parent.width
                    sourceSize.height: parent.height

                    width: parent.width * 0.95
                    height: parent.height * 0.95

                    source: board.boardData[piece.row][piece.col] !== "" ? "img/piece/alpha/" + board.boardData[piece.row][piece.col] + ".svg" : ""

                    fillMode: Image.PreserveAspectFit

                    anchors.centerIn: pieceTouch.pressed ? undefined : parent
                    x:  pieceTouch.pressed ? pieceTouch.mouseX - (width / 2) : 0
                    y:  pieceTouch.pressed ? pieceTouch.mouseY - (height / 2) : 0
                }

                MouseArea {
                    id: pieceTouch
                    anchors.fill: piece

                    property int hoverCol: Math.floor((piece.x + mouseX) / board.cellSize)
                    property int hoverRow: Math.floor((piece.y + mouseY) / board.cellSize)

                    cursorShape: board.dragOrigin >= 0 ? Qt.ClosedHandCursor : (board.boardData[row][col] !== ""  ? Qt.PointingHandCursor : Qt.ArrowCursor)

                    onPressed: {
                        board.dragOrigin = index;
                    }

                    onReleased: {
                        board.dragOrigin = -1;
                        if (chessPosition)
                            chessPosition.release(piece.row, piece.col, hoverRow, hoverCol);
                    }

                    onPositionChanged: {
                        // Update the hover position on every mouse move
                        hoverCol = Math.floor((piece.x + mouseX) / board.cellSize)
                        hoverRow = Math.floor((piece.y + mouseY) / board.cellSize)
                    }
                }
            }
        }

    }
}
