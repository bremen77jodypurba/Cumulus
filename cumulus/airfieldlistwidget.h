/***********************************************************************
**
**   airfieldlistwidget.h
**
**   This file is part of Cumulus.
**
************************************************************************
**
**   Copyright (c):  2002      by André Somers
**                   2008-2009 by Axel Pauli
**
**   This file is distributed under the terms of the General Public
**   Licence. See the file COPYING for more information.
**
**   $Id$
**
***********************************************************************/

#ifndef AIRFIELD_LISTWIDGET_H
#define AIRFIELD_LISTWIDGET_H

#include <QWidget>
#include <QVector>

#include "waypoint.h"
#include "wplistwidgetparent.h"
#include "mapcontents.h"

/**
 * This widget provides a list of airfields and a means to select one.
 * @author André Somers
 */

class AirfieldListWidget : public WpListWidgetParent
{
    Q_OBJECT

public:

    AirfieldListWidget( QVector<enum MapContents::MapContentsListID> &itemList,
                        QWidget *parent=0);

    ~AirfieldListWidget();

    /**
     * @returns a pointer to the currently highlighted waypoint.
     */
    wayPoint *getSelectedWaypoint();

    /**
     * Clears and refills the airfield item list
     */
    void refillWpList();

    /**
     * Generates the complete list of airfield items; display is done by the filter
     */
    void fillWpList();

private:

    wayPoint *wp;
    QVector<enum MapContents::MapContentsListID> itemList;

class _AirfieldItem : public QTreeWidgetItem
    {
    public:
        _AirfieldItem(Airfield*);
        Airfield* airport;
    };
};

#endif
