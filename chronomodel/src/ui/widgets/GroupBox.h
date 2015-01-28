#ifndef GroupBox_H
#define GroupBox_H

#include <QWidget>


class GroupBox: public QWidget
{
    Q_OBJECT
public:
    GroupBox(const QString& title, QWidget* parent = 0, Qt::WindowFlags flags = 0);
    virtual ~GroupBox();
    
protected:
    virtual void paintEvent(QPaintEvent* e);
    
protected:
    int mMargin;
    int mLineH;
    QString mTitle;
    
public:
    static int sTitleHeight;
};

#endif
