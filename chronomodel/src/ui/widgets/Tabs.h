#ifndef TABS_H
#define TABS_H

#include <QWidget>


class Tabs: public QWidget
{
    Q_OBJECT
public:
    Tabs(QWidget* parent = 0);
    ~Tabs();
    
    void addTab(const QString& name);
    int currentIndex() const;
    void setTab(int index);
    
signals:
    void tabClicked(int index);
    
protected:
    void paintEvent(QPaintEvent* e);
    void resizeEvent(QResizeEvent* e);
    void mousePressEvent(QMouseEvent* e);
    void updateLayout();
    
private:
    QStringList mTabNames;
    QList<QRectF> mTabRects;
    QFont mFont;
    int mCurrentIndex;
};

#endif
