#ifndef EventPropertiesView_H
#define EventPropertiesView_H

#include <QWidget>
#include <QJsonObject>

class Event;
class Date;
class Label;
class LineEdit;
class QComboBox;
class ColorPicker;
class DatesList;
class Button;
class RadioButton;
class GraphView;


class EventPropertiesView: public QWidget
{
    Q_OBJECT
public:
    EventPropertiesView(QWidget* parent = 0, Qt::WindowFlags flags = 0);
    ~EventPropertiesView();
    
    void updateEvent();
    const QJsonObject& getEvent() const;
    int mToolbarH;
    
public slots:
    void setEvent(const QJsonObject& event);
    
protected:
    void paintEvent(QPaintEvent* e);
    void resizeEvent(QResizeEvent* e);
    void updateLayout();
    
private slots:
    
    void updateEventName(const QString& name);
    void updateEventColor(QColor color);
    void updateEventMethod(int index);
    
    void createDate();
    void deleteSelectedDates();
    void recycleDates();
    
    void updateCombineAvailability();
    void sendMergeSelectedDates();
    void sendSplitDate();
    
    void showDatesOptions();
    
    void updateKnownType();
    void updateKnownFixed(const QString& text);
    void updateKnownUnifStart();
    void updateKnownUnifEnd();
    void loadKnownCsv();
    
    void updateKnownGraph();
    void updateKnownControls();
    
public slots:
    void hideCalibration();
    
signals:
    void mergeDatesRequested(const int eventId, const QList<int>& dateIds);
    void splitDateRequested(const int eventId, const int dateId);
    
    void updateCalibRequested(const QJsonObject& date);
    void showCalibRequested(bool show);
    
private:
    int minimumHeight;
    QJsonObject mEvent;
    
    QWidget* mEventView;
    QWidget* mBoundView;
    
    Label* mNameLab;
    Label* mColorLab;
    Label* mMethodLab;
    
    LineEdit* mNameEdit;
    ColorPicker* mColorPicker;
    QComboBox* mMethodCombo;
    
    DatesList* mDatesList;
    QList<Button*> mPluginButs1;
    QList<Button*> mPluginButs2;
    Button* mDeleteBut;
    Button* mRecycleBut;
    
    Button* mCalibBut;
    Button* mOptsBut;
    Button* mMergeBut;
    Button* mSplitBut;
    
    RadioButton* mKnownFixedRadio;
    RadioButton* mKnownUniformRadio;
    
    Label* mKnownFixedLab;
    Label* mKnownStartLab;
    Label* mKnownEndLab;
    
    LineEdit* mKnownFixedEdit;
    LineEdit* mKnownStartEdit;
    LineEdit* mKnownEndEdit;
    
    GraphView* mKnownGraph;
};

#endif
