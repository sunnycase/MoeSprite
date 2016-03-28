//
// Moe Sprite
// QQ
// 作者：SunnyCase
// 创建日期：2016-03-25
#include "stdafx.h"
#include "qq.h"
#include <cpprest/http_client.h>
#include <cpprest/filestream.h>
#include <sstream>

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
			if (!response.headers().match(U("Set-Cookie"), qrSig))
				throw std::runtime_error("cannot get login QR signature.");
			{
				const auto qrSigOff = qrSig.find(L"qrsig=");
				session->qrSig.assign(qrSig.begin() + qrSigOff, qrSig.begin() + qrSig.find(L';', qrSigOff));
			}
			return qrCodeProcessor(response.body());
		});
	}

	class set_useragent_stage : public http_pipeline_stage
	{
	public:
		set_useragent_stage()
		{

		}

		virtual pplx::task<http_response> propagate(http_request request) override
		{
			request.headers().add(L"User-Agent", L"Mozilla / 5.0 (Windows NT 10.0; WOW64; rv:44.0) Gecko / 20100101 Firefox / 44.0");
			return this->next_stage()->propagate(request);
		}
	};
}

QQClient::QQClient()
	:_loginSession(std::make_shared<LoginSession>()),
	_client(U("https://ssl.ptlogin2.qq.com/")),
	_session(std::make_shared<Session>())
{
	_client.add_handler(static_cast<std::shared_ptr<http_pipeline_stage>>(std::make_shared<set_useragent_stage>()));
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
		return response.extract_string(true).then([=](string_t content)
		{
			if (content.find(L"未失效") != string_t::npos)
				_state = State::WaitForScanQR;
			else if (content.find(L"已失效") != string_t::npos)
				_state = State::NotLogin;
			else if (content.find(L"认证中") != string_t::npos)
				_state = State::WaitForScanQR;
			else
			{
				auto uriOff = content.find(L"http");
				_loginSession->checkSigUri.assign(content.begin() + uriOff, content.begin() + content.find(L"',", uriOff));

				string_t cookies;
				if (!response.headers().match(U("Set-Cookie"), cookies))
					throw std::runtime_error("cannot get ptwebqq.");
				{
					constexpr wchar_t ptwebqqStr[] = L"ptwebqq=";
					const auto ptWebQQOff = cookies.find(ptwebqqStr);
					_session->ptwebqq.assign(cookies.begin() + ptWebQQOff + ARRAYSIZE(ptwebqqStr) - 1,
						cookies.begin() + cookies.find(L';', ptWebQQOff));
				}
				_state = State::CheckSignature;
			}
			return true;
		});
	});
}

concurrency::task<bool> QQClient::CheckSignature()
{
	http_request request(methods::GET);
	request.set_request_uri(_loginSession->checkSigUri);

	return _client.request(request).then([this](http_response& response)
	{
		if (response.status_code() != status_codes::Found)
			throw std::runtime_error("cannot check signature.");

		string_t cookies;
		if (!response.headers().match(U("Set-Cookie"), cookies))
			throw std::runtime_error("cannot get ptwebqq.");
		{
			const auto p_uinOff = cookies.find(L"p_uin");
			_loginSession->p_uin.assign(cookies.begin() + p_uinOff, cookies.begin() + cookies.find(L';', p_uinOff));
			const auto p_skeyOff = cookies.find(L"p_skey");
			_loginSession->p_skey.assign(cookies.begin() + p_skeyOff, cookies.begin() + cookies.find(L';', p_skeyOff));
			const auto pt4_tokenOff = cookies.find(L"pt4_token");
			_loginSession->pt4_token.assign(cookies.begin() + pt4_tokenOff, cookies.begin() + cookies.find(L';', pt4_tokenOff));

			const auto pt2gguinOff = cookies.find(L"pt2gguin");
			_loginSession->pt2gguin.assign(cookies.begin() + pt2gguinOff, cookies.begin() + cookies.find(L';', pt2gguinOff));
			const auto uinOff = cookies.find(L"uin");
			_loginSession->uin.assign(cookies.begin() + uinOff, cookies.begin() + cookies.find(L';', uinOff));
			const auto skeyOff = cookies.find(L"skey");
			_loginSession->skey.assign(cookies.begin() + skeyOff, cookies.begin() + cookies.find(L';', skeyOff));
		}
		_state = State::GetVfWebQQ;
		return true;
	});
}

concurrency::task<bool> QQClient::GetVfWebQQ()
{
	http_request request(methods::GET);
	request.set_request_uri(U("http://s.web2.qq.com/api/getvfwebqq?ptwebqq=") + _session->ptwebqq + U("&clientid=53999199&psessionid=&t=0.1"));
	auto& headers = request.headers();
	headers.add(U("Refer"), U("http://s.web2.qq.com/proxy.html?v=20130916001&callback=1&id=1"));
	stringstream_t cookies;
	cookies << L"ptwebqq=" << _session->ptwebqq << L';' << _loginSession->p_uin
		<< L';' << _loginSession->p_skey << L';' << _loginSession->pt4_token
		<< L';' << _loginSession->pt2gguin << L';' << _loginSession->uin << L';' << _loginSession->skey;
	headers.add(U("Cookie"), cookies.str());

	return _client.request(request).then([this](http_response& response)
	{
		//if (response.status_code() != status_codes::OK)
		//	throw std::runtime_error("cannot get vfwebqq.");
		return response.extract_string(true).then([=](string_t str)
		{
			return true;
		});
		return response.extract_json().then([=](web::json::value& value)
		{
			auto& result = value[U("result")].as_object();
			_session->vfwebqq = result[U("vfwebqq")].as_string();
			_state = State::DoLogin;
			return true;
		});
	});
}

concurrency::task<bool> QQClient::DoLogin()
{
	http_request request(methods::POST);
	request.set_request_uri(U("http://d1.web2.qq.com/channel/login2"));
	request.headers().add(U("Refer"), U("http://s.web2.qq.com/proxy.html?v=20130916001&callback=1&id=2"));
	request.set_body(U(R"(r={"ptwebqq":")") + _session->ptwebqq + U(R"(","clientid":53999199,"psessionid":"","status":"online"})"), U("application/x-www-form-urlencoded"));

	return _client.request(request).then([this](http_response& response)
	{
		if (response.status_code() != status_codes::OK)
			throw std::runtime_error("cannot do login.");
		return response.extract_json().then([=](web::json::value& value)
		{
			auto& result = value[U("result")].as_object();
			_session->uin = result[U("uin")].as_integer();
			_session->psessionid = result[U("psessionid")].as_string();
			_state = State::Ready;
			return false;
		});
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
	case State::CheckSignature:
		t = CheckSignature();
		break;
	case State::GetVfWebQQ:
		t = GetVfWebQQ();
		break;
	case State::DoLogin:
		t = DoLogin();
		break;
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
