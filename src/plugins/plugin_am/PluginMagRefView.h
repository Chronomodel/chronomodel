#ifndef PluginMagRefView_H
#define PluginMagRefView_H

#if USE_PLUGIN_AM

#include "../GraphViewRefAbstract.h"

class PluginMag;
class GraphView;
class QLocale;


class PluginMagRefView: public GraphViewRefAbstract
{
    Q_OBJECT
public:
    explicit PluginMagRefView(QWidget* parent = 0);
    virtual ~PluginMagRefView();
    
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
