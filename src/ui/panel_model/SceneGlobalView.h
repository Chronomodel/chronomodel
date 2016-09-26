#ifndef SceneGlobalView_H
#define SceneGlobalView_H

#include <QWidget>

class QGraphicsScene;
class QGraphicsView;


class SceneGlobalView: public QWidget
{
    Q_OBJECT
public:
    SceneGlobalView(QGraphicsScene* scene, QGraphicsView* view, QWidget* parent = 0, Qt::WindowFlags flags = 0);
    ~SceneGlobalView();
    
protected:
    void paintEvent(QPaintEvent* e);
    void mousePressEvent(QMouseEvent* e);
    void mouseReleaseEvent(QMouseEvent* e);
    void mouseMoveEvent(QMouseEvent* e);
    QRectF getTargetRect();
    void setPosition(const QPoint& pos);
    
private:
    QGraphicsScene* mScene;
    QGraphicsView* mView;
    bool mIsDragging;
};

#endif
