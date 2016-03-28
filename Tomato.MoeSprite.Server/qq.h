//
// Moe Sprite
// QQ
// 作者：SunnyCase
// 创建日期：2016-03-25
#pragma once
#include "common.h"
#include <memory>
#include <ppltasks.h>
#include <cpprest/streams.h>
#include <cpprest/http_client.h>

DEFINE_NS_SPRITE

namespace qq
{
	struct Session
	{
		utility::string_t ptwebqq, vfwebqq, psessionid;
		int uin;
	};

	enum class State
	{
		// 未登录
		NotLogin,
		// 等待扫描 QR
		WaitForScanQR,
		CheckSignature,
		GetVfWebQQ,
		DoLogin,
		Ready
	};

	struct LoginSession
	{
		utility::string_t qrSig, checkSigUri, p_uin, p_skey, pt4_token,
			pt2gguin, uin, skey;
	};

	class QQClient
	{
	public:
		QQClient();

		void Start();
		virtual void OnProcessLoginQRCode(concurrency::streams::istream& stream);
	private:
		concurrency::task<bool> Login();
		concurrency::task<bool> CheckQRScanState();
		concurrency::task<bool> CheckSignature();
		concurrency::task<bool> GetVfWebQQ();
		concurrency::task<bool> DoLogin();
		void CheckState();
	private:
		State _state = State::NotLogin;
		web::http::client::http_client _client;
		std::shared_ptr<LoginSession> _loginSession;
		std::shared_ptr<Session> _session;
	};
}

END_NS_SPRITE