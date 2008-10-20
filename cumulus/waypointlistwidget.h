/***********************************************************************
**
**   waypointlistwidget.h
**
**   This file is part of Cumulus.
**
************************************************************************
**
**   Copyright (c):  2002 by André Somers
**
**   This file is distributed under the terms of the General Public
**   Licence. See the file COPYING for more information.
**
**   $Id$
**
***********************************************************************/

#ifndef WAYPOINT_LISTWIDGET_H
#define WAYPOINT_LISTWIDGET_H

#include <QWidget>

#include "waypoint.h"
#include "wplistwidgetparent.h"

/**
 * This widget provides a list of waypoints and a means to select one.
 * @author André Somers
 */
class WaypointListWidget : public WpListWidgetParent
{
    Q_OBJECT

public:
    WaypointListWidget(QWidget *parent=0);

    ~WaypointListWidget();

    /**
     * @returns a pointer to the currently highlighted waypoint.
     */
    wayPoint *getSelectedWaypoint();

    /**
     * @updates the currently highlighted waypoint after editing.
     */
    void updateSelectedWaypoint(wayPoint &);

    /**
     * @removes the currently highlighted waypoint.
     */
    void deleteSelectedWaypoint();

    /**
     * @adds a waypoint.
     */
    void addWaypoint(wayPoint &);

    /**
     * Retrieves the waypoints from the map contents and fills
     * the list.
     */
    void fillWpList();


private: // Private methods

class _WaypointItem : public QTreeWidgetItem
    {
    public:
        _WaypointItem(QTreeWidget*, wayPoint&);
        wayPoint &wp;
    };
};

#endif
