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

class Project;
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
    
    void doProjectConnections(Project* project);
    void resetInterface();
    void showHelp(bool show);
    
    void readSettings();
    void writeSettings();
    
public slots:
    void updateProject();
    void applySettings();
    void adjustStep();
    void studyPeriodChanging();
    
protected:
    void paintEvent(QPaintEvent* e);
    void resizeEvent(QResizeEvent* e);
    void updateLayout();
    void mousePressEvent(QMouseEvent* e);
    void mouseReleaseEvent(QMouseEvent* e);
    void mouseMoveEvent(QMouseEvent* e);
    void keyPressEvent(QKeyEvent* event);
    
private slots:
    void showProperties();
    void showImport();
    void showPhases();
    void slideRightPanel();
    void prepareNextSlide();
    
    void updateEventsZoom(double prop);
    void exportEventsScene();
    
    void updatePhasesZoom(double prop);
    void exportPhasesScene();
    
    void updateCalibration(const QJsonObject& date);
    void showCalibration(bool show = true);
    void hideCalibration();
    
private:
    void exportSceneImage(QGraphicsScene* scene);
    
private:
    QWidget* mLeftWrapper;
    QWidget* mRightWrapper;
    
    // ------
    
    EventsScene* mEventsScene;
    QGraphicsView* mEventsView;
    
    SceneGlobalView* mEventsGlobalView;
    ScrollCompressor* mEventsGlobalZoom;
    
    Button* mButNewEvent;
    Button* mButNewEventKnown;
    Button* mButDeleteEvent;
    Button* mButRecycleEvent;
    Button* mButExportEvents;
    Button* mButEventsOverview;
    Button* mButEventsGrid;
    
    // ------
    
    ImportDataView* mImportDataView;
    EventPropertiesView* mEventPropertiesView;
    
    QWidget* mPhasesWrapper;
    PhasesScene* mPhasesScene;
    QGraphicsView* mPhasesView;
    
    SceneGlobalView* mPhasesGlobalView;
    ScrollCompressor* mPhasesGlobalZoom;
    
    Button* mButNewPhase;
    Button* mButDeletePhase;
    Button* mButExportPhases;
    Button* mButPhasesOverview;
    Button* mButPhasesGrid;
    
    QPropertyAnimation* mAnimationHide;
    QPropertyAnimation* mAnimationShow;
    
    QWidget* mCurrentRightWidget;
    
    // ------
    
    CalibrationView* mCalibrationView;
    Button* mButBackEvents;
    QPropertyAnimation* mAnimationCalib;
    
    // ------
    
    Label* mStudyLab;
    Label* mMinLab;
    Label* mMaxLab;
    //Label* mStepLab;
    
    LineEdit* mMinEdit;
    LineEdit* mMaxEdit;
    //LineEdit* mStepEdit;
    
    Button* mButApply;
    Button* mButStep;
    
    Button* mButProperties;
    Button* mButImport;
    Button* mButPhasesModel;
    
    QRect mLeftRect;
    QRect mLeftHiddenRect;
    QRect mRightRect;
    QRect mRightSubRect;
    QRect mRightSubHiddenRect;
    QRect mHandlerRect;
    
    int mMargin;
    int mToolbarH;
    double mSplitProp;
    int mHandlerW;
    bool mIsSplitting;
    bool mCalibVisible;
};

#endif
