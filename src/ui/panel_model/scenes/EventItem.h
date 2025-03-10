/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2024

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

#ifndef EVENTITEM_H
#define EVENTITEM_H

#include "AbstractItem.h"
#include "CurveSettings.h"

class EventsScene;

class EventItem : public AbstractItem
{
    Q_OBJECT
protected:

    QJsonObject mStudyPeriodSettings;
    bool mWithSelectedPhase;
    bool mThumbVisible;

    bool mIsNode;

    const qreal mNodeSkin;

    qreal mPhasesHeight;
    qreal mCurveLineHeight;
    qreal mCurveTextHeight;

public:
    EventItem(QGraphicsItem* parent = nullptr);

    explicit EventItem(EventsScene* scene, const QJsonObject &eventObj, const QJsonObject &StudyPeriodSettings, QGraphicsItem* parent = nullptr);
    virtual ~EventItem();

    enum { Type = UserType + 11 };

    int type() const override
    {
        // Enable the use of qgraphicsitem_cast with this item.
        return Type;
    };

    void remove_dateItems();
    virtual void setGreyedOut(const bool greyedOut) override;

    void setWithSelectedPhase(const bool selected) {mWithSelectedPhase = selected;}
    inline bool withSelectedPhase() { return mWithSelectedPhase;}

    virtual void setEvent(const QJsonObject& event, const QJsonObject& StudyPeriodSettings);

    void handleDrop(QGraphicsSceneDragDropEvent* e);
    QJsonArray getPhases() const;
    inline const QJsonObject &getSettings() const {return mStudyPeriodSettings;}

    virtual void setDatesVisible(bool visible);
    void redrawEvent();

    bool withSelectedDate() const;
    bool isCurveNode() const;

    void mousePressEvent(QGraphicsSceneMouseEvent* e) override;

    void updateGreyedOut();

protected:
    virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;
    virtual void dropEvent(QGraphicsSceneDragDropEvent* e) override;

    int getNumberCurveLines(const CurveSettings &cs) const;
    void paintBoxCurveParameter (QPainter *painter, const QRectF &rectBox, const CurveSettings &cs);

    void paintBoxPhases (QPainter *painter, const QRectF &rectBox);

    void resizeEventItem();
    void repositionDateItems();

    
};

#endif
