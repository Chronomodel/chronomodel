/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2018

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

#ifndef PHASEITEM_H
#define PHASEITEM_H

#include "AbstractItem.h"

bool sortEvents(QPair<int, int> e1, QPair<int, int> e2);

class PhaseItem : public AbstractItem
{
    Q_OBJECT
// members
public:
    enum { Type = UserType + 21 };

    int type() const override
    {
        // Enable the use of qgraphicsitem_cast with this item.
        return Type;
    };

    bool mControlsVisible;
    bool mControlsEnabled;
    bool matLeastOneEventSelected;
    bool mOneEventSelectedOnScene;

    static int mTitleHeight;
    static int mEltsHeight;

 void redrawPhase();

//functions
protected:

    void paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*) override;
    void mousePressEvent(QGraphicsSceneMouseEvent* e) override;

    void hoverEnterEvent(QGraphicsSceneHoverEvent* e) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent* e) override;

    // size and position of the buttons
    QRectF checkRect() const;
    QRectF insertRect() const;
    QRectF extractRect() const;

    QJsonArray getEvents() const;
    QString getTauString() const;
    // the two pixmaps of insert_Event and extract Event
    QPixmap* inPix;
    QPixmap* exPix;

public:
    PhaseItem(AbstractScene* scene, const QJsonObject& phase, QGraphicsItem* parent = nullptr);
    virtual ~PhaseItem();

    void setPhase(const QJsonObject& phase);

    void setControlsVisible(const bool visible);
    void setControlsEnabled(const bool enabled);


};

#endif
