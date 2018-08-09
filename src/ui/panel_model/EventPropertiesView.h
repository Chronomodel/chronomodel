#ifndef EventPropertiesView_H
#define EventPropertiesView_H

#include <QWidget>
#include <QJsonObject>

class Event;
class Date;
class Label;
class LineEdit;

class QLabel;
class QLineEdit;
class QComboBox;
class QGroupBox;
class QRadioButton;

class ColorPicker;
class DatesList;
class Button;
class RadioButton;
class GraphView;


class EventPropertiesView: public QWidget
{
    Q_OBJECT
public:
    EventPropertiesView(QWidget* parent = nullptr, Qt::WindowFlags flags = Qt::Widget);
    ~EventPropertiesView();
    
    void updateEvent();
    const QJsonObject& getEvent() const;
    
    void setCalibChecked(bool checked);
    bool isCalibChecked() const;
    bool hasEvent() const;
    bool hasBound() const;
    bool hasEventWithDates() const;
    
public slots:
    void setEvent(const QJsonObject& event);
    void applyAppSettings();
    
protected:
    void paintEvent(QPaintEvent* e);
    void resizeEvent(QResizeEvent* e);
    void keyPressEvent(QKeyEvent* e);
    void updateLayout();
    
private slots:
    
    void updateEventName();
    void updateEventColor(const QColor& color);
    void updateEventMethod(int index);
    void updateIndex(int index);

    void createDate();
    void deleteSelectedDates();
    void recycleDates();
    
    void updateCombineAvailability();
    void sendCombineSelectedDates();
    void sendSplitDate();

    void updateButton();
    
//    void updateKnownType();
    void updateKnownFixed(const QString& text);
//    void updateKnownUnifStart(const QString& text);
//    void updateKnownUnifEnd(const QString& text);
    
    void updateKnownGraph();
//    void updateKnownControls();
    
signals:
    void combineDatesRequested(const int eventId, const QList<int>& dateIds);
    void splitDateRequested(const int eventId, const int dateId);
    
    void updateCalibRequested(const QJsonObject& date);
    void showCalibRequested(bool show);
    
private:
    int minimumHeight;
    QJsonObject mEvent;
    int mCurrentDateIdx;
    
    QWidget* mTopView;
    QWidget* mEventView;
    QWidget* mBoundView;
    
    QLabel* mNameLab;
    QLabel* mColorLab;
    QLabel* mMethodLab;
    
    QLineEdit* mNameEdit;
    ColorPicker* mColorPicker;
    QComboBox* mMethodCombo;
    
    DatesList* mDatesList;
    QList<Button*> mPluginButs1;
    QList<Button*> mPluginButs2;
    Button* mDeleteBut;
    Button* mRecycleBut;
    
    Button* mCalibBut;
    Button* mCombineBut;
    Button* mSplitBut;
    
   // QRadioButton* mKnownFixedRadio;
   // QRadioButton* mKnownUniformRadio;
    
    QLineEdit* mKnownFixedEdit;
   // QLineEdit* mKnownStartEdit;
   // QLineEdit* mKnownEndEdit;
    
    GraphView* mKnownGraph;
    
    QGroupBox* mFixedGroup;
//    QGroupBox* mUniformGroup;
    
    int mToolbarH;
    int mButtonWidth;
    int mButtonHeight;

    int mLineEditHeight;

     int mComboBoxHeight;




};

#endif
