#include <support/String.h>
#include "ErrorAlert.h"

ErrorAlert::ErrorAlert(const char* msg,const status_t err)
	: BAlert("ErrorAlert",
		(BString(msg) << " [" << err << "]").String(),"Ok")
{
}

ErrorAlert::~ErrorAlert(void)
{
}