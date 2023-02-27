


#pragma once

enum PowerStatus {
	pwUnknown,
	pwVeryLow,
	pwLow,
	pwOK, 
	pwHigh,
	pwStatusCount
};
const char* const pwrStatusText[pwStatusCount] = {"unknown", "very low", "low", "OK", "high"};

/// power status change handler
///  from, to
typedef void(*OnPowerStatusChange)(PowerStatus, PowerStatus);

PowerStatus getPowerStatus();
void serPwStatusChangeHandler(OnPowerStatusChange h);
float getBattVoltage();
float getBattPercent();
void batteryUpdate(int a);
void batteryPrint();