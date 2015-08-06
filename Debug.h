//============================================================================
// Name			: Debug.h
// Author		: Peter Žužek
// License		: BSD
//============================================================================

#ifndef DEBUG_H_
#define DEBUG_H_

#include <iostream>
#include <sstream>

#define DSELF() Debug(__func__)

class Debug
{
protected:
	static constexpr bool DEBUG_ACTIVE = true;
	std::stringstream stream;
	std::stringstream before;

	template<class T, int size>
	void AddToStream(T array[size])
	{
		// TODO
		DSELF() << "array";
		for(int i = 0; i < size; ++i)
		{
			AddToStream(array[i]);
		}
	}

	template<typename T>
	void AddToStream(T add)
	{
		if(DEBUG_ACTIVE)
		{
			stream << add << ' ';
		}
	}

	template<class T>
	void AddToStream(std::basic_string<T> string)
	{
		//DSELF() << "string";
		if(DEBUG_ACTIVE)
		{
			stream << string << ' ';
		}
	}

	template<template<class, class...> class Container, class First, class... Others>
	void AddToStream(Container<First, Others...> list)
	{
		//DSELF() << "container";
		for(auto&& element : list)
		{
			AddToStream(element);
		}
	}

public:
	Debug() = default;
	Debug(Debug&& move) = default;
	Debug(const Debug& copy) = delete;

	template<typename T>
	Debug(T add)
		: Debug()
	{
		before << "Debug: ";
		AddToStream(add);
	}

	template<typename T>
	Debug& operator<<(T add)
	{
		AddToStream(add);
		return *this;
	}

	template<typename U, typename T>
	Debug(U before, T add)
		: Debug()
	{
		this->before << before;
		AddToStream(add);
	}

	virtual ~Debug()
	{
		if(DEBUG_ACTIVE)
		{
			std::cout << before.str() << stream.str() << std::endl;
		}
	}
};

template<typename T>
static Debug Warning(T message)
{
	return Debug("Warning: ", message);
}

#endif /* DEBUG_H_ */
