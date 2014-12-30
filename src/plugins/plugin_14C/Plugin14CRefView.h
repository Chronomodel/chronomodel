#ifndef Plugin14CRefView_H
#define Plugin14CRefView_H

#if USE_PLUGIN_14C

#include "../GraphViewRefAbstract.h"

class GraphView;
class QVBoxLayout;


class Plugin14CRefView: public GraphViewRefAbstract
{
    Q_OBJECT
public:
    explicit Plugin14CRefView(QWidget* parent = 0);
    virtual ~Plugin14CRefView();
    
    void setDate(const Date& d, const ProjectSettings& settings);
    
public slots:
    void zoomX(double min, double max);
    
protected:
    void resizeEvent(QResizeEvent* e);
    
private:
    GraphView* mGraph;
};

#endif
#endif

