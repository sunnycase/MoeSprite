// Tomato.MoeSprite.Server.cpp : �������̨Ӧ�ó������ڵ㡣
//

#include "stdafx.h"
#include "qq.h"
#include <iostream>

using namespace NS_SPRITE;

int main()
{
	//std::locale::global(std::locale(""));

	std::cout << "Press enter to exit." << std::endl;
	std::cout << "[INFO] ��ȡ��ά��..." << std::endl;
	qq::QQClient client;
	client.Start();
	std::cout << "[INFO] ��ȡ��ά����ɣ���ʹ���ֻ�QQɨ�衣" << std::endl;

	while (std::cin.get() != 10);
    return 0;
}

