/***********************************************************************
**
**   preflightwidget.h
**
**   This file is part of Cumulus.
**
************************************************************************
**
**   Copyright (c):  2003      by André Somers
**                   2008-2012 by Axel Pauli
**
**   This file is distributed under the terms of the General Public
**   License. See the file COPYING for more information.
**
**   $Id$
**
***********************************************************************/

#ifndef _PreFlightWidget_h
#define _PreFlightWidget_h

#include <QString>
#include <QTabWidget>

class PreFlightGliderPage;
class PreFlightMiscPage;
class PreFlightTaskList;
class PreFlightWaypointPage;
class Waypoint;

/**
 * \class PreFlightWidget
 *
 * \author André Somers, Axel Pauli
 *
 * \brief Widget for pre-flight settings.
 *
 * This widget provides an interface to set all the pre-flight settings like
 * glider type, copilot, task, amount of water taken on, etc.
 *
 * \date 2003-2011
 */
class PreFlightWidget : public QWidget
{
  Q_OBJECT

private:

  /**
   * That macro forbids the copy constructor and the assignment operator.
   */
  Q_DISABLE_COPY( PreFlightWidget )

public:

  /**
   * Constructor
   * @param parent Pointer to parent widget
   * @param name Name of the page to be displayed. Current options: "taskselection".
   *             Any other string will select the glider page.
   */
  PreFlightWidget(QWidget *parent, const char* name);

  /**
   * Destructor
   */
  virtual ~PreFlightWidget();

protected:

  /** Used to handle language change events */
  virtual void changeEvent( QEvent* event );

  void keyReleaseEvent( QKeyEvent* event );

private:

  /** Sets all widget labels, which need a translation. */
  void setLabels();

signals:

  /**
   * This signal is emitted if the settings are changed
   */
  void settingsChanged();

  /**
   * This signal is emitted if a new waypoint is selected.
   */
  void newWaypoint(Waypoint *, bool);

  /**
   * This signal is emitted before the widget is closed.
   * MainWindow will use it to update the current view setting.
   */
  void closeConfig();

  /**
   * This signal is emitted, if a new task has been selected. IGC logger
   * uses this info to restart the flight recording.
   */
  void newTaskSelected();

protected slots:

  /**
   * Called if dialog is accepted (OK button is clicked)
   */
  void slot_accept();

  /**
   * Called if dialog is rejected (X button is clicked)
   */
  void slot_reject();

private slots:

  // shortcuts for switching between tabulators
  void slot_keyLeft();
  void slot_keyRight();

private:

  PreFlightTaskList *taskpage;
  PreFlightGliderPage *gliderpage;
  PreFlightMiscPage *miscpage;
  PreFlightWaypointPage *wppage;

  QTabWidget* tabWidget;
};

#endif
