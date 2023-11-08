// Header guard.

#ifndef APP_H
#define APP_H

#include <Application.h>
#include <Window.h>

extern const char* kSignature;

class App : public BApplication
{
private:
	BWindow* window;

public:
	App(void);
};

#endif
