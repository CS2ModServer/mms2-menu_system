#include "pymenu.hpp"
#include "ISmmPlugin.h"
#include <pybind11/functional.h>

extern ISmmAPI* g_SMAPI;

namespace Source2Py {
	void PyMenuAPI::TestPrint(std::string message)
	{
		message.append("\n");
		META_CONPRINT(message.c_str());
	};
}
