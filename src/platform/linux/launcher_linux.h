#pragma once

#include "launcher.h"

namespace Platform {

class Launcher : public Core::Launcher {
public:
	Launcher(int argc, char *argv[]);

	int exec() override;

private:
	bool launchUpdater(UpdaterLaunch action) override;
	bool _updating = false;
};
};
