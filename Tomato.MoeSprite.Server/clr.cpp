//
// Moe Sprite
// CLR
// 作者：SunnyCase
// 创建日期：2016-03-28
#include "stdafx.h"
#include "clr.h"
#include "../vendors/mscoree.h"
#include <sstream>

using namespace NS_SPRITE;

namespace
{
	// The name of the CoreCLR native runtime DLL.
	constexpr wchar_t coreCLRDll[] = LR"(runtimes\clr\CoreCLR.dll)";

	FnGetCLRRuntimeHost GetGetCLRRuntimeHost()
	{
		static struct loader_t
		{
			loader_t()
			{
				_module = ::LoadLibrary(coreCLRDll);
				ThrowWin32IfNot(_module);
				fn = reinterpret_cast<decltype(fn)>(GetProcAddress(_module, "GetCLRRuntimeHost"));
				ThrowWin32IfNot(fn);
			}

			~loader_t()
			{
				FreeLibrary(_module);
			}

			HMODULE _module = nullptr;
			FnGetCLRRuntimeHost fn = nullptr;
		} loader;
		return loader.fn;
	}
#define MAX_LONGPATH 256
	void RemoveExtensionAndNi(_In_z_ wchar_t* fileName)
	{
		// Remove extension, if it exists
		wchar_t* extension = wcsrchr(fileName, L'.');
		if (extension != NULL)
		{
			extension[0] = L'\0';

			// Check for .ni
			size_t len = wcslen(fileName);
			if (len > 3 &&
				fileName[len - 1] == U('i') &&
				fileName[len - 2] == U('n') &&
				fileName[len - 3] == U('.'))
			{
				fileName[len - 3] = U('\0');
			}
		}
	}

	bool TPAListContainsFile(std::wstringstream& m_tpaList, _In_z_ wchar_t* fileNameWithoutExtension, _In_reads_(countExtensions) wchar_t** rgTPAExtensions, int countExtensions)
	{
		if (m_tpaList.str().empty()) return false;

		for (int iExtension = 0; iExtension < countExtensions; iExtension++)
		{
			wchar_t fileName[MAX_LONGPATH];
			wcscpy_s(fileName, MAX_LONGPATH, U("\\")); // So that we don't match other files that end with the current file name
			wcscat_s(fileName, MAX_LONGPATH, fileNameWithoutExtension);
			wcscat_s(fileName, MAX_LONGPATH, rgTPAExtensions[iExtension] + 1);
			wcscat_s(fileName, MAX_LONGPATH, U(";")); // So that we don't match other files that begin with the current file name

			if (wcsstr(m_tpaList.str().c_str(), fileName))
			{
				return true;
			}
		}
		return false;
	}

	std::wstring GetCurrentDirectory()
	{
		const auto length = ::GetCurrentDirectory(0, nullptr);
		std::wstring str(length - 1, L'\0');
		ThrowWin32IfNot(::GetCurrentDirectory(length, &str.front()));
		return str;
	}

	void AddFilesFromDirectoryToTPAList(std::wstringstream& m_tpaList, const std::wstring& targetPath, _In_reads_(countExtensions) wchar_t** rgTPAExtensions, int countExtensions)
	{
		wchar_t assemblyPath[MAX_LONGPATH];

		for (int iExtension = 0; iExtension < countExtensions; iExtension++)
		{
			wcscpy_s(assemblyPath, MAX_LONGPATH, targetPath.c_str());

			const size_t dirLength = targetPath.length();
			wchar_t* const fileNameBuffer = assemblyPath + dirLength;
			const size_t fileNameBufferSize = MAX_LONGPATH - dirLength;

			wcscat_s(assemblyPath, rgTPAExtensions[iExtension]);
			WIN32_FIND_DATA data;
			HANDLE findHandle = FindFirstFile(assemblyPath, &data);

			if (findHandle != INVALID_HANDLE_VALUE) {
				do {
					if (!(data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
						// It seems that CoreCLR doesn't always use the first instance of an assembly on the TPA list (ni's may be preferred
						// over il, even if they appear later). So, only include the first instance of a simple assembly name to allow
						// users the opportunity to override Framework assemblies by placing dlls in %CORE_LIBRARIES%

						// ToLower for case-insensitive comparisons
						wchar_t* fileNameChar = data.cFileName;
						while (*fileNameChar)
						{
							*fileNameChar = towlower(*fileNameChar);
							fileNameChar++;
						}

						// Remove extension
						wchar_t fileNameWithoutExtension[MAX_LONGPATH];
						wcscpy_s(fileNameWithoutExtension, MAX_LONGPATH, data.cFileName);

						RemoveExtensionAndNi(fileNameWithoutExtension);

						// Add to the list if not already on it
						if (!TPAListContainsFile(m_tpaList, fileNameWithoutExtension, rgTPAExtensions, countExtensions))
						{
							const size_t fileLength = wcslen(data.cFileName);
							const size_t assemblyPathLength = dirLength + fileLength;
							wcsncpy_s(fileNameBuffer, fileNameBufferSize, data.cFileName, fileLength);
							m_tpaList.write(assemblyPath, assemblyPathLength);
							m_tpaList.write(U(";"), 1);
						}
						else
						{
							//std::cout << W("Not adding ") << targetPath << data.cFileName << W(" to the TPA list because another file with the same name is already present on the list") << Logger::endl;
						}
					}
				} while (0 != FindNextFile(findHandle, &data));

				FindClose(findHandle);
			}
		}
	}

	std::wstring GetTpaList() {
		std::wstringstream m_tpaList;
		wchar_t *rgTPAExtensions[] = {
			U("*.ni.dll"),		// Probe for .ni.dll first so that it's preferred if ni and il coexist in the same dir
			U("*.dll"),
			U("*.ni.exe"),
			U("*.exe"),
		};

		// Add files from %CORE_LIBRARIES% if specified
		wchar_t coreLibraries[MAX_LONGPATH];
		size_t outSize;
		if (_wgetenv_s(&outSize, coreLibraries, MAX_LONGPATH, U("CORE_LIBRARIES")) == 0 && outSize > 0)
		{
			wcscat_s(coreLibraries, MAX_LONGPATH, U("\\"));
			AddFilesFromDirectoryToTPAList(m_tpaList, coreLibraries, rgTPAExtensions, _countof(rgTPAExtensions));
		}
		else
		{
			//*m_log << W("CORE_LIBRARIES not set; skipping") << Logger::endl;
			//*m_log << W("You can set the environment variable CORE_LIBRARIES to point to a") << Logger::endl;
			//*m_log << W("path containing additional platform assemblies,") << Logger::endl;
		}

		AddFilesFromDirectoryToTPAList(m_tpaList, GetCurrentDirectory() + LR"(\runtimes\clr\lib\)", rgTPAExtensions, _countof(rgTPAExtensions));

		return m_tpaList.str();
	}
}

ClrHost::ClrHost()
{
	ThrowIfFailed(GetGetCLRRuntimeHost()(IID_ICLRRuntimeHost2, static_cast<IUnknown**>(&_host)));
	ThrowIfFailed(_host->Authenticate(CORECLR_HOST_AUTHENTICATION_KEY));
	ThrowIfFailed(_host->Start());
}

AppDomain ClrHost::CreateAppDomain(const std::wstring & entryPointAssemblyName)
{
	DWORD domainId;
	DWORD appDomainFlags = APPDOMAIN_ENABLE_PLATFORM_SPECIFIC_APPS | APPDOMAIN_ENABLE_PINVOKE_AND_CLASSIC_COMINTEROP;
	const wchar_t* property_keys[] =
	{
		L"TRUSTED_PLATFORM_ASSEMBLIES",
		L"APP_PATHS",
		L"NATIVE_DLL_SEARCH_DIRECTORIES",
	};

	const auto cntDir = GetCurrentDirectory();

	const std::wstring values[] = { 
		GetTpaList(),
		cntDir + LR"(\app\)",
		cntDir + LR"(\runtimes\clr\)"
	};
	// The concrete values of the properties
	const wchar_t* property_values[] = {
		// TRUSTED_PLATFORM_ASSEMBLIES
		values[0].c_str(),
		// APP_PATHS
		values[1].c_str(),
		values[2].c_str()
	};

	// Calculate the number of elements in the array
	int nprops = sizeof(property_keys) / sizeof(wchar_t*);
	ThrowIfFailed(_host->CreateAppDomainWithManager(
		entryPointAssemblyName.c_str(),
		appDomainFlags,
		NULL,
		NULL,
		nprops,
		property_keys,
		property_values,
		&domainId));
	return{ domainId, _host.Get() };
}

AppDomain::AppDomain(DWORD domainId, ICLRRuntimeHost2 * host)
	:_domainId(domainId), _host(host)
{
}

AppDomain::~AppDomain()
{
	if (_domainId)
	{
		_host->UnloadAppDomain(_domainId, true);
		_domainId = 0;
	}
}

AppDomain::AppDomain(AppDomain && other)
	:_domainId(other._domainId), _host(std::move(other._host))
{
	other._domainId = 0;
}

std::intptr_t AppDomain::CreateDelegate(const std::wstring& assemblyName, const std::wstring & typeName, const std::wstring & methodName)
{
	std::intptr_t method = 0;
	ThrowIfFailed(_host->CreateDelegate(
		_domainId,
		assemblyName.c_str(),
		typeName.c_str(),
		methodName.c_str(),
		&method));
	return method;
}
