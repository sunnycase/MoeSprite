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

#undef DispatchMessage

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
		Ready,
	};

	struct LoginSession
	{
		utility::string_t qrSig, checkSigUri, p_uin, p_skey, pt4_token,
			pt2gguin, uin, skey;
	};

	struct GroupIncomeMessage
	{

	};

	class QQClient
	{
	public:
		QQClient();

		void Start();
	protected:
		virtual void OnProcessLoginQRCode(concurrency::streams::istream& stream);
		virtual void OnReady() = 0;
	private:
		concurrency::task<bool> Login();
		concurrency::task<bool> CheckQRScanState();
		concurrency::task<bool> CheckSignature();
		concurrency::task<bool> GetVfWebQQ();
		concurrency::task<bool> DoLogin();
		void StartMessagePump();
		concurrency::task<bool> UpdateInfo();
		concurrency::task<bool> Poll();
		void SetCookie(web::http::http_headers& headers) const;
		void CheckState();

		void DispatchMessage(web::json::object& message);
	private:
		State _state = State::NotLogin;
		std::shared_ptr<web::http::client::http_client> _client;
		web::http::client::http_client _swebqq, _d1webqq, _pollClient;
		std::shared_ptr<LoginSession> _loginSession;
		std::shared_ptr<Session> _session;
	};
}

END_NS_SPRITE