/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef __PROTOCOL__
#define __PROTOCOL_

const uint32 kInitiateConnection = 'inim';
const uint32 kOpenConnection = 'opnm';
const uint32 kCloseConnection = 'clsm';
const uint32 kServerQuitting = 'srvq';
const uint32 kApplyValue = 'appl';
const uint32 kActivateWindow = 'acti';

#define kInvokePoint "be:invokePoint"
#define kTargetName "be:targetName"
#define kInitialValue "be:initialValue"
#define kNewValue "be:value"
#define kRequestedValues "be:requestedValues"
#define kProvidedValues "be:providedValues"
#define kServerAddress "be:serverAddress"
#define kClientAddress "be:clientAddress"


#define kColorPickerType "application/x-vnd.Be.colorPicker"

#endif
