##################################################################
# Cumulus Maemo 5.x project file for qmake
#
# Copyright (c): 2010 Axel Pauli
#
# This file is distributed under the terms of the General Public
# License. See the file COPYING for more information.
#
# $Id$
#
##################################################################

TEMPLATE = app

CONFIG = qt \
    warn_on \
    release

# CONFIG = debug qt warn_on

QT += network

HEADERS = \
    airfieldlistview.h \
    airfieldlistwidget.h \
    airfield.h \
    airregion.h \
    airspace.h \
    airspacedownloaddialog.h \    
    airspacewarningdistance.h \
    altimetermodedialog.h \
    altitude.h \
    authdialog.h \
    basemapelement.h \
    configwidget.h \
    calculator.h \
    distance.h \
    downloadmanager.h \    
    elevationcolorimage.h \
    filetools.h \
    flighttask.h \
    generalconfig.h \
    glider.h \
    glidereditor.h \
    gliderflightdialog.h \
    gliderlistwidget.h \
    gpscon.h \
    gpsnmea.h \
    gpsstatusdialog.h \
    coordedit.h \
    helpbrowser.h \
    httpclient.h \    
    hwinfo.h \
    igclogger.h \
    interfaceelements.h \
    ipc.h \
    isohypse.h \
    isolist.h \
    limitedlist.h \
    lineelement.h \
    listviewfilter.h \
    maemostyle.h \
    mainwindow.h \
    mapcalc.h \
    mapconfig.h \
    mapcontents.h \
    mapdefaults.h \
    map.h \
    mapinfobox.h \
    mapmatrix.h \
    mapview.h \
    messagehandler.h \
    multilayout.h \
    openairparser.h \
    polardialog.h \
    polar.h \
    preflightwidget.h \
    preflightgliderpage.h \
    preflightmiscpage.h \
    preflighttasklist.h \
    projectionbase.h \
    projectioncylindric.h \
    projectionlambert.h \
    protocol.h \
    proxydialog.h \
    radiopoint.h \
    reachablelist.h \
    reachablepoint.h \
    reachpointlistview.h \
    resource.h \
    rowdelegate.h \
    runway.h \
    settingspageairfields.h \
    settingspageairspace.h \
    settingspagegps.h \
    settingspageglider.h \
    settingspageinformation.h \
    settingspagelooknfeel.h \
    settingspagemapobjects.h \
    settingspagemapsettings.h \
    settingspagepersonal.h \
    settingspagesector.h \
    settingspageterraincolors.h \
    settingspageunits.h \
    signalhandler.h \
    singlepoint.h \
    sonne.h \
    sound.h \
    speed.h \
    splash.h \
    target.h \
    taskeditor.h \
    tasklistview.h \
    taskpoint.h \
    time_cu.h \
    tpinfowidget.h \
    vario.h \
    variomodedialog.h \
    vector.h \
    waitscreen.h \
    waypointcatalog.h \
    waypointlistview.h \
    waypointlistwidget.h \
    welt2000.h \
    wgspoint.h \
    whatsthat.h \
    windanalyser.h \
    windmeasurementlist.h \
    windstore.h \
    wpeditdialog.h \
    wpeditdialogpageaero.h \
    wpeditdialogpagegeneral.h \
    waypoint.h \
    wpinfowidget.h \
    wplistwidgetparent.h

SOURCES = \
    airfieldlistview.cpp \
    airfieldlistwidget.cpp \
    airfield.cpp \
    airregion.cpp \
    airspace.cpp \
    airspacedownloaddialog.cpp \    
    altimetermodedialog.cpp \
    altitude.cpp \
    authdialog.cpp \
    basemapelement.cpp \
    configwidget.cpp \
    calculator.cpp \
    distance.cpp \
    downloadmanager.cpp \
    elevationcolorimage.cpp \
    filetools.cpp \
    flighttask.cpp \
    generalconfig.cpp \
    glider.cpp \
    glidereditor.cpp \
    gliderflightdialog.cpp \
    gliderlistwidget.cpp \
    gpscon.cpp \
    gpsnmea.cpp \
    gpsstatusdialog.cpp \
    coordedit.cpp \
    helpbrowser.cpp \
    httpclient.cpp \
    hwinfo.cpp \
    igclogger.cpp \
    ipc.cpp \
    isohypse.cpp \
    isolist.cpp \
    lineelement.cpp \
    listviewfilter.cpp \
    main.cpp \
    mainwindow.cpp \
    maemostyle.cpp \
    mapcalc.cpp \
    mapconfig.cpp \
    mapcontents.cpp \
    map.cpp \
    mapinfobox.cpp \
    mapmatrix.cpp \
    mapview.cpp \
    messagehandler.cpp \
    openairparser.cpp \
    polar.cpp \
    polardialog.cpp \
    preflightwidget.cpp \
    preflightgliderpage.cpp \
    preflightmiscpage.cpp \
    preflighttasklist.cpp \
    projectionbase.cpp \
    projectioncylindric.cpp \
    projectionlambert.cpp \
    proxydialog.cpp \
    radiopoint.cpp \
    reachablelist.cpp \
    reachablepoint.cpp \
    reachpointlistview.cpp \
    rowdelegate.cpp \
    runway.cpp \
    settingspageairfields.cpp \
    settingspageairspace.cpp \
    settingspagegps.cpp \
    settingspageglider.cpp \
    settingspageinformation.cpp \
    settingspagelooknfeel.cpp \
    settingspagemapobjects.cpp \
    settingspagemapsettings.cpp \
    settingspagepersonal.cpp \
    settingspagesector.cpp \
    settingspageterraincolors.cpp \
    settingspageunits.cpp \
    signalhandler.cpp \
    singlepoint.cpp \
    sonne.cpp \
    sound.cpp \
    speed.cpp \
    splash.cpp \
    target.h \
    taskeditor.cpp \
    tasklistview.cpp \
    taskpoint.cpp \
    time_cu.cpp \
    tpinfowidget.cpp \
    vario.cpp \
    variomodedialog.cpp \
    vector.cpp \
    waitscreen.cpp \
    waypointcatalog.cpp \
    waypointlistview.cpp \
    waypointlistwidget.cpp \
    welt2000.cpp \
    wgspoint.cpp \
    whatsthat.cpp \
    windanalyser.cpp \
    windmeasurementlist.cpp \
    windstore.cpp \
    waypoint.cpp \
    wpeditdialog.cpp \
    wpeditdialogpageaero.cpp \
    wpeditdialogpagegeneral.cpp \
    wpinfowidget.cpp \
    wplistwidgetparent.cpp
    
INTERFACES = 
TARGET = cumulus

DESTDIR = .

INCLUDEPATH += ../ \
    /usr/lib/glib-2.0/include \
    /usr/include/glib-2.0 \
    /usr/include/dbus-1.0 \
    /usr/lib/dbus-1.0/include
    
DEFINES += MAEMO MAEMO5

LIBS += -lstdc++ \
    -losso \
    -lgpsbt \
    -lgps \
    -lgpsmgr \
    -llocation
    
TRANSLATIONS = cumulus_de.ts \
    cumulus_nl.ts \
    cumulus_it.ts \
    cumulus_sp.ts \
    cumulus_fr.ts
