#ifndef DRAGGABLECHECKBOX_H
#define DRAGGABLECHECKBOX_H

#include <QWidget>
#include <QCheckBox>
#include <QPoint>
#include <QVBoxLayout>

class DraggableCheckBox : public QCheckBox
{
    Q_OBJECT
public:
    explicit DraggableCheckBox(const QString& text, QWidget *parent = nullptr);

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;

private:
    QPoint mDragStart;
};


class DraggableCheckBoxContainer : public QWidget {
    Q_OBJECT
public:
    DraggableCheckBoxContainer(QWidget* parent);

    void addCheckBox(DraggableCheckBox* checkBox);

    QVector<DraggableCheckBox*> getCheckBoxes() const;

    void clear();

protected:
    void dragEnterEvent(QDragEnterEvent* event) override;

    void dropEvent(QDropEvent* event) override;

private:
    QVBoxLayout* mLayout;
    QVector<DraggableCheckBox*> mBoxes;
};

#endif // DRAGGABLECHECKBOX_H
