/* ---------------------------------------------------------------------

Copyright © CNRS	2014 - 2026

Authors :
	Philippe LANOS
	Helori LANOS
	Philippe DUFRESNE

This software is a computer program whose purpose is to
create chronological models of archeological data using Bayesian statistics.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.

SPDX-License-Identifier: GPL-3.0-or-later
--------------------------------------------------------------------- */

#ifndef SETPROJECTSTATE_H
#define SETPROJECTSTATE_H

#include <QUndoCommand>
#include <QJsonObject>
#include <QString>
#include <QPointer>

#include "Project.h"


class SetProjectState: public QUndoCommand
{
public:
    SetProjectState(Project* project,
                    const QJsonObject& prevState,
                    const QJsonObject& nextState,
                    const Project::ReasonId id);

    ~SetProjectState() override = default;
    // QUndoCommand interface
    void undo() override;
    void redo() override;

    // Fusion de commandes consécutives du même type
   // int  id() const override { return 0xBEEF; }          // identifiant de type
   // bool mergeWith(const QUndoCommand *other) override;

private:
    //QPointer<Project> mProject;          // devient nullptr dès que le Project est delete‑d
    Project* mProject;
    QJsonObject mPrevState;
    QJsonObject mNextState;
    Project::ReasonId mReason;

};


#endif
