/*
	Licensed under the MIT license.
	Made for Haiku.
*/
const char* kSignature = "application/x-vnd.przemub.HaikuWeather";

#include "App.h"
#include "MainWindow.h"

App::App(void) : BApplication(kSignature) {
	MainWindow *mw = new MainWindow();
	mw->Show();
}

int main() {
	App *mApp = new App();
	mApp->Run();
	delete mApp;
	return 0;
}
