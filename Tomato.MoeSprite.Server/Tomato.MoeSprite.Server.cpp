// Tomato.MoeSprite.Server.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "qq.h"
#include "clr.h"
#include <iostream>

using namespace NS_SPRITE;

class myClient : public qq::QQClient
{
public:
	// 通过 QQClient 继承
	virtual void OnReady() override
	{
	}
};

__interface
	IApplication : public IUnknown
{
	void Run();
};

int main()
{
	//std::locale::global(std::locale(""));

	std::cout << "Press enter to exit." << std::endl;
#if 0
	std::cout << "[INFO] 获取二维码..." << std::endl;
	myClient client;
	client.Start();
	std::cout << "[INFO] 获取二维码完成，请使用手机QQ扫描。" << std::endl;
#else
	ClrHost clr;
	auto appDomain{ clr.CreateAppDomain(L"Tomato.MoeSprite.Heart.dll") };
	WRL::ComPtr<IApplication> objUnk;
	auto fn = appDomain.CreateDelegate<void _stdcall(IApplication**)>(L"Tomato.MoeSprite.Heart", L"Tomato.MoeSprite.Heart.Startup", L"CreateApplication");
	fn(&objUnk);
	objUnk->Run();
	//auto fn = appDomain.CreateDelegate<int32_t _stdcall(int32_t)>(L"Tomato.MoeSprite.Heart", L"Tomato.MoeSprite.Heart.Program", L"Add");
	//std::cout << "Call Add(1):" << fn(1) << std::endl;
#endif

	while (std::cin.get() != 10);
    return 0;
}

