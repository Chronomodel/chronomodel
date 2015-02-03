#ifndef Collapsible_H
#define Collapsible_H

#include <QWidget>

class QPropertyAnimation;
class Collapsible;


class CollapsibleHeader: public QWidget
{
    Q_OBJECT
public:
    CollapsibleHeader(Collapsible* collapsible, Qt::WindowFlags flags = 0);
    ~CollapsibleHeader();
    
    void setTitle(const QString& title);
    
protected:
    void paintEvent(QPaintEvent* e);
    void mousePressEvent(QMouseEvent* e);
    
private:
    Collapsible* mCollapsible;
    QString mTitle;
};


class Collapsible: public QWidget
{
    Q_OBJECT
public:
    Collapsible(const QString& title, QWidget* parent = 0, Qt::WindowFlags flags = 0);
    ~Collapsible();

    void setWidget(QWidget* widget, int height);
    void setTitle(const QString& title);
    bool opened() const;
    
public slots:
    void open(bool open);
    
protected:
    void paintEvent(QPaintEvent* e);
    void resizeEvent(QResizeEvent* e);
    void updateLayout();
    
private slots:
    void sendCollapsing(const QVariant& variant);
    
signals:
    void collapsing(int h);
    
private:
    CollapsibleHeader* mHeader;
    QWidget* mWidget;
    int mHeaderHeight;
    int mWidgetHeight;
    bool mOpened;
    
    QPropertyAnimation* mAnimation;
};

#endif
