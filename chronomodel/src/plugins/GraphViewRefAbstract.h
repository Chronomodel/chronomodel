#ifndef GraphViewRefAbstract_H
#define GraphViewRefAbstract_H

#include <QWidget>
#include "ProjectSettings.h"

class GraphView;
class Date;


class GraphViewRefAbstract: public QWidget
{
    Q_OBJECT
public:
    explicit GraphViewRefAbstract(QWidget* parent = 0):QWidget(parent),mMeasureColor(56, 120, 50)
    {
        setMouseTracking(true);
    }
    virtual ~GraphViewRefAbstract(){}

    virtual void setDate(const Date& date, const ProjectSettings& settings)
    {
        Q_UNUSED(date);
        mSettings = settings;
    }
    
public slots:
    virtual void zoomX(double min, double max)
    {
        Q_UNUSED(min);
        Q_UNUSED(max);
    }
    
protected:
    ProjectSettings mSettings;
    QColor mMeasureColor;
};

#endif
