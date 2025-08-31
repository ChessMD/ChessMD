#ifndef DRAGGABLECHECKBOX_H
#define DRAGGABLECHECKBOX_H

#include <QWidget>
#include <QCheckBox>
#include <QPoint>
#include <QVBoxLayout>
#include <QPixmap>
#include <QPushButton>
#include <QLabel>


class DraggableCheckBox : public QWidget
{
    Q_OBJECT
public:
    explicit DraggableCheckBox(const QString& text, QWidget *parent = nullptr);

    QString text() const;
    void setText(const QString& text);
    bool isChecked() const;
    void setChecked(bool checked);
    void setDeleteEnabled(bool enabled);
    void setDragEnabled(bool enabled); 
    void setCheckBoxEnabled(bool enabled);

signals:
    void deleteRequested();

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;  
    void enterEvent(QEnterEvent* event) override;  
    void leaveEvent(QEvent* event) override;     

private:
    QPixmap createDragIcon();
    QPixmap createDisabledDragIcon(); 
    QPoint mDragStart;
    QCheckBox* mCheckBox;
    QPushButton* mDeleteButton;
    QLabel* mDragHandle; 
    bool mDragEnabled;   
};


class DraggableCheckBoxContainer : public QWidget {
    Q_OBJECT
public:
    DraggableCheckBoxContainer(QWidget* parent);

    void addCheckBox(DraggableCheckBox* checkBox);
    void removeCheckBox(DraggableCheckBox* checkBox);

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
