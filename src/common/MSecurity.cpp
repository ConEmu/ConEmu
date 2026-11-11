
/*
Copyright (c) 2011-present Maximus5
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the authors may not be used to endorse or promote products
   derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ''AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#define HIDE_USE_EXCEPTION_INFO

#include <mutex>

#include "defines.h"
#include "Memory.h"
#include "MAssert.h"
#include "WObjects.h"

class MNullDesc;

MNullDesc *gNullDesc = NULL;
static std::mutex securityMutex;


class MNullDesc
{
protected:
	PACL mp_ACL;
	#ifndef CONEMU_MINIMAL
	PSECURITY_DESCRIPTOR mp_NullDesc;
	SECURITY_ATTRIBUTES  m_NullSecurity;
	#endif
	PSECURITY_DESCRIPTOR mp_LocalDesc;
	SECURITY_ATTRIBUTES  m_LocalSecurity;
	HMODULE mh_AdvApi;

	typedef BOOL (WINAPI *AddAccessAllowedAce_t)(PACL pAcl, DWORD dwAceRevision, DWORD AccessMask, PSID pSid);
	AddAccessAllowedAce_t AddAccessAllowedAce;
	typedef BOOL (WINAPI *AddAccessDeniedAce_t)(PACL pAcl, DWORD dwAceRevision, DWORD AccessMask, PSID pSid);
	AddAccessDeniedAce_t AddAccessDeniedAce;
	typedef BOOL (WINAPI *AllocateAndInitializeSid_t)(PSID_IDENTIFIER_AUTHORITY pIdentifierAuthority, BYTE nSubAuthorityCount, DWORD dwSubAuthority0, DWORD dwSubAuthority1, DWORD dwSubAuthority2, DWORD dwSubAuthority3, DWORD dwSubAuthority4, DWORD dwSubAuthority5, DWORD dwSubAuthority6, DWORD dwSubAuthority7, PSID *pSid);
	AllocateAndInitializeSid_t AllocateAndInitializeSid;
	typedef DWORD (WINAPI *GetLengthSid_t)(PSID pSid);
	GetLengthSid_t GetLengthSid;
	typedef BOOL (WINAPI *InitializeAcl_t)(PACL pAcl, DWORD nAclLength, DWORD dwAclRevision);
	InitializeAcl_t InitializeAcl;
	typedef BOOL (WINAPI *InitializeSecurityDescriptor_t)(PSECURITY_DESCRIPTOR pSecurityDescriptor, DWORD dwRevision);
	InitializeSecurityDescriptor_t InitializeSecurityDescriptor;
	typedef BOOL (WINAPI *SetSecurityDescriptorDacl_t)(PSECURITY_DESCRIPTOR pSecurityDescriptor, BOOL bDaclPresent, PACL pDacl, BOOL bDaclDefaulted);
	SetSecurityDescriptorDacl_t SetSecurityDescriptorDacl;

	BOOL LoadAdvApi()
	{
		if (mh_AdvApi)
		{
			return TRUE;
		}

		for (int i = 0; i <= 1; i++)
		{
			if (i == 0)
			{
				_ASSERTE(_WIN32_WINNT_WIN7==0x601);
				if (!IsWin7())
					continue; // There was no "KernelBase.dll" in Vista and below
				mh_AdvApi = LoadLibrary(L"KernelBase.dll");
			}
			else
			{
				mh_AdvApi = LoadLibrary(L"Advapi32.dll");
			}
			
			if (!mh_AdvApi)
			{
				#ifdef _DEBUG
				mn_LastError = GetLastError();
				#endif
				return FALSE;
			}

			AddAccessAllowedAce = (AddAccessAllowedAce_t)GetProcAddress(mh_AdvApi, "AddAccessAllowedAce");
			AddAccessDeniedAce = (AddAccessDeniedAce_t)GetProcAddress(mh_AdvApi, "AddAccessDeniedAce");
			AllocateAndInitializeSid = (AllocateAndInitializeSid_t)GetProcAddress(mh_AdvApi, "AllocateAndInitializeSid");
			GetLengthSid = (GetLengthSid_t)GetProcAddress(mh_AdvApi, "GetLengthSid");
			InitializeAcl = (InitializeAcl_t)GetProcAddress(mh_AdvApi, "InitializeAcl");
			InitializeSecurityDescriptor = (InitializeSecurityDescriptor_t)GetProcAddress(mh_AdvApi, "InitializeSecurityDescriptor");
			SetSecurityDescriptorDacl = (SetSecurityDescriptorDacl_t)GetProcAddress(mh_AdvApi, "SetSecurityDescriptorDacl");

			if (AddAccessAllowedAce && AddAccessDeniedAce && AllocateAndInitializeSid &&
				GetLengthSid && InitializeAcl && InitializeSecurityDescriptor && SetSecurityDescriptorDacl)
			{
				return TRUE;
			}

	    	UnloadAdvApi();
	    	return FALSE;
	    }
		return FALSE;
	}
	void UnloadAdvApi()
	{
		if (mh_AdvApi)
		{
			FreeLibrary(mh_AdvApi);
			mh_AdvApi = NULL;
		}
	}
	
public:
	#ifdef _DEBUG
	DWORD mn_LastError;
	#endif
public:
	MNullDesc()
	{
		#ifndef CONEMU_MINIMAL
		memset(&m_NullSecurity, 0, sizeof(m_NullSecurity));
		mp_NullDesc = NULL;
		#endif
		memset(&m_LocalSecurity, 0, sizeof(m_LocalSecurity));
		mp_LocalDesc = NULL;
		mp_ACL = NULL;
		#ifdef _DEBUG
		mn_LastError = 0;
		#endif
		mh_AdvApi = NULL;
	};
	~MNullDesc()
	{
		#ifndef CONEMU_MINIMAL
		memset(&m_NullSecurity, 0, sizeof(m_NullSecurity));
		LocalFree(mp_NullDesc); mp_NullDesc = NULL;
		#endif
		memset(&m_LocalSecurity, 0, sizeof(m_LocalSecurity));
		LocalFree(mp_LocalDesc); mp_LocalDesc = NULL;
		LocalFree(mp_ACL); mp_ACL = NULL;
	};
public:
	#ifndef CONEMU_MINIMAL
	SECURITY_ATTRIBUTES* NullSecurity()
	{
		if (mp_NullDesc)
		{
			_ASSERTE(m_NullSecurity.lpSecurityDescriptor==mp_NullDesc);
			return (&m_NullSecurity);
		}

		#ifdef _DEBUG
		mn_LastError = 0;
		#endif

		SECURITY_ATTRIBUTES* lpSec = NULL;
		
		if (!LoadAdvApi())
			goto wrap;

		mp_NullDesc = (PSECURITY_DESCRIPTOR) LocalAlloc(LPTR,
		              SECURITY_DESCRIPTOR_MIN_LENGTH);

		if (mp_NullDesc == NULL)
		{
			#ifdef _DEBUG
			mn_LastError = GetLastError();
			#endif
			goto wrap;
		}

		if (!InitializeSecurityDescriptor(mp_NullDesc, SECURITY_DESCRIPTOR_REVISION))
		{
			#ifdef _DEBUG
			mn_LastError = GetLastError();
			#endif
			LocalFree(mp_NullDesc); mp_NullDesc = NULL;
			goto wrap;
		}

		// Add a null DACL to the security descriptor.
		if (!SetSecurityDescriptorDacl(mp_NullDesc, TRUE, (PACL) NULL, FALSE))
		{
			#ifdef _DEBUG
			mn_LastError = GetLastError();
			#endif
			LocalFree(mp_NullDesc); mp_NullDesc = NULL;
			goto wrap;
		}

		m_NullSecurity.nLength = sizeof(m_NullSecurity);
		m_NullSecurity.lpSecurityDescriptor = mp_NullDesc;
		m_NullSecurity.bInheritHandle = FALSE; //111122 - установка Inherit на пайпы приводит к неприятным результатам (хэндл пайпа не закрывается)
		lpSec = &m_NullSecurity;
	wrap:
		UnloadAdvApi();
		return lpSec;
	};
	#endif // CONEMU_MINIMAL
	SECURITY_ATTRIBUTES* LocalSecurity()
	{
		if (mp_LocalDesc)
		{
			_ASSERTE(m_LocalSecurity.lpSecurityDescriptor==mp_LocalDesc);
			return (&m_LocalSecurity);
		}

		#ifdef _DEBUG
		mn_LastError = 0;
		#endif

		//#ifdef CONEMU_MINIMAL
		//		// Возможно, есть переменная окружения CESECURITYNAME?
		//		int nLen = 1024;
		//		char* pszEncoded = (char*)calloc(1024,1);
		//		if (!pszEncoded)
		//			goto DoLoad;
		//		nLen = GetEnvironmentVariableA(CESECURITYNAME, pszEncoded, 1024);
		//		if (nLen > 0)
		//		{
		//			if (nLen > 1024)
		//			{
		//				nLen ++;
		//				free(pszEncoded);
		//				pszEncoded = (char*)calloc(nLen,1);
		//				nLen = GetEnvironmentVariableA(CESECURITYNAME, pszEncoded, 1024);
		//				if (!nLen)
		//				{
		//					free(pszEncoded);
		//					goto DoLoad;
		//				}
		//			}
		//		}
		//		free(pszEncoded);
		//		DoLoad:
		//#endif
		
		SECURITY_ATTRIBUTES* lpSec = NULL;
		BYTE NetworkSid[12] = {01,01,00,00,00,00,00,05,02,00,00,00};
		BYTE SidSystem[12]  = {01,01,00,00,00,00,00,05,18,00,00,00};
		PSID pNetworkSid = (PSID)NetworkSid;
		PSID pSidSystem = (PSID)SidSystem;
		alignas(TOKEN_USER) BYTE user[sizeof(TOKEN_USER) + SECURITY_MAX_SID_SIZE] = {};
		HANDLE token = nullptr;
		DWORD userSize = 0;
		PSID pSidUser = nullptr;
		DWORD dwSize = 0;

		if (!LoadAdvApi())
			goto wrap;
		if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token)) goto wrap;
		if (!GetTokenInformation(token, TokenUser, user, sizeof(user), &userSize)) goto wrap;
		pSidUser = reinterpret_cast<TOKEN_USER*>(user)->User.Sid;

		mp_LocalDesc = (PSECURITY_DESCRIPTOR) LocalAlloc(LPTR,
		               SECURITY_DESCRIPTOR_MIN_LENGTH);

		if (mp_LocalDesc == NULL)
		{
			#ifdef _DEBUG
			mn_LastError = GetLastError();
			#endif
			goto wrap;
		}

		if (!InitializeSecurityDescriptor(mp_LocalDesc, SECURITY_DESCRIPTOR_REVISION))
		{
			#ifdef _DEBUG
			mn_LastError = GetLastError();
			#endif
			LocalFree(mp_LocalDesc); mp_LocalDesc = NULL;
			goto wrap;
		}

		dwSize = sizeof(ACL) //-V104 //-V119 //-V103
		         + sizeof(ACCESS_DENIED_ACE) + GetLengthSid(pNetworkSid)
		         + sizeof(ACCESS_ALLOWED_ACE) + GetLengthSid(pSidUser)
		         + sizeof(ACCESS_ALLOWED_ACE) + GetLengthSid(pSidSystem);
		mp_ACL = (PACL)LocalAlloc(LPTR, dwSize); //-V106

		if (!mp_ACL || !InitializeAcl(mp_ACL, dwSize, ACL_REVISION))
		{
			#ifdef _DEBUG
			mn_LastError = GetLastError();
			#endif
			goto wrap;
		}

		if (!AddAccessDeniedAce(mp_ACL, ACL_REVISION, GENERIC_ALL, pNetworkSid))
		{
			#ifdef _DEBUG
			mn_LastError = GetLastError();
			#endif
			goto wrap;
		}

		if (!AddAccessAllowedAce(mp_ACL, ACL_REVISION, GENERIC_ALL, pSidUser)
			|| !AddAccessAllowedAce(mp_ACL, ACL_REVISION, GENERIC_ALL, pSidSystem))
		{
			#ifdef _DEBUG
			mn_LastError = GetLastError();
			#endif
			goto wrap;
		}

		if (!SetSecurityDescriptorDacl(mp_LocalDesc, TRUE, mp_ACL, FALSE))
		{
			#ifdef _DEBUG
			mn_LastError = GetLastError();
			#endif
			LocalFree(mp_LocalDesc); mp_LocalDesc = NULL;
			goto wrap;
		}

	//SetLocal:
		m_LocalSecurity.nLength = sizeof(m_LocalSecurity);
		m_LocalSecurity.lpSecurityDescriptor = mp_LocalDesc;
		m_LocalSecurity.bInheritHandle = FALSE; //111122 - установка Inherit на пайпы приводит к неприятным результатам (хэндл пайпа не закрывается)
		lpSec = &m_LocalSecurity;
	wrap:
		if (token) CloseHandle(token);
		if (!lpSec)
		{
			LocalFree(mp_LocalDesc); mp_LocalDesc = nullptr;
			LocalFree(mp_ACL); mp_ACL = nullptr;
		}
		UnloadAdvApi();
		return lpSec;
	};
};



#ifndef CONEMU_MINIMAL
SECURITY_ATTRIBUTES* NullSecurity()
{
	std::lock_guard<std::mutex> lock(securityMutex);
	if (!gNullDesc) gNullDesc = new MNullDesc();

	return gNullDesc->NullSecurity();
}
#endif

SECURITY_ATTRIBUTES* LocalSecurity()
{
	std::lock_guard<std::mutex> lock(securityMutex);
	if (!gNullDesc) gNullDesc = new MNullDesc();

	return gNullDesc->LocalSecurity();
}


void ShutdownSecurity()
{
	std::lock_guard<std::mutex> lock(securityMutex);
	if (gNullDesc)
	{
		delete gNullDesc;
		gNullDesc = NULL;
	}
}

namespace {
struct PipeIdentity
{
	BYTE sid[SECURITY_MAX_SID_SIZE];
	DWORD integrity = 0;
};

bool ReadPipeIdentity(HANDLE token, PipeIdentity& identity)
{
	alignas(TOKEN_USER) BYTE user[sizeof(TOKEN_USER) + SECURITY_MAX_SID_SIZE] = {};
	alignas(TOKEN_MANDATORY_LABEL) BYTE label[sizeof(TOKEN_MANDATORY_LABEL) + SECURITY_MAX_SID_SIZE] = {};
	DWORD size = 0;
	if (!GetTokenInformation(token, TokenUser, user, sizeof(user), &size)
		|| !CopySid(sizeof(identity.sid), identity.sid, reinterpret_cast<TOKEN_USER*>(user)->User.Sid)
		|| !GetTokenInformation(token, TokenIntegrityLevel, label, sizeof(label), &size)) return false;
	const PSID sid = reinterpret_cast<TOKEN_MANDATORY_LABEL*>(label)->Label.Sid;
	const auto count = *GetSidSubAuthorityCount(sid);
	if (!count) return false;
	identity.integrity = *GetSidSubAuthority(sid, count - 1);
	return true;
}
}

bool IsLocalPipeClientTrusted(HANDLE pipe, bool requireIntegrity)
{
	HANDLE token = nullptr;
	PipeIdentity client = {}, server = {};
	if (!ImpersonateNamedPipeClient(pipe)) return false;
	const bool opened = OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, TRUE, &token) != FALSE;
	const bool readClient = opened && ReadPipeIdentity(token, client);
	if (token) CloseHandle(token);
	const bool reverted = RevertToSelf() != FALSE;
	if (!reverted) TerminateProcess(GetCurrentProcess(), ERROR_ACCESS_DENIED);
	if (!readClient || !reverted) return false;
	token = nullptr;
	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token)) return false;
	const bool readServer = ReadPipeIdentity(token, server);
	CloseHandle(token);
	return readServer && (EqualSid(client.sid, server.sid) || IsWellKnownSid(client.sid, WinLocalSystemSid))
		&& (!requireIntegrity || client.integrity >= server.integrity);
}
