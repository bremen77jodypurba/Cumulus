/***********************************************************************
**
**   preflightflarmpage.h
**
**   This file is part of Cumulus.
**
************************************************************************
**
**   Copyright (c):  2012 by Axel Pauli
**
**   This file is distributed under the terms of the General Public
**   License. See the file COPYING for more information.
**
**   $Id$
**
***********************************************************************/

/**
 * \class PreflightFlarmPage
 *
 * \author Axel Pauli
 *
 * \brief A widget for pre-flight Flarm IGC settings.
 *
 * A widget for pre-flight Flarm IGC settings.
 *
 * \date 2012
 *
 * \version $Id$
 */

#ifndef PREFLIGHT_FLARM_PAGE_H_
#define PREFLIGHT_FLARM_PAGE_H_

#include <QWidget>
#include <QMessageBox>

#include "flarm.h"

class QComboBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QTimer;
class FlightTask;

class PreFlightFlarmPage : public QWidget
{
  Q_OBJECT

 private:

  Q_DISABLE_COPY ( PreFlightFlarmPage )

 public:

  PreFlightFlarmPage(FlightTask* ftask=0, QWidget *parent=0);

  virtual ~PreFlightFlarmPage();

 private slots:

 /** Requests the Flarm data from the device.*/
 void slotRequestFlarmData();

 /** Sends all IGC data to the Flarm. */
 void slotWriteFlarmData();

 /** Sets all IGC data as defined in Cumulus. */
 void slotSetIgcData();

 /** Clears all IGC data in the editable widgets. */
 void slotClearIgcData();

 /** Called to update version info. */
  void slotUpdateVersions( const Flarm::FlarmVersion& info );

  /** Called to update error info. */
  void slotUpdateErrors( const Flarm::FlarmError& info );

  /** Called to update configuration info. */
  void slotUpdateConfiguration( const QStringList& info );

  /** Called if the connection timer has expired. */
  void slotTimeout();

  /** Called if the widget is closed. */
  void slotClose();

 private:

  /** Loads the available Flarm data into the label displays. */
  void loadFlarmData();

  /** Toggles operation of buttons. */
  void enableButtons( const bool toggle );

  /** Shows a popup message box to the user. */
  void messageBox( QMessageBox::Icon icon, QString message, QString title="" );

  QLabel*    hwVersion;
  QLabel*    swVersion;
  QLabel*    obstVersion;
  QLabel*    flarmId;
  QLabel*    errSeverity;
  QLabel*    errCode;
  QComboBox* logInt;
  QLineEdit* pilot;
  QLineEdit* copil;
  QLineEdit* gliderId;
  QLineEdit* gliderType;
  QLineEdit* compId;
  QLineEdit* compClass;
  QLineEdit* task;

  QPushButton* readButton;
  QPushButton* writeButton;
  QPushButton* setButton;
  QPushButton* clearButton;

  QTimer*    m_timer;

  FlightTask* m_ftask;
};

#endif