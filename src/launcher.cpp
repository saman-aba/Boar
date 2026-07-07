#include "launcher.h"
#include <QDir>

Launcher *Launcher::InstanceSetter::Instance = nullptr;

std::unique_ptr<Launcher> Launcher::Create(int argc, char *argv[]) {
    return std::make_unique<Launcher>(argc, argv);
}

Launcher::Launcher(int argc, char *argv[]): _argc(argc), _argv(argv)
, _initialWorkingDir(QDir::currentPath() + '/') 
{

}

Launcher::~Launcher() {
	InstanceSetter::Instance = nullptr;
}

int Launcher::exec() {
	auto result = executeApplication();
	return result;	
}

int Launcher::executeApplication() {
	return 0;
}
