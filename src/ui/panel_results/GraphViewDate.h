#ifndef GraphViewDate_H
#define GraphViewDate_H

#include "GraphViewResults.h"

class Date;


class GraphViewDate: public GraphViewResults
{
    Q_OBJECT
public:
    explicit GraphViewDate(QWidget *parent = 0);
    virtual ~GraphViewDate();
    
    void setDate(Date* date);
    void setColor(const QColor& color);
    void showCalib(bool show);
    
private slots:
    void saveGraphData();
    
protected:
    void paintEvent(QPaintEvent* e);
    void refresh();
    
private:
    Date* mDate;
    bool mShowCalib;
    QColor mColor;
};


#endif
