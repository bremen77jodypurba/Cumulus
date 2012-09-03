/***********************************************************************
**
**   wpinfowidget.h
**
**   This file is part of Cumulus.
**
************************************************************************
**
**   Copyright (c):  2002      by André Somers
**                   2008-2012 by Axel Pauli
**
**   This file is distributed under the terms of the General Public
**   License. See the file COPYING for more information.
**
**   $Id$
**
***********************************************************************/

/**
 * \class WPInfoWidget
 *
 * \author André Somers, Axel Pauli
 *
 * \brief This widget shows the details of a waypoint together with some
 * command buttons for executing different actions.
 *
 * This widget shows the details of a waypoint together with some
 * command buttons for executing different actions.
 *
 * \date 2002-2012
 *
 * $Id$
 */

#ifndef WP_INFO_WIDGET_H
#define WP_INFO_WIDGET_H

#include <QWidget>
#include <QTimer>
#include <QTextEdit>
#include <QHBoxLayout>
#include <QPushButton>
#include <QMainWindow>
#include <QShortcut>

#include "waypoint.h"
#include "tpinfowidget.h"

class MainWindow;

class WPInfoWidget : public QWidget
{
  Q_OBJECT

private:

  Q_DISABLE_COPY ( WPInfoWidget )

public:

  WPInfoWidget( MainWindow *parent=0 );

  virtual ~WPInfoWidget();

  /**
   * This method is called by MainWindow to set the view to which
   * there must be returned and the waypoint to view.
   */
  bool showWP(int lastView, const Waypoint& wp);

public slots:

  /**
   * No descriptions
   */
  void slot_SwitchBack();

  /**
   * This slot is called by the KeepOpen button to...
   * yes... keep the dialog open. :-)
   * Any timer that would close the dialog will be stopped.
   */
  void slot_KeepOpen();

signals:

  /**
   * Emitted if a waypoint should be added to the list.
   */
  void addWaypoint(Waypoint& wp);

  /**
   * Emitted if a waypoint should be selected.
   */
  void selectWaypoint(Waypoint*, bool);

  /**
   * Emitted if a waypoint should be deleted.
   */
  void deleteWaypoint(Waypoint& wp);

  /**
   * Emitted if a new home position is selected
   */
  void newHomePosition(const QPoint&);

  /**
   * Emitted to move the map to the new home position
   */
  void gotoHomePosition();

  /**
   * Emitted if a waypoint was edited.
   */
  void waypointEdited(Waypoint& wp);

protected:

  /**
   * Called, if the widget will be shown.
   */
  void showEvent(QShowEvent *);

private:

  /**
   * Set the visibility of the widget buttons
   */
  void setButtonsVisibility();

  /**
   * This method actually fills the widget with the info.
   */
  void writeText();


private slots:

  /**
   * This slot get called on the timer timeout.
   */
  void slot_timeout();

  /**
   * This slot is called if the Add as Waypoint button is clicked.
   */
  void slot_addAsWaypoint();

  /**
   * This slot is called if the Home button is clicked.
   */
  void slot_setNewHome();

  /**
   * This slot is called if the Select Waypoint button is clicked.
   */
  void slot_selectWaypoint();

  /**
   * This slot is called if the Unselect Waypoint button is clicked.
   */
  void slot_unselectWaypoint();

  /**
   * This slot is called if the arrival button is clicked.
   */
  void slot_arrival();

  /**
   * This slot is called if the arrival widget is closed
   */
  void slot_arrivalClose();

  /**
   * This slot is called if the edit button is clicked.
   */
  void slot_edit();

  /**
   * This slot is called if the delete button is clicked.
   */
  void slot_delete();

  /**
   * This slot is called when the waypoint editing is finished.
   */
  void slot_edited( Waypoint& wp);

private: // Private attributes

  /**
   * This is the widget that actually contains the info we
   * want to represent.
   */
  QTextEdit* text;

  QHBoxLayout* buttonrow1;
  QHBoxLayout* buttonrow2;

  QPushButton* cmdClose;
  QPushButton* cmdKeep;
  QPushButton* cmdAddWaypoint;
  QPushButton* cmdHome;
  QPushButton* cmdSelectWaypoint;
  QPushButton* cmdUnselectWaypoint;
  QPushButton* cmdArrival;
  QPushButton* cmdEdit;
  QPushButton* cmdRemove;

  QShortcut* scClose;

  /**
   * Reference to a timer. When this timer fires, the view
   * is returned to the previous one. This allow for a brief
   * display of data (i.e.: what is the frequency of my target
   * field?) after which the view is automatically returned to
   * the last one.
   */
  QTimer* timer;

  /** TimerCount */
  int _timerCount;

  /** Reference to the waypoint who's details to be displayed. */
  Waypoint _wp;

  /** last selected waypoint  */
  Waypoint myWp;

  /** contains the ID of the last view (the view that called this one) */
  int _lastView;

  /** contains a reference to the parent, the application */
  MainWindow *mainWindow;

  /** that shall store a home position change */
  bool homeChanged;
};

#endif
