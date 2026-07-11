#pragma once
#include <QApplication>

namespace Core {

class Sandbox final : public QApplication {
public:
	Sandbox(int &argc, char **argv);

	Sandbox(const Sandbox &other) = delete;
	Sandbox &operator=(const Sandbox &other) = delete;

	static Sandbox &Instance() {
		assert(QCoreApplication::instance() != nullptr);
		return *static_cast<Sandbox*>(QCoreApplication::instance());
	}

	~Sandbox();
	int start();

private:
	bool _started = false;
};

};
