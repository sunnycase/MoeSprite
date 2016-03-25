//
// Tomato Media
// ����ͷ�ļ�
// ���ߣ�SunnyCase
// ����ʱ�䣺2015-08-04
//
#pragma once

#define DEFINE_NS_SPRITE namespace Tomato { namespace Sprite {
#define END_NS_SPRITE }}
#define NS_SPRITE Tomato::Sprite

#include "platform.h"

#define DEFINE_PROPERTY_GET(name, type) __declspec(property(get = get_##name)) type name
#define ARGUMENT_NOTNULL_HR(pointer) if(!(pointer)) return E_POINTER

#include <functional>
#include <memory>

///<summary>�ս���</summary>
template<typename TCall>
class finalizer final
{
public:
	finalizer(TCall&& action)
		:action(std::forward<decltype(action)>(action))
	{

	}

	finalizer(finalizer&&) = default;
	finalizer& operator=(finalizer&&) = default;

	~finalizer()
	{
		action();
	}
private:
	TCall action;
};

template<typename TCall>
finalizer<TCall> make_finalizer(TCall&& action)
{
	return finalizer<TCall>(std::forward<decltype(action)>(action));
}

#ifdef _WIN32

template<class T>
struct cotaskmem_deleter
{
	void operator()(T* handle) const noexcept
	{
		CoTaskMemFree(handle);
	}
};

template<class T>
using unique_cotaskmem = std::unique_ptr<T, cotaskmem_deleter<T>>;

#endif

#include <chrono>
typedef std::ratio<1, 10000000> hn;
typedef std::chrono::duration<long long, hn> hnseconds;