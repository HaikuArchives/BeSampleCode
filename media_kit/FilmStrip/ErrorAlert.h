#ifndef _ERRORALERT_H_
#define _ERRORALERT_H_

#include <Alert.h>

class ErrorAlert : public BAlert {
public:
	ErrorAlert(const char* message,const status_t err);
	virtual ~ErrorAlert(void);
};

#endif // #ifndef _ERRORALERT_H_