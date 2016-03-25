//
// Moe Sprite
// QQ
// 作者：SunnyCase
// 创建日期：2016-03-25
#include "stdafx.h"
#include "qq.h"
#include <cpprest/http_client.h>
#include <cpprest/filestream.h>

using namespace NS_SPRITE::qq;
using namespace concurrency;
using namespace web::http;
using namespace web::http::client;
using namespace utility;

namespace
{

	// 获取二维码
	task<void> GetLoginQRCode(http_client& client, std::shared_ptr<LoginSession> session, std::function<task<void>(streams::istream&)>&& qrCodeProcessor)
	{
		return client.request(methods::GET, U("https://ssl.ptlogin2.qq.com/ptqrshow?appid=501004106&e=0&l=M&s=5&d=72&v=4&t=0.1"))
			.then([qrCodeProcessor = std::move(qrCodeProcessor),
			session = std::move(session)](http_response& response)
		{
			if (response.status_code() != status_codes::OK)
				throw std::runtime_error("cannot get login QR code.");

			string_t qrSig;
			if(!response.headers().match(U("Set-Cookie"), qrSig))
				throw std::runtime_error("cannot get login QR signature.");
			{
				const auto qrSigOff = qrSig.find(L"qrsig=");
				session->qrSig.assign(qrSig.begin() + qrSigOff, qrSig.begin() + qrSig.find(L';', qrSigOff));
			}
			return qrCodeProcessor(response.body());
		});
	}
}

QQClient::QQClient()
	:_loginSession(std::make_shared<LoginSession>()),
	_client(U("https://ssl.ptlogin2.qq.com/")),
	_session(std::make_shared<Session>())
{
}

void QQClient::Start()
{
	CheckState();
}

void QQClient::OnProcessLoginQRCode(concurrency::streams::istream & stream)
{
	streams::file_buffer<uint8_t>::open(U("qr.png"), std::ios::out | std::ios::binary | std::ios::trunc)
		.then([=](streams::streambuf<uint8_t> file)
	{
		return stream.read_to_end(file).then([](size_t) {
			std::cout << "[INFO] QR Code saved to qr.png." << std::endl;
		});
	});
}

task<bool> QQClient::Login()
{
	return GetLoginQRCode(_client, _loginSession, [this](streams::istream & stream)
	{
		OnProcessLoginQRCode(stream);
		return task_from_result();
	}).then([this]
	{
		_state = State::WaitForScanQR;
		return true;
	});
}

concurrency::task<bool> QQClient::CheckQRScanState()
{
	http_request request(methods::GET);
	request.set_request_uri(U("https://ssl.ptlogin2.qq.com/ptqrlogin?webqq_type=10&remember_uin=1&login2qq=1&aid=501004106&u1=http%3A%2F%2Fw.qq.com%2Fproxy.html%3Flogin2qq%3D1%26webqq_type%3D10&ptredirect=0&ptlang=2052&daid=164&from_ui=1&pttype=1&dumy=&fp=loginerroralert&action=0-0-157510&mibao_css=m_webqq&t=1&g=1&js_type=0&js_ver=10143&login_sig=&pt_randsalt=0"));
	auto& headers = request.headers();
	headers.add(U("Refer"), U("https://ui.ptlogin2.qq.com/cgi-bin/login?daid=164&target=self&style=16&mibao_css=m_webqq&appid=501004106&enable_qlogin=0&no_verifyimg=1&s_url=http%3A%2F%2Fw.qq.com%2Fproxy.html&f_url=loginerroralert&strong_login=1&login_state=10&t=20131024001"));
	headers.add(U("Cookie"), _loginSession->qrSig);
	return _client.request(request).then([this](http_response& response)
	{
		if (response.status_code() != status_codes::OK)
			throw std::runtime_error("cannot check QR scan state.");
		return response.extract_string(true).then([this](string_t content)
		{
			if(content.find(L"未失效") != string_t::npos)
				_state = State::WaitForScanQR;
			else if(content.find(L"已失效") != string_t::npos)
				_state = State::NotLogin;
			else if (content.find(L"认证中") != string_t::npos)
				_state = State::WaitForScanQR;
			else
			{
				auto uriOff = content.find(L"http");
				_loginSession->checkSigUri.assign(content.begin() + uriOff, content.begin() + content.find(L"',", uriOff));
				_state = State::GetPtWebQQ;
			}
			return true;
		});
	});
}

concurrency::task<bool> QQClient::GetPtWebQQ()
{
	http_request request(methods::GET);
	request.set_request_uri(_loginSession->checkSigUri);
	request.headers().add(U("Refer"), U("http://s.web2.qq.com/proxy.html?v=20130916001&callback=1&id=1"));
	
	return _client.request(request).then([this](http_response& response)
	{
		if (response.status_code() != status_codes::Found)
			throw std::runtime_error("cannot get ptwebqq.");
		string_t ptwebqq;
		if (!response.headers().match(U("Set-Cookie"), ptwebqq))
			throw std::runtime_error("cannot get ptwebqq.");
		{
			const auto ptWebQQOff = ptwebqq.find_first_of(L"ptwebqq=");
			_session->ptwebqq.assign(ptwebqq.begin() + ptWebQQOff, ptwebqq.begin() + ptwebqq.find_first_of(L';', ptWebQQOff));
		}

		return true;
	});
}

void QQClient::CheckState()
{
	task<bool> t;
	switch (_state)
	{
	case State::NotLogin:
		t = Login();
		break;
	case State::WaitForScanQR:
		t = CheckQRScanState();
		break;
	case State::GetPtWebQQ:
		t = GetPtWebQQ();
	default:
		break;
	}
	if (t != task<bool>())
	{
		t.then([this](bool next)
		{
			if (next) CheckState();
		});
	}
}
