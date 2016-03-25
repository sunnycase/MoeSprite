//
// Moe Sprite
// QQ
// ���ߣ�SunnyCase
// �������ڣ�2016-03-25
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
		utility::string_t ptwebqq;
	};

	enum class State
	{
		// δ��¼
		NotLogin,
		// �ȴ�ɨ�� QR
		WaitForScanQR,
		GetPtWebQQ
	};

	struct LoginSession
	{
		utility::string_t qrSig, checkSigUri;
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
		concurrency::task<bool> GetPtWebQQ();
		void CheckState();
	private:
		State _state = State::NotLogin;
		web::http::client::http_client _client;
		std::shared_ptr<LoginSession> _loginSession;
		std::shared_ptr<Session> _session;
	};
}

END_NS_SPRITE