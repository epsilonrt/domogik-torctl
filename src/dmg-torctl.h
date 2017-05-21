/**
 * Copyright © 2016-2017 epsilonRT, All rights reserved.
 *
 * This software is governed by the CeCILL license under French law and
 * abiding by the rules of distribution of free software.  You can  use, 
 * modify and/ or redistribute the software under the terms of the CeCILL
 * license as circulated by CEA, CNRS and INRIA at the following URL
 * <http://www.cecill.info>. 
 * 
 * The fact that you are presently reading this means that you have had
 * knowledge of the CeCILL license and that you accept its terms.
 *
 * @file
 * @brief
 */
#ifndef _DMG_TORCTL_HEADER_
#define _DMG_TORCTL_HEADER_
#include <stdlib.h>
#include <time.h>
#include <gxPL.h>
#include <gxPL/stdio.h>
#include <sysio/tinfo.h>
#include <sysio/doutput.h>
#include "version-git.h"

/* constants ================================================================ */
typedef enum {
  eTorCtlManual,
  eTorCtlAuto
} eTorCtlMode;

/* structures =============================================================== */
struct xTorCtlOutput {
  char sName[GXPL_NAME_MAX];
  char sType[GXPL_NAME_MAX];
  char sOnValue[GXPL_NAME_MAX];
  char sOffValue[GXPL_NAME_MAX];
  int  iValue;
  unsigned long ulAdco;
  eTorCtlMode eCurrentMode;
  eTorCtlMode ePreviousMode;
  bool bOutputStatusRequested;
};

struct xTorCtlContext {

  xDoutPort * xOutPort;
  struct xTorCtlOutput * xOutput;
  
  gxPLMessage * xMsg;
};

/* types ==================================================================== */
typedef struct xTorCtlContext xTorCtlContext;

/* public variables ========================================================= */
/*
 * Configurable parameters
 */
extern xTorCtlContext xCtx;

/* internal public functions ================================================ */
/*
 * Main Task
 */
void vMain (gxPLSetting * setting);
void vParseAdditionnalOptions (int argc, char ** argv);

/*
 * xPL Device
 */
gxPLDevice * xDeviceCreate(gxPLSetting * setting);

/*
 * TorCtl Interface
 */
int iTorCtlOpen (gxPLDevice * device);
int iTorCtlClose (gxPLDevice * device);
int iTorCtlPoll (gxPLDevice * device);
int iTorCtlSendStat (gxPLDevice * device);

/* ========================================================================== */
#endif /* _DMG_TORCTL_HEADER_ defined */
