#pragma once

#include <sstream>
#include <stdexcept>
#include <string>
#include <typeinfo>

class BaseGenerator {
public:  // TODO
	BaseGenerator() = default;  // TODO
	~BaseGenerator() = default;  // TODO

	template<typename T>
	std::string stringify(const T& t)
	{
		std::ostringstream ss;
		if (!(ss << t))  { throw std::runtime_error(std::string("error: ") + typeid(t).name()); }  // TODO: move into error handler
		return ss.str();
	}
};
