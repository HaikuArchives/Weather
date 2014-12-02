// MIT license

#include <Window.h>
#include <GridLayout.h>
#include <View.h>

class MainWindow : public BWindow {
private:
	BGridLayout* layout;

public:
	MainWindow(void);
	void MessageRecieved(BMessage *);
	void AddView(BView *);
};
