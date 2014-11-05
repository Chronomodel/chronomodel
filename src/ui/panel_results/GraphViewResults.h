#ifndef GraphViewResults_H
#define GraphViewResults_H

#include "ProjectSettings.h"
#include "MCMCSettings.h"

#include <QWidget>
#include <QList>
#include <QColor>

class GraphView;
class Button;
class QPropertyAnimation;


class GraphViewResults: public QWidget
{
    Q_OBJECT
public:
    explicit GraphViewResults(QWidget *parent = 0);
    virtual ~GraphViewResults();
    
    void setSettings(const ProjectSettings& settings);
    void setMCMCSettings(const MCMCSettings& mcmc);

    void setMainColor(const QColor& color);
    void toggle(const QRect& geometry);
    
public slots:
    void setRange(float min, float max);
    void zoom(float min, float max);
    
private slots:
    void exportAsImage();
    void exportData();
    
protected:
    virtual void paintEvent(QPaintEvent* e);
    void resizeEvent(QResizeEvent* e);
    
signals:
    void unfoldToggled(bool toggled);
    void visibilityChanged(bool visible);
    
protected:
    GraphView* mGraph;
    GraphViewResults* mParentGraph;
    
    ProjectSettings mSettings;
    MCMCSettings mMCMCSettings;
    
    QColor mMainColor;
    
    Button* mImageBut;
    Button* mDataBut;
    
    int mMargin;
    int mLineH;
    int mGraphLeft;
    
    QPropertyAnimation* mAnimation;
};

#endif
