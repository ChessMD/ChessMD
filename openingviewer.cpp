#include "openingviewer.h"

#include <algorithm>   
#include <cstring>  
#include <QFile>
#include <QDataStream>
#include <QtGlobal>
#include <QHeaderView>

OpeningTree::OpeningTree()
    : mRoot(new BuildNode),
      mMappedBase(nullptr),
      mCurOffset(0)
{}

OpeningTree::~OpeningTree() {
    if (mMappedBase) mFile.unmap(reinterpret_cast<uchar*>(const_cast<char*>(mMappedBase)));
    deleteSubtree(mRoot);
}

void OpeningTree::insertGame(const QVector<quint16>& moves){
    BuildNode* node = mRoot;
    node->gamesReached++;
    for (quint16 m: moves){
        // find move in trie
        auto it = std::find_if(node->children.begin(), node->children.end(), [=](auto &pr){return pr.first ==m;});
        if(it == node->children.end()){
            //not alreaqy exist
            auto* child = new BuildNode;
            child->gamesReached = 1;
            node->children.append(qMakePair(m, child));
            node = child;
        }
        else{
            //increment gamesreached if already there
            it->second->gamesReached++;
            node = it->second;
        }

    }
}

//serialize
bool OpeningTree::serialize(const QString& path){
    assignOffsets();
    QFile f(path);

    if (!f.open(QIODevice::WriteOnly)) return false;
    QDataStream out(&f);
    out.setByteOrder(QDataStream::LittleEndian);

    // write in bfs order
    for(auto* node: mBfsOrder){
        quint64 off = mOffsets[node];
        f.seek(off);
        out << node->gamesReached;
        quint32 cc = node->children.size();
        out << cc;
        for(auto& pr: node->children){
            quint16 mv = pr.first;
            BuildNode* ch = pr.second;
            quint32 cnt = ch->gamesReached;
            quint64 chOff = mOffsets[ch];
            out << mv << cnt << chOff;
        }
    }
    return 1;

}

//load from serialized
bool OpeningTree::load(const QString& path) {
    mFile.setFileName(path);
    if (!mFile.open(QIODevice::ReadOnly)) return false;
    mMappedSize = quint64(mFile.size());
    uchar* ptr = mFile.map(0, mMappedSize);
    if (!ptr) return false;
    mMappedBase = reinterpret_cast<const char*>(ptr);

    return true;
}

//
// navigate
//

void OpeningTree::reset(){
    mCurOffset = 0;
}

bool OpeningTree::play(quint16 moveCode){
    NodeView nv = readNode(mCurOffset);
    for(int i = 0; i < nv.childCount; i++){
        if (nv.children[i].moveCode == moveCode){
            mCurOffset = nv.children[i].offset;
            return 1;
        }
    }

    //no games
    return 0;
}
 
quint32 OpeningTree::gamesReached() const {
    return readNode(mCurOffset).gamesReached;
}


QVector<Continuation> OpeningTree::continuations() const {
    NodeView nv = readNode(mCurOffset);
    QVector<Continuation> out;
    out.reserve(nv.childCount);
    for (int i = 0; i < nv.childCount; i++){
        out.append({nv.children[i].moveCode, nv.children[i].count});
    }
    return out;
}

void OpeningTree::deleteSubtree(BuildNode* n) {
    //delete node recursively
    for(auto& pr: n->children) deleteSubtree(pr.second);
    delete n;
}

void OpeningTree::assignOffsets(){
    mBfsOrder.clear();
    mOffsets.clear();
    quint64 nextOff = 0;
    mBfsOrder.append(mRoot);
    for(int i = 0; i < mBfsOrder.size(); i++){
        BuildNode* n = mBfsOrder[i];
        mOffsets[n] = nextOff;
        quint64 size = 4 + 4 + quint64(n->children.size()) * (2+4+8);
        nextOff += size;
        for(auto& pr: n -> children) mBfsOrder.append(pr.second);
    }
}

OpeningTree::NodeView OpeningTree::readNode(quint64 off) const {
    const char* p = static_cast<const char*>(mMappedBase) + off;
    NodeView nv;
    std::memcpy(&nv.gamesReached, p, 4);  p += 4;
    std::memcpy(&nv.childCount,   p, 4);  p += 4;
    nv.children = reinterpret_cast<const ChildEntry*>(p);

    return nv;
}


OpeningViewer::OpeningViewer(QWidget *parent)
    : QWidget{parent}
{   

    //load opening book
    mOpeningBookLoaded = mTree.load("./openings.bin");

    //
    //ui
    //

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(6, 6, 6, 6);
    mainLayout->setSpacing(4);

    mPositionLabel = new QLabel("Starting Position");
    mPositionLabel->setStyleSheet("font-weight: bold; font-size: 13px;");

    mStatsLabel = new QLabel("No position data");
    mStatsLabel->setStyleSheet("font-size: 12px;"); 

    mMovesList = new QTreeWidget();
    mMovesList->setHeaderLabels(QStringList() << "Move" << "Games" << "Win %" << "Score");
    mMovesList->setRootIsDecorated(false);
    mMovesList->setAlternatingRowColors(true);
    mMovesList->setSortingEnabled(true);
    mMovesList->sortByColumn(1, Qt::DescendingOrder);
    mMovesList->setMinimumHeight(150);

    QHeaderView* header = mMovesList->header();
    header->setSectionResizeMode(QHeaderView::ResizeToContents);
    header->setStretchLastSection(true);
    header->setMinimumSectionSize(70);
    header->resizeSection(0, 70);   
    header->resizeSection(1, 70);   
    header->resizeSection(2, 70);   
    header->resizeSection(3, 70);   
    
    //styles
    mMovesList->setStyleSheet(R"(
        QTreeWidget {
            border: 1px solid palette(mid);
            border-radius: 3px;
        }
        QTreeWidget::item {
            height: 22px; /* Consistent row height */
        }
        QTreeWidget::item:hover {
            background: palette(highlight);
            color: palette(highlighted-text);
            opacity: 0.5;
        }
        QTreeWidget::item:selected {
            background: palette(highlight);
            color: palette(highlighted-text);
        }
    )");
    
    mainLayout->addWidget(mPositionLabel);
    mainLayout->addWidget(mStatsLabel);
    mainLayout->addWidget(mMovesList);
    
    connect(mMovesList, &QTreeWidget::itemDoubleClicked, this, &OpeningViewer::onMoveSelected);
}

void OpeningViewer::updatePosition(const QVector<QString>& uciMoves)
{
    if (!mOpeningBookLoaded) {
        mPositionLabel->setText(tr("No opening database loaded"));
        mStatsLabel->setText(tr(""));
        mMovesList->clear();
        return;
    }

    mTree.reset();
    for (const QString& uci: uciMoves){
        quint16 code = encodeMove(uci);
        if(!mTree.play(code)){
            //no games
            mMovesList->clear();
            mStatsLabel->setText(tr("0 Games"));
            return;
        }
    }

    quint32 total = mTree.gamesReached();
    mStatsLabel->setText(tr("%1 Games").arg(total));

    mMovesList->clear();
    for(auto cont: mTree.continuations()){
        QString uci = decodeMove(cont.moveCode);
        addMoveToList(uci, cont.count, 0, "0");
    }
}


//helper
void OpeningViewer::addMoveToList(const QString& move, int games, double winPercentage, const QString& score)
{
    QTreeWidgetItem* item = new QTreeWidgetItem(mMovesList);
    
    item->setText(0, move);
    item->setText(1, QString::number(games));
    item->setText(2, QString::number(winPercentage, 'f', 1) + "%");
    item->setText(3, score);
    
    // store move
    item->setData(0, Qt::UserRole, move);
    

}

void OpeningViewer::onMoveSelected(QTreeWidgetItem* item, int column)
{
    if (!item) return;
    
    QString move = item->data(0, Qt::UserRole).toString();
    emit moveClicked(move);
}


// some stolen conversion methods
quint16 OpeningViewer::encodeMove(const QString& uci)
{
    if (uci.size() < 4) return 0;
    int fileFrom = uci[0].toLatin1() - 'a';
    int rankFrom = uci[1].digitValue() - 1;
    int fileTo   = uci[2].toLatin1() - 'a';
    int rankTo   = uci[3].digitValue() - 1;

    quint16 from = quint16(rankFrom * 8 + fileFrom);
    quint16 to   = quint16(rankTo   * 8 + fileTo);
    return (from << 6) | to;
}

QString OpeningViewer::decodeMove(quint16 code)
{
    quint16 from = code >> 6;
    quint16 to   = code &  0x3F;
    int fileFrom = from % 8;
    int rankFrom = from / 8;
    int fileTo   = to   % 8;
    int rankTo   = to   / 8;

    auto sq = [](int f, int r){
        return QString(QChar('a' + f)) + QString::number(r + 1);
    };
    return sq(fileFrom, rankFrom) + sq(fileTo, rankTo);
}
