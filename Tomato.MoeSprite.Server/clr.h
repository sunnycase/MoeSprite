//
// Moe Sprite
// CLR
// 作者：SunnyCase
// 创建日期：2016-03-28
#pragma once
#include "common.h"
#include "../vendors/mscoree.h"
#include "NonCopyable.h"

DEFINE_NS_SPRITE

class AppDomain : private NonCopyable
{
public:
	AppDomain(DWORD domainId, ICLRRuntimeHost2* host);
	~AppDomain();

	AppDomain(AppDomain&& other);

	DWORD GetDomainId() const;

	template<typename TFn>
	TFn* CreateDelegate(const std::wstring& assemblyName, const std::wstring& typeName, const std::wstring& methodName)
	{
		return reinterpret_cast<TFn*>(CreateDelegate(assemblyName, typeName, methodName));
	}
private:
	std::intptr_t CreateDelegate(const std::wstring& assemblyName, const std::wstring& typeName, const std::wstring& methodName);
private:
	DWORD _domainId;
	WRL::ComPtr<ICLRRuntimeHost2> _host;
};

class ClrHost
{
public:
	ClrHost();

	AppDomain CreateAppDomain(const std::wstring& entryPointAssemblyName);
private:
	WRL::ComPtr<ICLRRuntimeHost2> _host;
};

END_NS_SPRITE