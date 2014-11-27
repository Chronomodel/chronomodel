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
    void updateDatesSelection();
    
    void sendMergeSelectedDates();
    void sendSplitDate();
    
    void updateKnownType();
    void updateKnownFixed();
    void updateKnownUnifStart();
    void updateKnownUnifEnd();
    void updateKnownGaussMeasure();
    void updateKnownGaussError();
    void loadKnownCsv();
    
    void updateKnownGraph();
    void updateKnownControls();
    
signals:
    void mergeSelectedDates(Event* event);
    void splitDate(Date* date);
    void calibRequested(const QJsonObject& date);
    
private:
    QJsonObject mEvent;
    
    QWidget* mDefaultView;
    QWidget* mKnownView;
    
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
    
    Button* mMergeBut;
    Button* mSplitBut;
    
    RadioButton* mKnownFixedRadio;
    RadioButton* mKnownUniformRadio;
    RadioButton* mKnownGaussRadio;
    
    Label* mKnownFixedLab;
    Label* mKnownStartLab;
    Label* mKnownEndLab;
    Label* mKnownGaussMeasureLab;
    Label* mKnownGaussErrorLab;
    
    LineEdit* mKnownFixedEdit;
    LineEdit* mKnownStartEdit;
    LineEdit* mKnownEndEdit;
    LineEdit* mKnownGaussMeasure;
    LineEdit* mKnownGaussError;
    
    GraphView* mKnownGraph;
};

#endif
