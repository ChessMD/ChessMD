import QtQuick 2.15
import QtQuick.Controls 2.15

pragma ComponentBehavior: Bound

Rectangle {
    SystemPalette {id: palette}

    id: root
    width: 580
    height: 480
    color: palette.base 
    
    property int padding: 16
    
    // Global drag images for palette pieces
    Image {
        id: globalDragPiece
        width: 32
        height: 32
        sourceSize.width: 32
        sourceSize.height: 32
        fillMode: Image.PreserveAspectFit
        visible: false
        z: 2000
        
        property string pieceType: ""
    }
    
    function getPieceSymbol(pieceCode) {
        if (!pieceCode || pieceCode === "") return ""
        
        // Convert piece code like "wK" to Unicode symbol
        const symbols = {
            'wK': '♔', 'wQ': '♕', 'wR': '♖', 'wB': '♗', 'wN': '♘', 'wP': '♙',
            'bK': '♚', 'bQ': '♛', 'bR': '♜', 'bB': '♝', 'bN': '♞', 'bP': '♟'
        }
        return symbols[pieceCode] || pieceCode
    }

    Item {
        id: board
        width: Math.min(parent.height - padding*2, parent.width - 120 - padding*2) // Reserve 120px for buttons
        height: width
        anchors.left: parent.left
        anchors.leftMargin: padding
        anchors.verticalCenter: parent.verticalCenter

        property real cellSize: width / 8
        property int dragOrigin: -1
        property var boardData: chessPosition ? chessPosition.boardData : [
            ["", "", "", "", "", "", "", ""],
            ["", "", "", "", "", "", "", ""],
            ["", "", "", "", "", "", "", ""],
            ["", "", "", "", "", "", "", ""],
            ["", "", "", "", "", "", "", ""],
            ["", "", "", "", "", "", "", ""],
            ["", "", "", "", "", "", "", ""],
            ["", "", "", "", "", "", "", ""]
        ]

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

                color: ((Math.floor(index / 8) + (index % 8)) % 2 === 0) ? "#f0d9b5" : "#b58863" //hcc
                x: col * board.cellSize
                y: row * board.cellSize

                // File coordinates (a-h) on bottom rank
                Text {
                    anchors.bottom: parent.bottom
                    anchors.right: parent.right
                    anchors.margins: 2
                    text: String.fromCharCode(97 + col) // a-h
                    font.pixelSize: 10
                    font.weight: Font.Bold
                    color: ((Math.floor(index / 8) + (index % 8)) % 2 === 0) ? "#b58863" : "#f0d9b5" //hcc
                    opacity: 0.8
                    visible: row === 7 // Only show on bottom rank
                }

                // Rank coordinates (1-8) on left file
                Text {
                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.margins: 2
                    text: String(8 - row) // 8-1
                    font.pixelSize: 10
                    font.weight: Font.Bold
                    color: ((Math.floor(index / 8) + (index % 8)) % 2 === 0) ? "#b58863" : "#f0d9b5" //hcc
                    opacity: 0.8
                    visible: col === 0 // Only show on left file
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

                Text {
                    id: pieceText
                    anchors.centerIn: pieceTouch.pressed ? undefined : parent
                    x: pieceTouch.pressed ? pieceTouch.mouseX - (width / 2) : 0
                    y: pieceTouch.pressed ? pieceTouch.mouseY - (height / 2) : 0
                    
                    text: root.getPieceSymbol(board.boardData[piece.row] && board.boardData[piece.row][piece.col] ? board.boardData[piece.row][piece.col] : "")
                    font.pixelSize: 32
                    color: "black" //hcc
                    visible: false // Hide text when using images
                }

                Image {
                    id: pieceImage
                    anchors.centerIn: pieceTouch.pressed ? undefined : parent
                    x: pieceTouch.pressed ? pieceTouch.mouseX - (width / 2) : 0
                    y: pieceTouch.pressed ? pieceTouch.mouseY - (height / 2) : 0
                    
                    // Svg Rasterisation
                    sourceSize.width: parent.width
                    sourceSize.height: parent.height
                    
                    width: parent.width * 0.95
                    height: parent.height * 0.95
                    
                    source: {
                        const pieceCode = board.boardData[piece.row] && board.boardData[piece.row][piece.col] ? board.boardData[piece.row][piece.col] : ""
                        return pieceCode !== "" ? "img/piece/alpha/" + pieceCode + ".svg" : ""
                    }
                    
                    fillMode: Image.PreserveAspectFit
                    visible: source !== ""
                }

                MouseArea {
                    id: pieceTouch
                    anchors.fill: piece

                    property int hoverCol: Math.floor((piece.x + mouseX) / board.cellSize)
                    property int hoverRow: Math.floor((piece.y + mouseY) / board.cellSize)

                    cursorShape: board.dragOrigin >= 0 ? Qt.ClosedHandCursor : (board.boardData[row] && board.boardData[row][col] && board.boardData[row][col] !== "" ? Qt.PointingHandCursor : Qt.ArrowCursor)

                    onPressed: {
                        if (board.boardData[row] && board.boardData[row][col] && board.boardData[row][col] !== "") {
                            board.dragOrigin = index;
                        }
                    }

                    onReleased: {
                        if (board.dragOrigin >= 0 && chessPosition) {
                            const fromRow = Math.floor(board.dragOrigin / 8)
                            const fromCol = board.dragOrigin % 8
                            const toRow = hoverRow
                            const toCol = hoverCol
                            
                            var newBoard = JSON.parse(JSON.stringify(chessPosition.boardData))
                            
                            if (toRow >= 0 && toRow < 8 && toCol >= 0 && toCol < 8) {
                                // Move piece on board using setup logic (not chess rules)
                                var movingPiece = newBoard[fromRow][fromCol]
                                newBoard[fromRow][fromCol] = ""  // Clear source
                                newBoard[toRow][toCol] = movingPiece  // Place at destination
                                chessPosition.boardData = newBoard
                            } else {
                                // Dragged outside board bounds - delete the piece
                                newBoard[fromRow][fromCol] = ""  // Clear source
                                chessPosition.boardData = newBoard
                            }
                        }
                        board.dragOrigin = -1;
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
    
    // Right side panel with piece palette and buttons
    Column {
        anchors.right: parent.right
        anchors.rightMargin: padding
        anchors.verticalCenter: parent.verticalCenter
        spacing: 10
        
        // Piece palette
        Rectangle {
            width: 100
            height: 260
            color: palette.window
            border.color: palette.mid
            border.width: 1
            radius: 5
            
            Column {
                anchors.fill: parent
                anchors.margins: 5
                spacing: 3
                
                Text {
                    text: "Pieces"
                    font.pixelSize: 12
                    font.weight: Font.Bold
                    color: palette.text
                    anchors.horizontalCenter: parent.horizontalCenter
                }
                
                // White pieces
                Grid {
                    columns: 2
                    spacing: 3
                    anchors.horizontalCenter: parent.horizontalCenter
                    
                    property var whitePieces: ["wK", "wQ", "wR", "wB", "wN", "wP"]
                    
                    Repeater {
                        model: parent.whitePieces
                        
                        Rectangle {
                            width: 35
                            height: 35
                            color: "#f0d9b5"
                            border.color: palette.mid
                            border.width: 1
                            radius: 3
                            
                            required property string modelData
                            property string pieceType: modelData
                            
                            Image {
                                anchors.centerIn: parent
                                width: 28
                                height: 28
                                sourceSize.width: 28
                                sourceSize.height: 28
                                source: "img/piece/alpha/" + parent.modelData + ".svg"
                                fillMode: Image.PreserveAspectFit
                                visible: true
                            }
                            
                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                
                                property bool isDragging: false
                                
                                onPressed: {
                                    isDragging = true
                                    globalDragPiece.pieceType = parent.modelData
                                    globalDragPiece.source = "img/piece/alpha/" + parent.modelData + ".svg"
                                    globalDragPiece.visible = true
                                    
                                    var globalPos = mapToItem(root, mouseX, mouseY)
                                    globalDragPiece.x = globalPos.x - globalDragPiece.width/2
                                    globalDragPiece.y = globalPos.y - globalDragPiece.height/2
                                }
                                
                                onPositionChanged: {
                                    if (isDragging) {
                                        var globalPos = mapToItem(root, mouseX, mouseY)
                                        globalDragPiece.x = globalPos.x - globalDragPiece.width/2
                                        globalDragPiece.y = globalPos.y - globalDragPiece.height/2
                                    }
                                }
                                
                                onReleased: {
                                    if (isDragging) {
                                        // Check if dropped on board
                                        var boardPos = mapToItem(board, mouseX, mouseY)
                                        var dropCol = Math.floor(boardPos.x / board.cellSize)
                                        var dropRow = Math.floor(boardPos.y / board.cellSize)
                                        
                                        if (dropCol >= 0 && dropCol < 8 && dropRow >= 0 && dropRow < 8 && 
                                            boardPos.x >= 0 && boardPos.y >= 0 && 
                                            boardPos.x < board.width && boardPos.y < board.height) {
                                            // Place piece on board
                                            var newBoard = JSON.parse(JSON.stringify(chessPosition.boardData))
                                            newBoard[dropRow][dropCol] = parent.modelData
                                            chessPosition.boardData = newBoard
                                        }
                                        
                                        // Hide drag piece
                                        globalDragPiece.visible = false
                                    }
                                    isDragging = false
                                }
                            }
                        }
                    }
                }
                
                // Black pieces
                Grid {
                    columns: 2
                    spacing: 3
                    anchors.horizontalCenter: parent.horizontalCenter
                    
                    property var blackPieces: ["bK", "bQ", "bR", "bB", "bN", "bP"]
                    
                    Repeater {
                        model: parent.blackPieces
                        
                        Rectangle {
                            width: 35
                            height: 35
                            color: "#b58863"
                            border.color: palette.mid
                            border.width: 1
                            radius: 3
                            
                            required property string modelData
                            property string pieceType: modelData
                            
                            Image {
                                anchors.centerIn: parent
                                width: 28
                                height: 28
                                sourceSize.width: 28
                                sourceSize.height: 28
                                source: "img/piece/alpha/" + parent.modelData + ".svg"
                                fillMode: Image.PreserveAspectFit
                                visible: true
                            }
                            
                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                
                                property bool isDragging: false
                                
                                onPressed: {
                                    isDragging = true
                                    globalDragPiece.pieceType = parent.modelData
                                    globalDragPiece.source = "img/piece/alpha/" + parent.modelData + ".svg"
                                    globalDragPiece.visible = true
                                    
                                    var globalPos = mapToItem(root, mouseX, mouseY)
                                    globalDragPiece.x = globalPos.x - globalDragPiece.width/2
                                    globalDragPiece.y = globalPos.y - globalDragPiece.height/2
                                }
                                
                                onPositionChanged: {
                                    if (isDragging) {
                                        var globalPos = mapToItem(root, mouseX, mouseY)
                                        globalDragPiece.x = globalPos.x - globalDragPiece.width/2
                                        globalDragPiece.y = globalPos.y - globalDragPiece.height/2
                                    }
                                }
                                
                                onReleased: {
                                    if (isDragging) {
                                        // Check if dropped on board
                                        var boardPos = mapToItem(board, mouseX, mouseY)
                                        var dropCol = Math.floor(boardPos.x / board.cellSize)
                                        var dropRow = Math.floor(boardPos.y / board.cellSize)
                                        
                                        if (dropCol >= 0 && dropCol < 8 && dropRow >= 0 && dropRow < 8 && 
                                            boardPos.x >= 0 && boardPos.y >= 0 && 
                                            boardPos.x < board.width && boardPos.y < board.height) {
                                            // Place piece on board
                                            var newBoard = JSON.parse(JSON.stringify(chessPosition.boardData))
                                            newBoard[dropRow][dropCol] = parent.modelData
                                            chessPosition.boardData = newBoard
                                        }
                                        
                                        // Hide drag piece
                                        globalDragPiece.visible = false
                                    }
                                    isDragging = false
                                }
                            }
                        }
                    }
                }
            }
        }
        
        // Control buttons
        Button {
            text: "Reset to Start"
            width: 100
            onClicked: {
                if (chessPosition) {
                    console.log("Reset clicked")
                    const startingBoard = [
                        ["bR", "bN", "bB", "bQ", "bK", "bB", "bN", "bR"],
                        ["bP", "bP", "bP", "bP", "bP", "bP", "bP", "bP"],
                        ["", "", "", "", "", "", "", ""],
                        ["", "", "", "", "", "", "", ""],
                        ["", "", "", "", "", "", "", ""],
                        ["", "", "", "", "", "", "", ""],
                        ["wP", "wP", "wP", "wP", "wP", "wP", "wP", "wP"],
                        ["wR", "wN", "wB", "wQ", "wK", "wB", "wN", "wR"]
                    ]
                    chessPosition.boardData = startingBoard
                }
            }
        }
        
        Button {
            text: "Clear Board"
            width: 100  
            onClicked: {
                if (chessPosition) {
                    console.log("Clear clicked")
                    const emptyBoard = [
                        ["", "", "", "", "", "", "", ""],
                        ["", "", "", "", "", "", "", ""],
                        ["", "", "", "", "", "", "", ""],
                        ["", "", "", "", "", "", "", ""],
                        ["", "", "", "", "", "", "", ""],
                        ["", "", "", "", "", "", "", ""],
                        ["", "", "", "", "", "", "", ""],
                        ["", "", "", "", "", "", "", ""]
                    ]
                    chessPosition.boardData = emptyBoard
                }
            }
        }
        
        // FEN input section
        Column {
            spacing: 5
            width: 100
            
            Text {
                text: "FEN Input"
                font.pixelSize: 10
                font.weight: Font.Bold
                color: palette.text
                anchors.horizontalCenter: parent.horizontalCenter
            }
            
            ScrollView {
                width: 100
                height: 60
                
                TextArea {
                    id: fenInput
                    width: parent.width
                    placeholderText: "Enter FEN position..."
                    wrapMode: TextArea.Wrap
                    selectByMouse: true
                    font.pixelSize: 9
                    
                    background: Rectangle {
                        color: palette.base
                        border.color: palette.mid
                        border.width: 1
                        radius: 2
                    }
                }
            }
            
            Button {
                text: "Upload Position"
                width: 100
                onClicked: {
                    if (chessPosition && fenInput.text.trim() !== "") {
                        console.log("Loading FEN:", fenInput.text.trim())
                        var boardFromFen = chessPosition.convertFenToBoardData(fenInput.text.trim())
                        if (boardFromFen.length === 8) {
                            chessPosition.boardData = boardFromFen
                        } else {
                            console.log("Invalid FEN format")
                        }
                    }
                }
            }
        }
    }
}