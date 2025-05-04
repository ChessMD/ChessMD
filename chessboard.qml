import QtQuick 2.15

pragma ComponentBehavior: Bound

Rectangle {
    id: recRoot
    anchors.fill: parent
    color: "#3f538a"

    property int padding: 16

    Item {
        id: board
        anchors.centerIn: parent
        width: Math.min(recRoot.width - recRoot.padding * 2, recRoot.height - recRoot.padding * 2)
        height: width  // keep board square

        property real cellSize: width / 8
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

                color: ((Math.floor(index / 8) + (index % 8)) % 2 === 0) ? "#EEE" : "#999999"
                x: col * board.cellSize
                y: row * board.cellSize
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

                Image {
                    // Svg Rasterisation
                    sourceSize.width: parent.width
                    sourceSize.height: parent.height

                    width: parent.width * 0.95
                    height: parent.height * 0.95

                    source: board.boardData[piece.row][piece.col] !== ""
                            ? "img/piece/alpha/" + board.boardData[piece.row][piece.col] + ".svg"
                            : ""

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

                    // onClicked: {
                    //     console.log("Board data:", chessPosition.boardData);
                    // }

                    onReleased: {
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
