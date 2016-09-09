#ifndef PluginGaussRefView_H
#define PluginGaussRefView_H

#if USE_PLUGIN_GAUSS

#include "../GraphViewRefAbstract.h"

class PluginGauss;
class GraphView;
class QVBoxLayout;


class PluginGaussRefView: public GraphViewRefAbstract
{
    Q_OBJECT
public:
    explicit PluginGaussRefView(QWidget* parent = 0);
    virtual ~PluginGaussRefView();
    
    void setDate(const Date& d, const ProjectSettings& settings);
    
public slots:
    void zoomX(float min, float max);
    void setMarginRight(const int margin);
protected:
    void resizeEvent(QResizeEvent* e);
    
private:
    GraphView* mGraph;
};

#endif
#endif

