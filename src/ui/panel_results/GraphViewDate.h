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
    
    void setDate(Date *date);
    void setColor(const QColor& color);
    QColor getEventColor();
    
    void generateCurves(TypeGraph typeGraph, Variable variable);
    void updateCurvesToShow(bool showAllChains, const QList<bool>& showChainList, bool showCredibility, bool showCalib, bool showWiggle);
    
protected:
    void paintEvent(QPaintEvent* e);
    
private:
    Date* mDate;
    QColor mColor;
};


#endif
