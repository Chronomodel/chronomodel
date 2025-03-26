/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2025

Authors :
	Philippe LANOS
	Helori LANOS
 	Philippe DUFRESNE

This software is a computer program whose purpose is to
create chronological models of archeological data using Bayesian statistics.

This software is governed by the CeCILL V2.1 license under French law and
abiding by the rules of distribution of free software.  You can  use,
modify and/ or redistribute the software under the terms of the CeCILL
license as circulated by CEA, CNRS and INRIA at the following URL
"http://www.cecill.info".

As a counterpart to the access to the source code and  rights to copy,
modify and redistribute granted by the license, users are provided only
with a limited warranty  and the software's author,  the holder of the
economic rights,  and the successive licensors  have only  limited
liability.

In this respect, the user's attention is drawn to the risks associated
with loading,  using,  modifying and/or developing or reproducing the
software by the user in light of its specific status of free software,
that may mean  that it is complicated to manipulate,  and  that  also
therefore means  that it is reserved for developers  and  experienced
professionals having in-depth computer knowledge. Users are therefore
encouraged to load and test the software's suitability as regards their
requirements in conditions enabling the security of their systems and/or
data to be ensured and,  more generally, to use and operate it in the
same conditions as regards security.

The fact that you are presently reading this means that you have had
knowledge of the CeCILL V2.1 license and that you accept its terms.
--------------------------------------------------------------------- */

#ifndef GRAPHVIEWRESULTS_H
#define GRAPHVIEWRESULTS_H

#include "ModelUtilities.h"
#include "StudyPeriodSettings.h"
#include "MCMCSettings.h"

#include "GraphView.h"

#include <QWidget>
#include <QList>
#include <QColor>
#include <QBrush>
#include <QTextEdit>
#include <QPointer>
#include <QEvent>

class Button;

class MHVariable;
class MetropolisVariable;
class Model;

/**
 * @brief The ScrollableLabel class
 * This widget was created to replace the QEditText, which crashed when the "show Stat." box was folded and unfolded several times.
 * This component is simpler, but provides fewer options.
 * Compensates for the bug in Qt under Windows that causes a bar to appear in the middle.
 */
class ScrollableLabel : public QScrollArea {

private:
    QLabel* label;

public:
    ScrollableLabel(QWidget *parent = nullptr) :
        QScrollArea(parent)
    {
        // Création du widget principal
        QWidget *mainWidget = new QWidget(parent);
        QVBoxLayout *layout = new QVBoxLayout(parent);

        // QLabel creation
        label = new QLabel(parent);
        // Enable text selection
        label->setTextInteractionFlags(Qt::TextSelectableByMouse);
        label->setText("<h1>Titre</h1>"
                       "<p>This is an <b>example</b> of text in <i>HTML</i>.</p>"
                       "<ul>"
                       "<li>Element 1</li>"
                       "<li>Element 2</li>"
                       "<li>Element 3</li>"
                       "</ul>");
        label->setWordWrap(true); // Enables line feed

        // Add QLabel to layout
        layout->addWidget(label);
        // Creating a QFrame for the frame
       // QFrame *frame = new QFrame(parent);
        //frame->setFrameShape(QFrame::StyledPanel);
       // frame->setStyleSheet("QFrame { border: 1px solid gray; }");
       // frame->setLayout(layout);

        mainWidget->setLayout(layout);
        setWidget(mainWidget);
        setWidgetResizable(true); // Allows the widget to adjust to the size of the QScrollArea
    }

    void setText(const QString &HTMLtext) {
        label->setText(HTMLtext);
    }


};


class Overlay : public QWidget {
public:
    Overlay(QWidget * parent = nullptr) : QWidget{parent} {
       setAttribute(Qt::WA_TransparentForMouseEvents);
     //   setWindowFlags(Qt::Widget | Qt::FramelessWindowHint | Qt::ToolTip | Qt::WindowStaysOnTopHint);
       setAttribute(Qt::WA_NoSystemBackground, true);
       setAttribute(Qt::WA_TranslucentBackground, true);
    }

protected:

    void paintEvent(QPaintEvent *) override {
        const QColor color (49, 112, 176, 40);
        QFont font (QFont().family(), 30, QFont::Medium, false);

        QPainter p (this);
        p.setFont(font);
        p.fillRect(rect(), color);
        p.setPen(color.darker());
        p.drawText(rect(), Qt::AlignCenter | Qt::TextWordWrap, QObject::tr("Selected"));
        p.end();
    }
};

class OverLine : public QWidget {
public:
    OverLine(QWidget * parent = nullptr) : QWidget{parent} {
       setAttribute(Qt::WA_TransparentForMouseEvents);
     //   setWindowFlags(Qt::Widget | Qt::FramelessWindowHint | Qt::ToolTip | Qt::WindowStaysOnTopHint);
       setAttribute(Qt::WA_NoSystemBackground, true);
       setAttribute(Qt::WA_TranslucentBackground, true);
    }

protected:

    void paintEvent(QPaintEvent *) override {
        const QColor color (49, 112, 176, 40);
        QFont font (QFont().family(), 30, QFont::Medium, false);

        QPainter p (this);
        p.setFont(font);
        p.fillRect(rect(), color);
        p.setPen(color.darker());
        p.drawText(rect(), Qt::AlignCenter | Qt::TextWordWrap, QObject::tr("Selected"));
        p.end();
    }
};

class GraphViewResults: public QWidget
{
    Q_OBJECT
public:
    enum graph_t{
        ePostDistrib,
        eTrace,
        eAccept,
        eCorrel,
        eFunction
    };
    enum variable_t{
        eBeginEnd,
        eThetaEvent, eS02,
        eDataTi, eDataCalibrate, eDataWiggle,
        eCredibility,
        eSigma ,
        eVg,

        eTempo,
        eActivity,
        eActivityUnif,
        eError,
        eDuration,

        eG, eGError, eMap, eGEventsPts, eGDatesPts,
        eGP,
        eGS,
        eLambda,
        eS02Vg
    };


    // member
protected:
    int mHeightForVisibleAxis;
    Overlay* mOverLaySelect;

    GraphView* mGraph;
    graph_t mCurrentTypeGraph;
    QList<variable_t> mCurrentVariableList;

    QString mTitle;

    QColor mItemColor;

    bool mShowAllChains;
    QList<bool> mShowChainList;
    QList<variable_t> mShowVariableList;

    ScrollableLabel* mStatArea;
    QString mStatHTMLText;
    bool mShowNumResults;
    bool mIsSelected;
    bool mShowSelectedRect;

    StudyPeriodSettings mSettings;
    MCMCSettings mMCMCSettings;
    std::vector<ChainSpecs> mChains;

    QColor mMainColor;

    qreal mMargin;
    qreal mLineH;
    int mTopShift;

    QFont mGraphFont;
    //-----

public:

    explicit GraphViewResults(QWidget *parent = nullptr);
    virtual ~GraphViewResults();

    virtual void mousePressEvent(QMouseEvent *);

    void setSettings(const StudyPeriodSettings &settings);
    void setMCMCSettings(const MCMCSettings &mcmc, const std::vector<ChainSpecs> &chains);

    void setMainColor(const QColor &color);

    void setTitle(const QString &title);
    inline void setTipYLab (const QString &label) {mGraph->setTipYLab(label);};
    inline QString title() const {return mTitle;};

    void setMarginLeft (qreal &m);
    void setMarginRight (qreal &m);

    virtual void setGraphsFont(const QFont &font);
    void setGraphsThickness(int value);
    void setGraphsOpacity(int value);

    void setItemColor(const QColor &itemColor);

    void setHeightForVisibleAxis( const int height)
    {
        mHeightForVisibleAxis = height;
    }

    inline bool isSelected() const  {return mIsSelected;}

    inline void setSelected( const bool selected) {
        mIsSelected = selected;
    }

    inline void showSelectedRect(const bool show) {
        mShowSelectedRect = show;
    }

    void setShowNumericalResults(const bool show);

    QFont getGraphFont() const {return mGraphFont;};
    GraphView *getGraph() const;  //{return mGraph; }
    inline QList<variable_t> getCurrentVariables() const {return mCurrentVariableList;}
    inline graph_t getCurrentType() const { return mCurrentTypeGraph; }

    QString getTextAreaToHtml() const { return QString();};//mStatArea->toHtml();}
    // GraphView::Rendering getRendering() const  { return mGraph->getRendering(); }
    //QString getResultsText() const {return mStatHTMLText;}; // useless
    QString getTextAreaToPlainText() const { return html_to_plain_text(mStatHTMLText);};


    void generateTraceCurves(const std::vector<ChainSpecs> &chains, MetropolisVariable* variable, const QString& name = QString());

    void generateAcceptCurves(const std::vector<ChainSpecs> &chains, MHVariable* variable);

    void generateCorrelCurves(const std::vector<ChainSpecs> &chains, MHVariable* variable);

    void graph_reset();
    void graph_density();
    void graph_trace();
    void graph_acceptation();
    void graph_correlation();

    // This method is used to recreate all curves in mGraph.
    // It is vitual because we want a different behavior in sub-classes (GraphViewDate, GraphViewEvent and GraphViewPhase)
    virtual void generateCurves(const graph_t typeGraph, const QList<variable_t> &variableList);

    // This method is used to update visible existing curves in mGraph.
    // It is vitual because we want a different behavior in suclasses (GraphViewDate, GraphViewEvent and GraphViewPhase)
    //virtual void updateCurvesToShow(bool showAllChains, const QList<bool>& showChainList, bool showCredibility, bool showCalib, bool showWiggle);
    virtual void updateCurvesToShow(bool showAllChains, const QList<bool> &showChainList, const QList<variable_t> &showVariableList);


    inline void changeYScaleDivision(const Scale &sc) {mGraph->setYScaleDivision(sc);};
    inline void changeYScaleDivision(const double major, const int minor) {mGraph->setYScaleDivision(major, minor);};

    // This is not from QWidget : we create this function to update the layout from different places (eg: in resizeEvent()).
    // It is vitual because we want a different behavior in subclasses (GraphViewDate, GraphViewEvent and GraphViewPhase)
    void updateLayout();
    void showNumericalResults(const bool show);
    inline void setNumericalResults(const QString &resultsHTML){mStatHTMLText = resultsHTML; mStatArea->setText(mStatHTMLText);};

    void setView(type_data range_Xmin, type_data range_Xmax,  type_data resultCurrentMinT, type_data resultCurrentMaxT, const double scale_major, const int scale_minor);

public slots:
    void setRange(type_data min, type_data max);
    void setCurrentX(type_data min, type_data max);

    void zoom(type_data min, type_data max);

    void saveAsImage();
    void imageToClipboard();
    void resultsToClipboard();
    void saveGraphData() const; // must be accessible by ResultsView
    void changeXScaleDivision(const Scale &sc) {mGraph->changeXScaleDivision(sc);};
    void changeXScaleDivision(const double &major, const int &minor) {mGraph->changeXScaleDivision(major, minor);};

protected:

    // These methods are from QWidget and we want to modify their behavior
    void paintEvent(QPaintEvent* e);
    void resizeEvent(QResizeEvent*);

signals:
    void unfoldToggled(bool toggled);
    void visibilityChanged(bool visible);
    void selected();

};




class Filter : public QObject {
    QPointer<Overlay> m_overlay;
    QPointer<QWidget> m_overlayOn;
public:
    Filter(QObject * parent = nullptr) : QObject{parent} {}

protected:
    bool eventFilter(QObject * obj, QEvent * ev) override {
        //if (!obj->isWidgetType()) return false;
        auto w = static_cast<QWidget*>(obj);
        if (ev->type() == QEvent::MouseButtonPress) {
            if (!m_overlay) m_overlay = new Overlay(w->parentWidget());
            m_overlay->setGeometry(w->geometry());
            m_overlayOn = w;
            m_overlay->show();
        }
        else if (ev->type() == QEvent::Resize) {
            if (m_overlay && m_overlayOn == w)
                m_overlay->setGeometry(w->geometry());
        }
        return false;
    }
};


#endif
