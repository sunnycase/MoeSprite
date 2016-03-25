// Tomato.MoeSprite.Server.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "qq.h"
#include <iostream>

using namespace NS_SPRITE;

int main()
{
	//std::locale::global(std::locale(""));

	std::cout << "Press enter to exit." << std::endl;
	std::cout << "[INFO] 获取二维码..." << std::endl;
	qq::QQClient client;
	client.Start();
	std::cout << "[INFO] 获取二维码完成，请使用手机QQ扫描。" << std::endl;

	while (std::cin.get() != 10);
    return 0;
}

