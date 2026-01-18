/*
 * BKEL_APP_sendDiagData.h
 *
 *  Created on: Jan 18, 2026
 *      Author: PNYcom
 */

#ifndef ASW_INC_BKEL_APP_SENDDIAGDATA_H_
#define ASW_INC_BKEL_APP_SENDDIAGDATA_H_

#include "BKEL_APP_protocol.h"

extern void AppSendDiagPWMOut();
extern void AppSendDiagPWMIn();
extern void AppSendDiagADC1Val();
extern void AppSendDiagADC2Val();
extern void AppSendDiagGPOPinState();
extern void AppSendDiagGPIPinState();
extern void AppSendDiagLD2PinState();

#endif /* ASW_INC_BKEL_APP_SENDDIAGDATA_H_ */
