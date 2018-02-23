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
class ProjectSettings;
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
class MultiCalibrationView;
class Label;
class LineEdit;


class ModelView: public QWidget
{
    Q_OBJECT
public:
    ModelView(QWidget* parent = nullptr, Qt::WindowFlags flags = 0);
    ~ModelView();
    
    void setProject(Project* project);
    Project* getProject() const;
    void calibrateAll(ProjectSettings newS);
    bool findCalibrateMissing();

    void resetInterface();
    void showHelp(bool show);
    
    void readSettings();
    void writeSettings();
    void createProject();

    
public slots:
    void updateProject();
    void modifyPeriod();

    void updateMultiCalibration();

    void eventsAreSelected(); //connect with EventAreSelected
    //void phasesAreSelected();
    void noEventSelected();
    void applyAppSettings();
    
protected:
    void paintEvent(QPaintEvent* e);
    void resizeEvent(QResizeEvent* e);
    void updateLayout();
    void mousePressEvent(QMouseEvent* e);
    void mouseReleaseEvent(QMouseEvent* e);
    void mouseMoveEvent(QMouseEvent* e);
    void keyPressEvent(QKeyEvent* event);

    void adaptStudyPeriodButton(const double& min, const double& max);
    void connectScenes();
    void disconnectScenes();
    
private slots:
    void togglePropeties();
    void showProperties();
    void hideProperties();
    void showImport();
    void showPhases();
    void slideRightPanel();
    void prepareNextSlide();
    
    void updateEventsZoom(const double prop);
    void exportEventsScene();
    
    void updatePhasesZoom(const double prop);
    void exportPhasesScene();
    
    void updateCalibration(const QJsonObject& date);
    void showCalibration(bool show);
    
    void showMultiCalib();

    void setSettingsValid(bool valid);
    
    void searchEvent();
    
private:
    void exportSceneImage(QGraphicsScene* scene);
    
private:
    QWidget* mTopWrapper;
    QWidget* mLeftWrapper;
    QWidget* mRightWrapper;
    
    // ------
    
    EventsScene* mEventsScene;
    QGraphicsView* mEventsView;
    
    SceneGlobalView* mEventsGlobalView;
    ScrollCompressor* mEventsGlobalZoom;
    
    LineEdit* mEventsSearchEdit;
    QString mLastSearch;
    QVector<int> mSearchIds;
    int mCurSearchIdx;
    
    Button* mButNewEvent;
    Button* mButNewEventKnown;
    Button* mButDeleteEvent;
    Button* mButRecycleEvent;
    Button* mButExportEvents;
    Button* mButEventsOverview;
    Button* mButEventsGrid;
    Button* mButProperties;
    Button* mButMultiCalib;
    Button* mButImport;
    
    // ------
    
    ImportDataView* mImportDataView;
    EventPropertiesView* mEventPropertiesView;
    
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
    MultiCalibrationView* mMultiCalibrationView;

    QPropertyAnimation* mAnimationCalib;
    
    // ------
    double mTmin;
    double mTmax;

    Button* mButModifyPeriod;
    
    Label* mLeftPanelTitle;
    Label* mRightPanelTitle;
    
    QRect mTopRect;
    QRect mHandlerRect;

    QRect mLeftRect;
    QRect mLeftHiddenRect;

    QRect mRightRect;
    QRect mRightHiddenRect;


    
private:
    Project* mProject;
    
    int mMargin;
    int mToolbarH;
    int mButtonWidth;
    double mSplitProp;
    int mHandlerW;
    bool mIsSplitting;
    bool mCalibVisible;
};

#endif
