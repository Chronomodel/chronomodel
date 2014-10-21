#ifndef GraphViewResults_H
#define GraphViewResults_H

#include "ProjectSettings.h"
#include "MCMCSettings.h"

#include <QWidget>
#include <QList>
#include <QColor>

class GraphView;
class Event;


class GraphViewResults: public QWidget
{
    Q_OBJECT
public:
    explicit GraphViewResults(QWidget *parent = 0);
    virtual ~GraphViewResults();
    
    void setSettings(const ProjectSettings& settings);
    void setMCMCSettings(const MCMCSettings& mcmc);
    
    void setParentGraph(GraphViewResults* graph);
    bool visible() const;
    void toggleUnfold(bool toggle);
    void showUnfold(bool show, const QString& text);
    void setMainColor(const QColor& color);
    
public slots:
    void setVisibility(bool visible);
    void setRange(float min, float max);
    void zoom(float min, float max);
    
protected:
    virtual void paintEvent(QPaintEvent* e);
    void resizeEvent(QResizeEvent* e);
    void mousePressEvent(QMouseEvent* e);
    void mouseReleaseEvent(QMouseEvent* e);
    
signals:
    void unfoldToggled(bool toggled);
    void visibilityChanged(bool visible);
    
protected:
    GraphView* mGraph;
    GraphViewResults* mParentGraph;
    
    ProjectSettings mSettings;
    MCMCSettings mMCMCSettings;
    
    QColor mMainColor;
    
    QRectF mUnfoldRect;
    QRectF mDataRect;
    QRectF mImageRect;
    
    bool mVisible; // flag to indicate if the graph is hidden (eg :data in closed event)
    
    bool mUnfoldToggled; // unfold button state : to indicate if subgraphs must be shown
    bool mShowUnfold;
    QString mUnfoldText;
    
    bool mIsDataDown;
    bool mIsImageDown;
    
    int mMargin;
    int mLineH;
    int mGraphLeft;
    
    QList<QColor> mChainColors;
};

#endif
