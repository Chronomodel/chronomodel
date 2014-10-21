#ifndef ModelView_H
#define ModelView_H

#include <QWidget>

class QStackedWidget;
class QSplitter;
class QGraphicsItem;
class QGraphicsView;
class QPushButton;
class LineEdit;
class QPropertyAnimation;
class QGraphicsScene;

class ModelToolsView;
class EventsScene;
class PhasesScene;
class Event;
class PhasesList;
class ImportDataView;
class EventPropertiesView;
class Button;
class SceneGlobalView;
class ScrollCompressor;
class CalibrationView;
class Label;
class LineEdit;


class ModelView: public QWidget
{
    Q_OBJECT
public:
    ModelView(QWidget* parent = 0, Qt::WindowFlags flags = 0);
    ~ModelView();
    
public slots:
    void updateProject();
    void applySettings();
    
protected:
    void paintEvent(QPaintEvent* e);
    void resizeEvent(QResizeEvent* e);
    void updateLayout();
    void mousePressEvent(QMouseEvent* e);
    void mouseReleaseEvent(QMouseEvent* e);
    void mouseMoveEvent(QMouseEvent* e);
    void keyPressEvent(QKeyEvent* event);
    
private slots:
    void slideRightPanel();
    void prepareNextSlide();
    
    void updateEventsZoom(float prop);
    void exportEventsScenePNG();
    void exportEventsSceneSVG();
    
    void updatePhasesZoom(float prop);
    void exportPhasesScenePNG();
    void exportPhasesSceneSVG();
    
    void showCalibration(const QJsonObject& date);
    void showEvents(bool show);
    
private:
    void exportSceneImage(QGraphicsScene* scene, bool asSvg);
    
private:
    QWidget* mLeftWrapper;
    QWidget* mRightWrapper;
    
    QWidget* mEventsWrapper;
    EventsScene* mEventsScene;
    QGraphicsView* mEventsView;
    
    SceneGlobalView* mEventsGlobalView;
    ScrollCompressor* mEventsGlobalZoom;
    Button* mEventsButExportPNG;
    Button* mEventsButExportSVG;
    
    QWidget* mPhasesWrapper;
    PhasesScene* mPhasesScene;
    QGraphicsView* mPhasesView;
    
    SceneGlobalView* mPhasesGlobalView;
    ScrollCompressor* mPhasesGlobalZoom;
    Button* mPhasesButExportPNG;
    Button* mPhasesButExportSVG;
    
    PhasesList* mPhasesList;
    ImportDataView* mImportDataView;
    EventPropertiesView* mEventPropertiesView;
    
    CalibrationView* mCalibrationView;
    QPropertyAnimation* mAnimationCalib;
    
    QPropertyAnimation* mAnimationHide;
    QPropertyAnimation* mAnimationShow;
    
    Label* mMinLab;
    Label* mMaxLab;
    Label* mStepLab;
    LineEdit* mMinEdit;
    LineEdit* mMaxEdit;
    LineEdit* mStepEdit;
    Button* mButApply;
    
    Button* mButEvents;
    
    Button* mButNewEvent;
    Button* mButNewEventKnown;
    Button* mButDeleteEvent;
    Button* mButRecycleEvent;
    
    Button* mButNewPhase;
    Button* mButDeletePhase;
    
    Button* mButImport;
    Button* mButPhasesList;
    Button* mButPhasesModel;
    Button* mButProperties;
    
    QRect mToolBarRect;
    QRect mLeftRect;
    QRect mLeftRectHidden;
    
    int mMargin;
    int mToolbarH;
    int mRightW;
    int mHandlerW;
    bool mIsSplitting;
};

#endif
