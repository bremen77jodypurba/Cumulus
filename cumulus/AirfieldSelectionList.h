/***********************************************************************
**
**   AirfieldSelectionList.h
**
**   This file is part of KFLog.
**
************************************************************************
**
**   Copyright (c):  2014 by Axel Pauli
**
**   This file is distributed under the terms of the General Public
**   License. See the file COPYING for more information.
**
***********************************************************************/

/**
 * \class AirfieldSelectionList
 *
 * \author Axel Pauli
 *
 * \brief A widget with a list and a search function for a single airfield.
 *
 * A widget with a list and a search function for a single airfield.
 * With the help of the search function you can navigate to a certain entry in
 * the list. The currently selected entry in the list is emitted as signal,
 * if the set button is clicked.
 *
 * \date 2016
 *
 * \version 1.0
 */

#ifndef AirfieldSelectionList_h
#define AirfieldSelectionList_h

#include <QComboBox>
#include <QGroupBox>
#include <QHash>
#include <QLineEdit>
#include <QString>
#include <QWidget>

#include "singlepoint.h"

class AirfieldSelectionList : public QWidget
{
  Q_OBJECT

 private:

  Q_DISABLE_COPY ( AirfieldSelectionList )

 public:

  AirfieldSelectionList( QWidget *parent=0 );

  virtual ~AirfieldSelectionList();

  /**
   * Gets the airfield selection pointer.
   *
   * \return Pointer to airfield selection list.
   */
  QTreeWidget* getSelectionList()
  {
    return m_airfieldTreeWidget;
  }

  /**
   * Called to fill the selection box with content.
   */
  void fillSelectionList();

  void setGroupBoxTitle( QString title )
  {
    m_groupBox->setTitle( title );
  };

 protected:

  void showEvent( QShowEvent *event );

 signals:

  /**
   * Emitted, if the set button is pressed to broadcast the selected point.
   */
  void takeThisPoint( const SinglePoint* singePoint );

 private slots:

 /**
  * Called, if the clear button is clicked.
  */
  void slotClearSearchEntry();

  /**
   * Called, if the set button is clicked to take over the selected entry from
   * the combo box.
   */
  void slotSetSelectedEntry();

  /**
   * Called if the return key is pressed.
   */
  void slotReturnPressed();

  /**
   * Called if the text in the search box is modified.
   */
  void slotTextEdited( const QString& text );

 private:

  QGroupBox* m_groupBox;
  QLineEdit* m_searchEntry;
  QTreeWidget* m_airfieldTreeWidget;

  QHash<QString, SinglePoint*> m_airfieldDict;
};

#endif /* AirfieldSelectionList_h */
