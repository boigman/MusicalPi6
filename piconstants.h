#ifndef PICONSTANTS
#define PICONSTANTS

// Copyright 2023 by Linwood Ferguson, licensed under GNU GPLv3

// These are real constants used at compile time.

// The settable items' defaults are in outsettings.cpp as defaults in the setups.

#define MUSICALPI_MAXPAGES 1000   // Maximum pages in any PDF -- minimal impact to make bigger

// review/play screen maximum "up" sizes - moderate memory impact if larger (but indirect large impact on cache as below)

#define MUSICALPI_MAXROWS 2
#define MUSICALPI_MAXCOLUMNS 4

// A Good rule of thumb is cores - 1

#define MUSICALPI_THREADS 3

#define MUSICALPI_BACKGROUND_COLOR_NORMAL "white"
#define MUSICALPI_BACKGROUND_COLOR_PLAYING "black"
#define MUSICALPI_POPUP_BACKGROUND_COLOR "rgb(240,240,200)"

#define MUSICALPI_SETTINGS_TIPOVERLAY_FONT_SIZE  "36px"
#define MUSICALPI_SETTINGS_PAGENUMBER_FONT_SIZE  "16px"

// Define this to get colored borders on key widgets (from stylesheet in main)
//#define MUSICALPI_DEBUG_WIDGET_BORDERS

// SplashBackend seems to render better quality and only slightly slower (based on 2017 testing -- may be different now)

//#define MUSICALPI_POPPLER_BACKEND Poppler::Document::RenderBackend::SplashBackend
#define MUSICALPI_POPPLER_BACKEND Poppler::Document::RenderBackend::QPainterBackend

// Midi Player queue and debug control information

#define MUSICALPI_ALSALOWWATER 70
#define MUSICALPI_ALSAHIGHWATER 170
#define MUSICALPI_ALSAMAXOUTPUTBUFFER 512
#define MUSICALPI_ALSAQUEUECHUNKSIZE 50
#define MUSICALPI_ALSAPACINGINTERVAL 10
#define MUSICALPI_MIDIPLAYER_STATUSUPDATERATE 500
#define MUSICALPI_MEAURE_MARKER_TAG "Measure "

// Utility macro for managing pointers and logging

#define DELETE_LOG(X) if(X != NULL) { qDebug() << "Freeing " #X; delete X; X = NULL; }

#endif // PICONSTANTS
