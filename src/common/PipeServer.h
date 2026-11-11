
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

#pragma once

#include <limits>
#include "ConEmuPipeMode.h"
#include "MSecurity.h"
#include "CmdLine.h" // required for PointToName

#ifdef _DEBUG
	#define USEPIPELOG
	//#undef USEPIPELOG
	#include "Common.h"
	#define DEBUGSTRPPCMD(s) //OutputDebugStringW(s)
#else
	#undef USEPIPELOG
	#define DEBUGSTRPPCMD(s)
#endif

// Defines GuiMessageBox
#include "ConEmuCheck.h"

#ifndef PIPEBUFSIZE
#define PIPEBUFSIZE 4096
#endif

#undef _GetTime
#if !defined(_DEBUG)
	#define _GetTime GetTickCount
	//#pragma message("--PipeServer.h in Release mode")
	#define PipeOutputDbg(x)
#elif defined(__GNUC__)
	#define _GetTime GetTickCount
	//#pragma message("--PipeServer.h in __GNUC__ mode")
	#define PipeOutputDbg(x)
#else
	#if defined(CONEMU_MINIMAL)
	#define _GetTime GetTickCount
	#else
	#define _GetTime timeGetTime
	#pragma comment(lib, "winmm.lib")
	#endif
	//#pragma message("--PipeServer.h in _DEBUG mode")
	#define PipeOutputDbg(x) OutputDebugStringW(x)
#endif

#include "WThreads.h"

enum PipeState
{
	STARTING_STATE = 0,
	STARTED_STATE = 1,
	CREATEPIPE_STATE = 2,
	PIPECREATED_STATE = 3,
	CONNECTING_STATE = 4,
	CONNECTED_STATE = 5,
	READING_STATE = 6,
	WRITING_STATE = 7,
	DISCONNECTING_STATE = 100,
	DISCONNECTED_STATE = 101,
	CLOSING_STATE = 200,
	CLOSED_STATE = 201,
	TERMINATED_STATE = 255,
	//TERMINATE_CALL_STATE,
	THREAD_FINISHED_STATE = 256,
};


#ifdef USEPIPELOG
	#include <intrin.h>

	//#define getThreadId() WIN3264TEST(((DWORD*) __readfsdword(24))[9],GetCurrentThreadId())
	#define getThreadId() GetCurrentThreadId()

	// Originally from http://preshing.com/20120522/lightweight-in-memory-logging
	namespace PipeServerLogger
	{
		struct Event
		{
			#ifdef _DEBUG
			DWORD tid;       // Thread ID
			#endif
			const char* msg; // Info
			int pipe;
			DWORD param;
			#ifdef _DEBUG
			DWORD tick;      // Current tick
			#endif
		};
	 
		static const int BUFFER_SIZE = 256;   // Must be a power of 2
		extern Event g_events[BUFFER_SIZE];
		extern LONG g_pos;
	 
		inline void Log(int pipe, const char* msg, DWORD param)
		{
			// Get next event index
			LONG i = _InterlockedIncrement(&g_pos);
			// Write an event at this index
			Event* e = g_events + (i & (BUFFER_SIZE - 1));  // Wrap to buffer size
			#ifdef _DEBUG
			e->tid = getThreadId();
			e->tick = _GetTime();
			#endif
			e->msg = msg;
			e->pipe = pipe;
			e->param = param;
		}
	}

	#undef getThreadId

	#define PLOG(m) PipeServerLogger::Log(pPipe->nIndex,m,pPipe->dwState)
	#define PLOG2(m,v) PipeServerLogger::Log(pPipe->nIndex,m,v)
	#define PLOG3(i,m,v) PipeServerLogger::Log(i,m,v)
#else
	#define PLOG(m)
	#define PLOG2(m,v)
	#define PLOG3(i,m,v)
#endif


// struct - для того, чтобы выделять память простым calloc. "new" не прокатывает, т.к. Crt может быть не инициализирован (ConEmuHk).

// Класс предполагает, что первый DWORD в (T*) это размер передаваемого блока в байтах!
template <class T>
struct PipeServer
{
	protected:
		#define PipeInstMagic 0xE9B6376A

		struct PipeInst
		{
			// Index in m_Pipes
			DWORD  nIndex;
			// Padding and magic check (PipeInstMagic)
			DWORD  nMagic;

			// Common
			HANDLE hPipeInst; // Pipe Handle

			// Server Thread
			HANDLE hThread;
			DWORD nThreadId;

			#ifdef _DEBUG
			BOOL   bFlagged;
			#endif

			DWORD  dwConnErr;
			DWORD  dwConnWait;

			BOOL   bDelayedWrite;
			BOOL   fWriteSuccess;
			DWORD  dwWriteErr;

			DWORD  nTerminateDelay, nTerminateDelay1, nTerminateDelay2, nTerminateDelay3;

			BOOL   bBreakConnection;
			
			HANDLE hEvent; // Overlapped event
			OVERLAPPED oOverlap;
			BOOL fPendingIO;
			PipeState dwState;
			//BOOL bSkipTerminate;
			DWORD nCreateBegin, nCreateEnd, nStartedTick, nTerminatedTick;
			DWORD nCreateError; // Thread creation error?
			
			wchar_t sErrorMsg[128];

			int nLongServerReading;
			#ifdef _DEBUG
			int nCreateCount, nConnectCount, nCallCount, nAnswCount;
			SYSTEMTIME stLastCall, stLastEndCall;
			#endif

			#ifdef _DEBUG
			wchar_t szDbgInfo[128];
			#endif
			
			// Request (from client)
			DWORD cbReadSize, cbMaxReadSize;
			T* ptrRequest;
			T* AllocRequestBuf(DWORD anSize)
			{
				if (ptrRequest && (cbMaxReadSize >= anSize))
					return ptrRequest;

				if (ptrRequest)
					free(ptrRequest);

				ptrRequest = (T*)malloc(anSize);
				cbMaxReadSize = anSize;
				return ptrRequest;
			};
			// Answer (to client)
			DWORD cbReplySize, cbMaxReplySize;
			T* ptrReply; // !!! Память выделяется в mfn_PipeServerCommand
			//T* AllocReplyBuf(DWORD anSize)
			//{
			//	if (ptrReply && (cbMaxReplySize >= anSize))
			//		return ptrReply;
			//	if (ptrReply)
			//		free(ptrReply);
			//	ptrReply = (T*)malloc(anSize);
			//	cbMaxReplySize = anSize;
			//	return ptrReply;
			//};
			//// Event descriptors
			//HANDLE hTermEvent;
			//HANDLE hServerSemaphore;

			// Pointer to object
			PipeServer *pServer;

			//HANDLE hThreadEnd;
		};
	public:
		typedef BOOL (WINAPI* PipeServerConnected_t)(LPVOID pInst, LPARAM lParam);
		typedef BOOL (WINAPI* PipeServerCommand_t)(LPVOID pInst, T* pCmd, T* &ppReply, DWORD &pcbReplySize, DWORD &pcbMaxReplySize, LPARAM lParam);
	protected:
		bool mb_Initialized;
		bool mb_Terminate;
		bool mb_UseForceTerminate;
		bool mb_StopFromDllMain;
		bool mb_Overlapped;
		bool mb_LoopCommands;
		bool mb_InputOnly;
		int  mn_Priority;
		wchar_t ms_PipeName[MAX_PATH];
		int mn_MaxCount;
		int mn_PipesToCreate;
		int mn_DummyAnswerSize;
		PipeInst *m_Pipes/*[mn_MaxCount]*/;
		PipeInst *mp_ActivePipe;
		
		LPSECURITY_ATTRIBUTES mlp_Sec;
		
		// *** Callback function and it's LParam ***
		
		// Сервером получена команда. Выполнить, вернуть результат.
		// Для облегчения жизни - сервер кеширует данные, калбэк может использовать ту же память (*pcbMaxReplySize)
		PipeServerCommand_t mfn_PipeServerCommand;
		
		// Вызывается после подключения клиента к серверному пайпу, но перед PipeServerRead и PipeServerCommand_t
		// (которых может быть много на одном подключении)
		PipeServerConnected_t mfn_PipeServerConnected;
		// Уведомление, что клиент отвалился (или его отвалили). Вызывается только если был mfn_PipeServerConnected
		PipeServerConnected_t mfn_PipeServerDisconnected;
		
		// Вызывается после того, как создан Pipe Instance (CreateNamedPipeW)
		typedef BOOL (WINAPI* PipeServerReady_t)(LPVOID pInst, LPARAM lParam);
		PipeServerReady_t mfn_PipeServerReady;
		
		// освободить память, отведенную под результат
		typedef void (WINAPI* PipeServerFree_t)(T* pReply, LPARAM lParam);
		PipeServerFree_t mfn_PipeServerFree;
		
		// Params
		BOOL mb_ReadyCalled;
		LPARAM m_lParam; // указан при StartPipeServer, передается в калбэки
	protected:
		// Event descriptors
		HANDLE mh_TermEvent;
		//BOOL   mb_Terminate;
		HANDLE mh_ServerSemaphore;

		void DumpError(PipeInst *pPipe, LPCWSTR asFormat, BOOL abShow = TRUE)
		{
			wchar_t sTitle[128], sText[MAX_PATH*2];
			DWORD dwErr = GetLastError();

			PLOG("DumpError");

			msprintf(pPipe->sErrorMsg, countof(pPipe->sErrorMsg), asFormat/*i.e. L"ConnectNamedPipe failed with 0x%08X."*/, dwErr);

			if (abShow)
			{
				msprintf(sText, countof(sText), L"%s\n%s", pPipe->sErrorMsg, ms_PipeName);
				msprintf(sTitle, countof(sTitle), L"PipeServerError, PID=%u, TID=%u",
				                GetCurrentProcessId(), GetCurrentThreadId());
				GuiMessageBox(NULL, sText, sTitle, MB_ICONSTOP|MB_SYSTEMMODAL);
			}
			else
			{
				msprintf(sText, countof(sText), L"PipeServerError, PID=%u, TID=%u\n  %s\n  %s\n",
				                GetCurrentProcessId(), GetCurrentThreadId(),
				                pPipe->sErrorMsg, ms_PipeName);
				PipeOutputDbg(sText);
			}
		}

		void _Disconnect(PipeInst* pPipe, bool bDisconnect, bool bClose, bool bDelayBeforeClose = true)
		{
			_ASSERTEX(pPipe && (pPipe->nMagic == PipeInstMagic) && (pPipe->nIndex < (DWORD)mn_MaxCount) && (pPipe == (m_Pipes+pPipe->nIndex)));

			if (!pPipe->hPipeInst || (pPipe->hPipeInst == INVALID_HANDLE_VALUE))
			{
				return;
			}

			if (bDisconnect)
			{
				pPipe->dwState = DISCONNECTING_STATE;
				PLOG("DisconnectNamedPipe");
				DisconnectNamedPipe(pPipe->hPipeInst);
				PLOG("DisconnectNamedPipe done");
				pPipe->dwState = DISCONNECTED_STATE;
			}

			if (bClose)
			{
				#if 0
				// Wait a little
				if (bDelayBeforeClose && !mb_Terminate)
				{
					Sleep(10);
				}
				#endif

				pPipe->dwState = CLOSING_STATE;
				PLOG("CloseHandle");
				CloseHandle(pPipe->hPipeInst);
				PLOG("CloseHandle done");
				pPipe->hPipeInst = NULL;
				pPipe->dwState = CLOSED_STATE;
			}
		}

		int WaitOverlapped(PipeInst* pPipe, DWORD* pcbDataSize)
		{
			BOOL fSuccess = FALSE;
			// Wait for overlapped connection
			HANDLE hConnWait[2] = {mh_TermEvent, pPipe->hEvent};
			DWORD nWait = (DWORD)-1, dwErr = (DWORD)-1, dwErrT = (DWORD)-1;
			DWORD nTimeout = 1000; //RELEASEDEBUGTEST(1000,30000);
			DWORD nStartTick = GetTickCount();
			DWORD nDuration = 0, nMaxConnectDuration = 10000;
			pPipe->nLongServerReading = 0;

			while (!mb_Terminate)
			{
				PLOG("WaitOverlapped.WaitMulti");
				nWait = WaitForMultipleObjects(2, hConnWait, FALSE, nTimeout);
				PLOG("WaitOverlapped.WaitMulti done");
				nDuration = (GetTickCount() - nStartTick);

				if (nWait == WAIT_OBJECT_0)
				{
					_ASSERTEX(mb_Terminate==true);
					DWORD t1 = _GetTime();
					TODO("Suspicion long process");
					PLOG("WaitOverlapped.Disconnect");
					_Disconnect(pPipe, true, false);
					PLOG("WaitOverlapped.Disconnect done");
					DWORD t2 = _GetTime();
					pPipe->nTerminateDelay1 = (t2 - t1);
					pPipe->nTerminateDelay += pPipe->nTerminateDelay1;
					return 0; // Завершение сервера
				}
				else if (nWait == (WAIT_OBJECT_0+1))
				{
					PLOG("WaitOverlapped.Overlapped");
					fSuccess = GetOverlappedResult(pPipe->hPipeInst, &pPipe->oOverlap, pcbDataSize, FALSE);
					dwErr = GetLastError();
					PLOG("WaitOverlapped.Overlapped done");

					if (fSuccess || dwErr == ERROR_MORE_DATA)
						return 1; // OK, клиент подключился

					#ifdef _DEBUG
					_ASSERTEX(dwErr==ERROR_BROKEN_PIPE); // Канал был закрыт?
					SetLastError(dwErr);
					#endif
					return 2; // Требуется переподключение
				}
				else if (nWait == WAIT_TIMEOUT)
				{
					PLOG("WaitOverlapped.Overlapped2");
					fSuccess = GetOverlappedResult(pPipe->hPipeInst, &pPipe->oOverlap, pcbDataSize, FALSE);
					dwErrT = GetLastError();
					PLOG("WaitOverlapped.Overlapped2 done");

					if (fSuccess || dwErrT == ERROR_MORE_DATA)
						return 1; // OK, клиент подключился

					if ((pPipe->dwState == READING_STATE) && (nDuration > 1000))
					{
						pPipe->nLongServerReading = 0; // Hunged?
					}

					if (dwErrT == ERROR_IO_INCOMPLETE)
					{
						if ((pPipe->dwState == CONNECTING_STATE) && (nDuration > nMaxConnectDuration))
						{
							// 120603. В принципе, это штатная ситуация, если ни одного запроса на
							// подключение не было. Но было зафиксировано странное зависание клиента
							// при попытке подключения к пайпу с mn_MaxCount=1. Пайп вроде был живой
							// и свободный, но тем не менее, на клиенте все время возвращалась
							// ошибка ERROR_PIPE_BUSY.
							//
							//_ASSERTEX((nDuration <= nMaxConnectDuration) && "Unknown problem, waiting for connection");
						}
						#if 0
						// некоторые серверы работают в постоянном чтении, и время ожидания (ввода с клавиатуры например)
						// может вызывать ASSERT
						else if ((pPipe->dwState == READING_STATE) && (nDuration > nMaxConnectDuration))
						{
							// В очень редких случаях - наблюдается "зависание" сервера-клиента.
							// То есть при запуске фара (AltServer) в GUI отображается только черный экран
							_ASSERTEX(FALSE && "Server was hunged at READING_STATE");
						}
						#endif
						else
						{
							continue;
						}
					}

					if (dwErrT == ERROR_IO_INCOMPLETE)
					{
						// просто пересоздать пайп?
					}
					else
					{
						_ASSERTEX(dwErrT == ERROR_IO_INCOMPLETE || dwErrT == ERROR_BROKEN_PIPE);
						DumpError(pPipe, L"PipeServerThread:GetOverlappedResult failed with 0x%08X", FALSE);
					}
				}
				else
				{
					_ASSERTEX(nWait == 0 || nWait == 1);
					DumpError(pPipe, L"PipeServerThread:WaitForMultipleObjects failed with 0x%08X", FALSE);
				}

				// Force client disconnect (error on server side)
				PLOG("WaitOverlapped.Disconnect2");
				_Disconnect(pPipe, true, true);
				PLOG("WaitOverlapped.Disconnect2 done");
				return 2; // ошибка, пробуем еще раз
			}

			return 2; // Terminate!
		}

		BOOL PipeServerRead(PipeInst* pPipe)
		{
			if (mb_Overlapped)
				ResetEvent(pPipe->hEvent);
			pPipe->dwState = READING_STATE;
			
			int nOverRc;
			DWORD cbRead = 0, dwErr = 0, cbWholeSize = 0;
			BOOL fSuccess;
			DWORD In[32];
			T *pIn = NULL;
			SetLastError(0);
			PLOG("PipeServerRead.ReadFile");
			// Send a message to the pipe server and read the response.
			fSuccess = ReadFile(
			               pPipe->hPipeInst, // pipe handle
			               In,               // buffer to receive reply
			               sizeof(In),       // size of read buffer
			               &cbRead,          // bytes read
			               mb_Overlapped ? &pPipe->oOverlap : NULL);
			dwErr = GetLastError();
			PLOG("PipeServerRead.ReadFile done");

			if (mb_Overlapped)
			{
				if (dwErr == ERROR_BROKEN_PIPE)
				{
					PLOG("PipeServerRead.ERROR_BROKEN_PIPE");
					BreakConnection(pPipe);
					return FALSE;
				}
				else if (!fSuccess || !cbRead)
				{
					PLOG("PipeServerRead.WaitOverlapped");
					nOverRc = WaitOverlapped(pPipe, &cbRead);
					if (nOverRc == 1)
					{
						dwErr = GetLastError();
						fSuccess = TRUE;
						PLOG("PipeServerRead.WaitOverlapped done");
					}
					else if (nOverRc == 2)
					{
						PLOG("PipeServerRead.WaitOverlapped break");
						BreakConnection(pPipe);
						return FALSE;
					}
					else
					{
						PLOG("PipeServerRead.WaitOverlapped failed");
						_ASSERTEX(mb_Terminate);
						return FALSE; // terminate
					}
				}
			}
			
			if ((!fSuccess && (dwErr != ERROR_MORE_DATA)) || (cbRead < sizeof(DWORD)) || mb_Terminate)
			{
				// Сервер закрывается?
				//DEBUGSTRPROC(L"!!! ReadFile(pipe) failed - console in close?\n");
				PLOG("PipeServerRead.FALSE1");
				return FALSE;
			}
			
			cbWholeSize = In[0];
			const DWORD minimum = sizeof(T) == sizeof(MSG64) ? sizeof(MSG64) : sizeof(CESERVER_REQ_HDR);
			// Bound allocation before trusting the wire length. This also rejects
			// header-only/truncated input records before dispatch.
			if (cbWholeSize < minimum || cbWholeSize > 64u * 1024 * 1024 || cbWholeSize < cbRead)
				return FALSE;

			PLOG("PipeServerRead.Alloc");
			if (cbWholeSize <= cbRead)
			{
				_ASSERTEX(cbWholeSize >= cbRead);
				pIn = pPipe->AllocRequestBuf(cbRead);
				if (!pIn)
				{
					_ASSERTEX(pIn!=NULL);  // -V547
					return FALSE;
				}
				memmove(pIn, In, cbRead);
				pPipe->cbReadSize = cbRead;
				PLOG("PipeServerRead.memmoved");
			}
			else if (cbWholeSize >= static_cast<DWORD>(std::numeric_limits<int32_t>::max()))
			{
				_ASSERTEX(cbWholeSize < 0x7FFFFFFF);  // -V547
				return FALSE;
			}
			else
			{
				int nAllSize = cbWholeSize;
				pIn = pPipe->AllocRequestBuf(cbWholeSize);
				if (!pIn)
				{
					_ASSERTEX(pIn!=NULL);  // -V547
					return FALSE;
				}
				memmove(pIn, In, cbRead);
				LPBYTE ptrData = ((LPBYTE)pIn)+cbRead;
				nAllSize -= cbRead;

				PLOG("PipeServerRead.ReadingData");

				while (nAllSize>0)
				{
					if (mb_Overlapped)
						ResetEvent(pPipe->hEvent);

					SetLastError(0);
					// Read from the pipe if there is more data in the message.
					fSuccess = ReadFile(
					               pPipe->hPipeInst, // pipe handle
					               ptrData,          // buffer to receive reply
					               nAllSize,         // size of buffer
					               &cbRead,          // number of bytes read
					               mb_Overlapped ? &pPipe->oOverlap : NULL);
					dwErr = GetLastError();

					if (mb_Overlapped)
					{
						if (dwErr == ERROR_BROKEN_PIPE)
						{
							PLOG("PipeServerRead.ERROR_BROKEN_PIPE (more)");
							BreakConnection(pPipe);
							return FALSE;
						}
						else if (!fSuccess || !cbRead)
						{
							nOverRc = WaitOverlapped(pPipe, &cbRead);
							if (nOverRc == 1)
							{
								dwErr = GetLastError();
								fSuccess = TRUE;
							}
							else
							{
								_ASSERTEX(mb_Terminate);
								return FALSE; // terminate
							}
						}
					}
					
					// Exit if an error other than ERROR_MORE_DATA occurs.
					if (!fSuccess && (dwErr != ERROR_MORE_DATA))
						break;

					ptrData += cbRead;
					nAllSize -= cbRead;
				}

				PLOG("PipeServerRead.DataReaded");

				TODO("Mozhet vozniknutj ASSERT, esli konsolj byla zakryta v processe chtenija");

				if (nAllSize>0)
				{
					_ASSERTEX(nAllSize==0);  // -V547
					PLOG("PipeServerRead.FALSE2");
					return FALSE; // удалось считать не все данные
				}

				pPipe->cbReadSize = (DWORD)(ptrData - ((LPBYTE)pIn));
			}

			if (pPipe->cbReadSize != cbWholeSize) return FALSE;
			if (sizeof(T) != sizeof(MSG64)
				&& reinterpret_cast<CESERVER_REQ_HDR*>(pIn)->nVersion != CESERVER_REQ_VER) return FALSE;
			PLOG("PipeServerRead.OK");
			return TRUE;
		}
		
		int PipeServerWrite(PipeInst* pPipe, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite, LPDWORD lpNumberOfBytesWritten, BOOL bDelayed=FALSE)
		{
			if (pPipe->bBreakConnection)
				return 0;

			BOOL fWriteSuccess = FALSE, fSuccess;
			DWORD dwErr = 0;

			if (mb_Overlapped)
				ResetEvent(pPipe->hEvent);
			pPipe->dwState = WRITING_STATE;

			pPipe->bDelayedWrite = bDelayed;
			
			if (!bDelayed || lpBuffer)
			{
				PLOG("PipeServerWrite.Write");
				pPipe->fWriteSuccess = fWriteSuccess = WriteFile(pPipe->hPipeInst, lpBuffer, nNumberOfBytesToWrite, lpNumberOfBytesWritten, mb_Overlapped ? &pPipe->oOverlap : NULL);

				// The write operation is still pending?
				pPipe->dwWriteErr = dwErr = GetLastError();

				PLOG("PipeServerWrite.Write done");
			}
			else // if (bDelayed)
			{
				fWriteSuccess = pPipe->fWriteSuccess;
				dwErr = pPipe->dwWriteErr;
			}
			

			if (mb_Overlapped)
			{
				if (!fWriteSuccess && (dwErr == ERROR_IO_PENDING))
				{
					if (bDelayed)
						return 1;

					PLOG("PipeServerWrite.Wait");
					int nOverRc = WaitOverlapped(pPipe, lpNumberOfBytesWritten);
					PLOG("PipeServerWrite.Wait done");
					if (nOverRc == 0)
						return 0;
					else if (nOverRc == 1)
						return 1;
					else
						return 2;
				}
				else if (fWriteSuccess)
				{
					PLOG("PipeServerWrite.Overlapped");
					fSuccess = GetOverlappedResult(pPipe->hPipeInst, &pPipe->oOverlap, lpNumberOfBytesWritten, FALSE);
					_ASSERTEX(fSuccess);
					PLOG("PipeServerWrite.Overlapped done");
					return fSuccess ? 1 : 2;
				}
				else
				{
					return 2;
				}
			}
			else
			{
				return mb_Terminate ? 0 : fWriteSuccess ? 1 : 2;
			}
		}

		bool WaitForClient(PipeInst* pPipe, BOOL& fConnected, BOOL& bNeedReply)
		{
			bool   lbRc = false;
			DWORD  cbOver = 0;
			HANDLE hWait[2] = {mh_TermEvent,mh_ServerSemaphore};

			// Цикл ожидания подключения
			while (!mb_Terminate)
			{
				PLOG("WaitForClient.WaitMulti");
				// !!! Переносить проверку семафора ПОСЛЕ CreateNamedPipe нельзя, т.к. в этом случае
				//     нити дерутся и клиент не может подцепиться к серверу
				// Дождаться разрешения семафора, или завершения сервера
				pPipe->dwConnWait = WaitForMultipleObjects(2, hWait, FALSE, INFINITE);
				PLOG2("WaitForClient.WaitMulti done",pPipe->dwConnWait);

				if (pPipe->dwConnWait == WAIT_OBJECT_0)
				{
					_ASSERTEX(mb_Terminate==true);
					break; // Сервер закрывается
				}

				if (mb_Overlapped)
				{
					ResetEvent(pPipe->hEvent);
				}

				_ASSERTEX(pPipe->dwConnWait == (WAIT_OBJECT_0+1));
				mp_ActivePipe = pPipe;

				// Пайп уже может быть создан
				if ((pPipe->hPipeInst == NULL) || (pPipe->hPipeInst == INVALID_HANDLE_VALUE))
				{
					// На WinXP наблюдался странный глючок при mn_MaxCount==1, возможно из-за того, что не вызывался DisconnectNamedPipe
					int InstanceCount = RELEASEDEBUGTEST(PIPE_UNLIMITED_INSTANCES,mn_MaxCount);
					DWORD nOutSize = PIPEBUFSIZE;
					DWORD nInSize = PIPEBUFSIZE;

					DWORD dwOpenMode = 
						(mb_InputOnly ? PIPE_ACCESS_INBOUND : PIPE_ACCESS_DUPLEX) |
						(mb_Overlapped ? FILE_FLAG_OVERLAPPED : 0); // overlapped mode?

					DWORD dwPipeMode =
						CE_PIPE_TYPE |         // message type pipe
						CE_PIPE_READMODE |     // message-read mode
						PIPE_WAIT | 0x00000008; // PIPE_REJECT_REMOTE_CLIENTS (Vista+)

					_ASSERTEX(mlp_Sec!=NULL);
					pPipe->dwState = CREATEPIPE_STATE;
					PLOG("WaitForClient.Create");
					pPipe->hPipeInst = CreateNamedPipeW(ms_PipeName, dwOpenMode, dwPipeMode, InstanceCount, nOutSize, nInSize, 0, mlp_Sec);
					pPipe->dwState = PIPECREATED_STATE;
					PLOG("WaitForClient.Create done");

					// Сервер закрывается?
					if (mb_Terminate)
					{
						if (mb_Overlapped)
						{
							PLOG("WaitForClient.ReleaseOnTerminate");
							ReleaseSemaphore(hWait[1], 1, NULL);
						}
						break;
					}

					if ((pPipe->hPipeInst == INVALID_HANDLE_VALUE) || (pPipe->hPipeInst == NULL))
					{
						pPipe->dwConnErr = GetLastError();

						_ASSERTEX(pPipe->hPipeInst != INVALID_HANDLE_VALUE);
						DumpError(pPipe, L"PipeServerThread:ConnectNamedPipe failed with 0x%08X", FALSE);

						//DisplayLastError(L"CreateNamedPipe failed");
						pPipe->hPipeInst = NULL;
						// Wait a little
						Sleep(10);
						// Разрешить этой или другой нити создать серверный пайп
						PLOG("WaitForClient.ReleaseOnError");
						ReleaseSemaphore(hWait[1], 1, NULL);
						// Try again
						continue;
					}

					DEBUGTEST(pPipe->nCreateCount++);
				}

				// Чтобы ConEmuC знал, что серверный пайп готов
				if (!mb_ReadyCalled && mfn_PipeServerReady)
				{
					mb_ReadyCalled = TRUE;
					mfn_PipeServerReady(pPipe, m_lParam);
				}

				_ASSERTEX(pPipe->hPipeInst && (pPipe->hPipeInst!=INVALID_HANDLE_VALUE));
				// Wait for the client to connect; if it succeeds,
				// the function returns a nonzero value. If the function
				// returns zero, GetLastError returns ERROR_PIPE_CONNECTED.
				SetLastError(0);
				pPipe->dwState = CONNECTING_STATE;
				PLOG("WaitForClient.Connect");
				fConnected = ConnectNamedPipe(pPipe->hPipeInst, mb_Overlapped ? &pPipe->oOverlap : NULL);
				pPipe->dwConnErr = GetLastError();
				PLOG("WaitForClient.Connect done");
				//? TRUE : ((dwErr = GetLastError()) == ERROR_PIPE_CONNECTED);
				// сразу разрешить другой нити принять вызов
				if (!mb_Overlapped)
				{
					PLOG("WaitForClient.ReleaseOnConnect1");
					ReleaseSemaphore(hWait[1], 1, NULL);
				}

				//bool lbFail = false;
				if (mb_Overlapped)
				{
					// Overlapped ConnectNamedPipe should return zero.
					if (fConnected)
					{
						DumpError(pPipe, L"PipeServerThread:ConnectNamedPipe failed with 0x%08X", FALSE);
						PLOG("WaitForClient.Sleeping-100");
						Sleep(100);
						PLOG("WaitForClient.ReleaseOnConnect2");
						ReleaseSemaphore(hWait[1], 1, NULL);
						continue;
					}
					bNeedReply = (pPipe->dwConnErr == ERROR_PIPE_CONNECTED);
				}
				else
				{
					if (!fConnected && pPipe->dwConnErr == ERROR_PIPE_CONNECTED)
						fConnected = TRUE;
					bNeedReply = fConnected;
				}

				// Сервер закрывается?
				if (mb_Terminate)
				{
					if (mb_Overlapped)
					{
						PLOG("WaitForClient.ReleaseOnTerminate2");
						ReleaseSemaphore(hWait[1], 1, NULL);
					}
					break;
				}

				// Обработка подключения
				if (mb_Overlapped)
				{
					// Wait for overlapped connection
					_ASSERTEX(pPipe->hPipeInst && (pPipe->hPipeInst!=INVALID_HANDLE_VALUE));
					int nOverRc = (pPipe->dwConnErr == ERROR_PIPE_CONNECTED) ? 1 : WaitOverlapped(pPipe, &cbOver);
					
					DWORD t1 = _GetTime();
					PLOG("WaitForClient.ReleaseOnConnect3");
					ReleaseSemaphore(hWait[1], 1, NULL);
					DWORD t2 = _GetTime();

					if (nOverRc == 0)
					{
						_ASSERTEX(mb_Terminate==true);
						pPipe->nTerminateDelay2 = (t2 - t1);
						pPipe->nTerminateDelay += pPipe->nTerminateDelay2;
						break; // Завершение сервера
					}
					else if (nOverRc == 1)
					{
						// OK, клиент подключился
						lbRc = true;
						pPipe->dwState = CONNECTED_STATE;
						bNeedReply = TRUE;
						break;
					}
					else
					{
						// Проверить, какие коды ошибок? Может быть нужно CloseHandle дернуть?
						// 120603 - см. комментарий на ERROR_IO_INCOMPLETE
						// Если это (nOverRc==2 && mb_Terminate) - то _Disconnect будет вызван в StopPipeServer
						_ASSERTEX(nOverRc==1
							|| (nOverRc==2 && pPipe->hPipeInst==NULL && pPipe->dwConnErr==ERROR_IO_PENDING && pPipe->dwState==CLOSED_STATE)
							|| (mb_Terminate && nOverRc==2 && pPipe->hPipeInst && pPipe->dwConnErr==ERROR_IO_PENDING && pPipe->dwState==CONNECTING_STATE));
						continue; // ошибка, пробуем еще раз
					}
				} // end - mb_Overlapped
				else
				{
					if (fConnected)
					{
						// OK, Выйти из цикла ожидания подключения, перейти к чтению
						lbRc = true;
						pPipe->dwState = CONNECTED_STATE;
						break;
					}
					else
					{
						// В каких случаях это может возникнуть? Нужно ли закрывать хэндл, или можно переподключиться?
						_ASSERTEX(fConnected!=FALSE);

						_Disconnect(pPipe, true, true);
					}
				} // end - !mb_Overlapped
			} // Цикл ожидания подключения while (!mb_Terminate)

			return lbRc;
		}
		
		BOOL WriteDummyAnswer(PipeInst* pPipe)
		{
			if (pPipe->bBreakConnection)
				return FALSE;

			BOOL fWriteSuccess = FALSE;
			DWORD cbSize = mn_DummyAnswerSize, cbWritten;
			_ASSERTEX(cbSize >= sizeof(DWORD) && cbSize <= sizeof(T));
			BYTE *ptrNil = (BYTE*)calloc(cbSize, 1);
			if (ptrNil)
			{
				fWriteSuccess = PipeServerWrite(pPipe, ptrNil, cbSize, &cbWritten);
				free(ptrNil);
			}
			return fWriteSuccess;
		}

		#ifdef _DEBUG
		void DebugDumpCommand(PipeInst* pPipe, CESERVER_REQ_HDR* ptrRequest)
		{
			if (ptrRequest->nVersion == CESERVER_REQ_VER)
			{
				if (ptrRequest->nCmd == CECMD_CONSOLEDATA)
					return; // Skip this event
				swprintf_c(pPipe->szDbgInfo,
					L"<< CmdRecv: PID=%5u  TID=%5u  Cmd=%3u  DataSize=%u  FromPID=%u  FromTID=%u\n", GetCurrentProcessId(), GetCurrentThreadId(), ptrRequest->nCmd,
					(UINT)(ptrRequest->cbSize - sizeof(*ptrRequest)),
					ptrRequest->nSrcPID, ptrRequest->nSrcThreadId);
				DEBUGSTRPPCMD(pPipe->szDbgInfo);
			}
			else
			{
				static bool bWarned = false; if (!bWarned)
				{
					bWarned = TRUE; _ASSERTE(FALSE && "Invalid packet version");
				}
			}
		}
		#endif

		// Processing methods
		DWORD PipeServerThread(PipeInst* pPipe)
		{
			_ASSERTEX(this && pPipe && pPipe->pServer==this);
			BOOL fConnected = FALSE;
			BOOL fNewCheckStart = (mn_PipesToCreate > 0);
			//DWORD dwErr = 0;
			DWORD dwTID = GetCurrentThreadId();
			// The main loop creates an instance of the named pipe and
			// then waits for a client to connect to it. When the client
			// connects, a thread is created to handle communications
			// with that client, and the loop is repeated.

			//HANDLE hWait[2]; // без Overlapped - используется только при ожидании свободного сервера
			//hWait[0] = mh_TermEvent;
			//hWait[1] = mh_ServerSemaphore;

			// debugging purposes
			pPipe->dwState = STARTED_STATE;
			pPipe->nTerminateDelay = pPipe->nTerminateDelay1 = pPipe->nTerminateDelay2 = pPipe->nTerminateDelay3 = 0;
			
			_ASSERTEX(!mb_Overlapped || pPipe->hEvent != NULL);
			_ASSERTEX(pPipe->hPipeInst == NULL);

			// Пока не затребовано завершение сервера
			while (!mb_Terminate)
			{
				int   fWriteSuccess = 0;
				BOOL  bNeedReply = FALSE;
				DWORD cbWritten = 0;

				// Цикл ожидания подключения
				if (!WaitForClient(pPipe, fConnected, bNeedReply))
				{
					_ASSERTEX(mb_Terminate==true);
				}
				else
				{
					DEBUGTEST(pPipe->nConnectCount++);
				}
				
				// Сервер закрывается?
				if (mb_Terminate)
				{
					DWORD t1 = _GetTime();
					// Всегда чего-нибудь ответить в пайп, а то ошибка (Pipe was closed) у клиента возникает
					if (pPipe->hPipeInst)
					{
						if (bNeedReply)
						{
							fWriteSuccess = WriteDummyAnswer(pPipe);
							
							// Function does not return until the client process has read all the data.
							//FlushFileBuffers(pPipe->hPipeInst);
						}

						_Disconnect(pPipe, false, true);
					}
					DWORD t2 = _GetTime();
					pPipe->nTerminateDelay3 = (t2 - t1);
					pPipe->nTerminateDelay += pPipe->nTerminateDelay3;
					return 0;
				}
				
				// Если еще не все пайпы открыты/потоки запущены
				if (fNewCheckStart && (mn_PipesToCreate > 0))
				{
					mn_PipesToCreate--;
					fNewCheckStart = FALSE;
					for (int j = 0; j < mn_MaxCount; j++)
					{
						if (m_Pipes[j].hThread == NULL)
						{
							// Стартуем новый instance
							bool bStartNewInstance = StartPipeInstance(&(m_Pipes[j]));
							_ASSERTEX(bStartNewInstance==true);
							UNREFERENCED_PARAMETER(bStartNewInstance);
							break;
						}
					}
				}
				
				BOOL lbFirstRead = TRUE;
				BOOL lbAllowClient = TRUE;
				

				if (lbAllowClient)
				{
					// *** Основной рабочий цикл ***
					// Чтение данных запроса из пайпа и запись результата
					while (PipeServerRead(pPipe))
					{
						#ifdef _DEBUG
						pPipe->nCallCount++;
						GetLocalTime(&pPipe->stLastCall);
						#endif

						if (lbFirstRead)
						{
							lbFirstRead = FALSE;
							// Impersonation authenticates the last message read, so it
							// must happen after ReadFile, but before any command dispatch.
							if (!IsLocalPipeClientTrusted(pPipe->hPipeInst)
								|| (mfn_PipeServerConnected && !mfn_PipeServerConnected(pPipe, m_lParam)))
							{
								BreakConnection(pPipe);
								break;
							}
						}
						
						pPipe->bBreakConnection = false;

						#ifdef _DEBUG
						{
							if (sizeof(*pPipe->ptrRequest) == sizeof(CESERVER_REQ))
							{
								DebugDumpCommand(pPipe, (CESERVER_REQ_HDR*)pPipe->ptrRequest);
							}
							else if (sizeof(*pPipe->ptrRequest) == sizeof(MSG64))
							{
								// Nothing to do yet
								int iDbg = 0;
							}
							else
							{
								static bool bWarned = false; if (!bWarned)
								{
									bWarned = TRUE; _ASSERTE(FALSE && "Unknown size of packet");
								}
							}
						}
						#endif

						if (mfn_PipeServerCommand(pPipe, pPipe->ptrRequest, /*OUT&*/pPipe->ptrReply,
									/*OUT&*/pPipe->cbReplySize, /*OUT&*/pPipe->cbMaxReplySize, m_lParam))
						{
							#ifdef _DEBUG
							{
								CESERVER_REQ_HDR* ph = (CESERVER_REQ_HDR*)pPipe->ptrRequest;
								if ((ph->cbSize >= sizeof(CESERVER_REQ_HDR))
									&& (ph->nVersion == CESERVER_REQ_VER)
									&& (ph->bAsync))
								{
									_ASSERTEX(pPipe->bBreakConnection && "Break connection on async!");
								}
							}
							#endif
							// Если данные вернули (могли сами в пайп ответить)
							if (!mb_InputOnly && pPipe->ptrReply && pPipe->cbReplySize)
							{
								// Если успешно - запись в пайп результата
								_ASSERTEX(pPipe->cbReplySize>=(DWORD)mn_DummyAnswerSize);
								fWriteSuccess = PipeServerWrite(pPipe, pPipe->ptrReply, pPipe->cbReplySize, &cbWritten);
							}
						}
						else if (!mb_InputOnly)
						{
							// Всегда чего-нибудь ответить в пайп, а то ошибка (Pipe was closed) у клиента возникает
							fWriteSuccess = WriteDummyAnswer(pPipe);
						}

						#ifdef _DEBUG
						if (fWriteSuccess)
							pPipe->nAnswCount++;
						GetLocalTime(&pPipe->stLastEndCall);
						#endif

						if (pPipe->bBreakConnection)
							break; // Переоткрыть пайп (ошибка протокола)
					
						if (mb_Terminate || !mb_LoopCommands)
							break;

					} // while (PipeServerRead(pPipe))
				}
				
				if (!pPipe->bBreakConnection)
				{
					// Fin
					if (lbFirstRead)
					{
						// Всегда чего-нибудь ответить в пайп, а то ошибка (Pipe was closed) у клиента возникает
						fWriteSuccess = WriteDummyAnswer(pPipe);
					}
					
					if (!mb_Terminate)
					{
						// Function does not return until the client process has read all the data.
						PLOG("WaitForClient.FlushFileBuffers");
						FlushFileBuffers(pPipe->hPipeInst);
						PLOG("WaitForClient.FlushFileBuffers done");
					}
				}
				
				if (mfn_PipeServerDisconnected)
					mfn_PipeServerDisconnected(pPipe, m_lParam);
				
				if (!mb_Terminate)
				{
					// FlushFileBuffers уже вызван, т.е. клиент данные не потеряет
					// А вот вызов CloseHandle перед DisconnectNamedPipe может привести к ошибке 231
					// (All pipe instances are busy) при следующей попытке CreateNamedPipe
					_Disconnect(pPipe, true, false);
				}

				TODO("Vozmogno, v kakih-to sluchajah nujno zakryt' pipe handle");

				// Перейти к открытию нового instance пайпа

				UNREFERENCED_PARAMETER(fWriteSuccess);
			} // Пока не затребовано завершение сервера while (!mb_Terminate)

			if (pPipe->hPipeInst && (pPipe->hPipeInst != INVALID_HANDLE_VALUE))
			{
				_Disconnect(pPipe, false, true, false);
			}

			UNREFERENCED_PARAMETER(dwTID);
			return 0;
		};
		bool StartPipeInstance(PipeInst* pPipe)
		{
			PLOG("StartPipeInstance");
			if (mb_Overlapped)
			{
				if (pPipe->hEvent == NULL)
				{
					pPipe->hEvent = CreateEvent(
						NULL,    // default security attribute
						TRUE,    // manual-reset(!) event
						TRUE,    // initial state = signaled
						NULL);   // unnamed event object

					if (pPipe->hEvent == NULL)
					{
						DumpError(pPipe, L"StartPipeInstance:CreateEvent failed with 0x%08X");
						return false; //-(200+j);
					}
				}

				pPipe->oOverlap.hEvent = pPipe->hEvent;
			}

			_ASSERTEX(pPipe->hThread == NULL);
			//_ASSERTEX(pPipe->hThreadEnd == NULL);

			_ASSERTEX(((DWORD_PTR)(pPipe - m_Pipes) < (DWORD_PTR)mn_MaxCount) && (mn_MaxCount > 0));
			pPipe->nIndex = LODWORD(pPipe - m_Pipes);
			pPipe->nMagic = PipeInstMagic;

			pPipe->pServer = this;
			pPipe->nThreadId = 0;
			pPipe->dwState = STARTING_STATE;
			//pPipe->hThreadEnd = CreateEvent(NULL, TRUE, FALSE, NULL);
			PLOG("StartPipeInstance.Thread");
			pPipe->nCreateBegin = GetTickCount(); pPipe->nCreateEnd = 0;
			char szPipeName[40] = ""; WideCharToMultiByte(CP_ACP, 0, PointToName(ms_PipeName), -1, szPipeName, countof(szPipeName)-1, NULL, NULL);
			pPipe->hThread = apiCreateThread(_PipeServerThread, pPipe, &pPipe->nThreadId, szPipeName[0] ? szPipeName : "PipeServerThread");
			pPipe->nCreateError = GetLastError();
			pPipe->nCreateEnd = GetTickCount();
			DWORD nThreadCreationTime = pPipe->nCreateEnd - pPipe->nCreateBegin;
			UNREFERENCED_PARAMETER(nThreadCreationTime);
			if (pPipe->hThread == NULL)
			{
				_ASSERTEX(pPipe->hThread!=NULL && "Pipe thread creation failed");
				DumpError(pPipe, L"StartPipeInstance:CreateThread failed, code=0x%08X");
				return false; // Не удалось создать серверные потоки
			}
			if (mn_Priority)
			{
				::SetThreadPriority(pPipe->hThread, mn_Priority);
			}
			#ifdef _DEBUG
			DWORD nRet = (DWORD)-1; BOOL bRetGet = (BOOL)-1;
			if (WaitForSingleObject(pPipe->hThread, 0) == WAIT_OBJECT_0)
			{
				bRetGet = GetExitCodeThread(pPipe->hThread, &nRet);
				_ASSERTEX((pPipe->dwState != STARTING_STATE) && "Thread was terminated outside?");
			}
			#endif

			PLOG("StartPipeInstance.Done");
			return true;
		};
		static DWORD WINAPI _PipeServerThread(LPVOID lpvParam)
		{
			DWORD nResult = 101;
			PipeInst* pPipe = (PipeInst*)lpvParam;
			_ASSERTEX(pPipe!=NULL && pPipe->pServer!=NULL && pPipe->nMagic==PipeInstMagic);

			pPipe->nStartedTick = GetTickCount();
			pPipe->dwState = STARTED_STATE;
			PLOG("_PipeServerThread.Started");

			if (!pPipe->pServer->mb_Terminate)
			{
				// Working method
				nResult = pPipe->pServer->PipeServerThread(pPipe);
			}

			pPipe->nTerminatedTick = GetTickCount();
			pPipe->dwState = TERMINATED_STATE;
			PLOG("_PipeServerThread.Finished");

			#if 0
			// When StopPipeServer is called from DllMain
			// we possibly get in deadlock on "normal" thread termination
			// LdrShutdownThread in THIS thread fails to enter into critical section,
			// because it already locked by MAIN thread (DllMain)
			// So, StopPipeServer will fail on wait for pipe threads
			if (pPipe->pServer->mb_StopFromDllMain)
			{
				//WaitForSingleObject(pPipe->hThread, INFINITE); -- don't enter infinite lock
				Sleep(RELEASEDEBUGTEST(1000,5000)); // just wait a little, main thread may be in time to terminate this thread
			}
			#endif
			
			// When "C:\Program Files (x86)\F-Secure\apps\ComputerSecurity\HIPS\fshook64.dll"
			// is loaded, possible TerminateThread locks together
			//// Force thread termination cause of unknown attached dll's.
			//if (pPipe->pServer->mb_Terminate && !pPipe->bSkipTerminate)
			//{
			//	pPipe->dwState = TERMINATE_CALL_STATE;
			//	apiTerminateThread(GetCurrentThread(), 100);
			//}
			return nResult;
		};
		static void TerminatePipeThread(HANDLE& hThread, const PipeInst& pipe, bool& rbForceTerminated)
		{
			if (!hThread)
				return;

			#ifdef _DEBUG
			LONG lSleeps = 0;
			while (gnInMyAssertTrap > 0)
			{
				lSleeps++;
				MessageBeep(MB_ICONSTOP);
				Sleep(10000);
			}
			// Due to possible deadlocks when finalizing from DllMain
			// the TerminateThread may be a sole way to terminate properly
			_ASSERTEX((pipe.dwState == TERMINATED_STATE) && "TerminatePipeThread");
			#endif

			// Last chance
			DWORD nWait = WaitForSingleObject(hThread, 150);
			switch (nWait)
			{
			case WAIT_OBJECT_0:
				// OK, thread managed to terminate properly
				break;
			default:
				rbForceTerminated = true;
				apiTerminateThread(hThread, 100);
			}

			SafeCloseHandle(hThread);
		}
	//public:
	//	CPipeServer()
	//	{
	//		mb_Initialized = FALSE;
	//		ms_PipeName[0] = 0;
	//		memset(m_Pipes, 0, sizeof(m_Pipes));
	//		mp_ActivePipe = NULL;
	//		mfn_PipeServerCommand = NULL;
	//		mfn_PipeServerReady = NULL;
	//		mb_ReadyCalled = FALSE;
	//		m_lParam = 0;
	//		mh_TermEvent = NULL;
	//		mb_Terminate = FALSE;
	//		mh_ServerSemaphore = NULL;
	//	};
	//	~CPipeServer()
	//	{
	//		StopPipeServer();
	//	};
	public:
		void SetPriority(int nPriority)
		{
			// Звать ПЕРЕД StartPipeServer
			_ASSERTEX(mb_Initialized==FALSE);
			mn_Priority = nPriority;
		};

		void SetInputOnly(bool bInputOnly)
		{
			// Звать ПЕРЕД StartPipeServer
			_ASSERTEX(mb_Initialized==FALSE);
			mb_InputOnly = bInputOnly;
		};

		void SetOverlapped(bool bOverlapped)
		{
			// Звать ПЕРЕД StartPipeServer
			_ASSERTEX(mb_Initialized==FALSE);
			mb_Overlapped = bOverlapped;
		};

		void SetLoopCommands(bool bLoopCommands)
		{
			// Звать ПЕРЕД StartPipeServer
			_ASSERTEX(mb_Initialized==FALSE);
			mb_LoopCommands = bLoopCommands;
		};

		void SetDummyAnswerSize(int anDummyAnswerSize)
		{
			// Звать ПЕРЕД StartPipeServer
			mn_DummyAnswerSize = anDummyAnswerSize;
		};

		void SetMaxCount(int nMaxCount)
		{
			// Звать ПЕРЕД StartPipeServer
			if (mb_Initialized || m_Pipes || mn_MaxCount > 0)
			{
				_ASSERTEX(mb_Initialized==FALSE && !m_Pipes && mn_MaxCount==0);
				return;
			}
			mn_MaxCount = nMaxCount;
		};
		
		bool StartPipeServer(
			    bool abForceTerminate,
			    LPCWSTR asPipeName,
			    LPARAM alParam,
			    LPSECURITY_ATTRIBUTES alpSec /*= LocalSecurity()*/, // MUST! be alive for life cycle
			    PipeServerCommand_t apfnPipeServerCommand,
			    PipeServerFree_t apfnPipeServerFree = NULL,
			    PipeServerConnected_t apfnPipeServerConnected = NULL,
			    PipeServerConnected_t apfnPipeServerDisconnected = NULL,
			    PipeServerReady_t apfnPipeServerReady = NULL
		    )
		{
			if (!alpSec || !alpSec->lpSecurityDescriptor || !apfnPipeServerCommand)
				return false; // Never fall back to an unintended default DACL.
			if (mb_Initialized)
			{
				_ASSERTEX(mb_Initialized==FALSE);  // -V547
				return false;
			}
			
			mb_Initialized = false;
			mb_UseForceTerminate = abForceTerminate;
			//mb_Overlapped = abOverlapped;
			//mb_LoopCommands = abLoopCommands;
			
			if (mn_MaxCount < 1)
				mn_MaxCount = 1;
			if (mn_DummyAnswerSize <= 0 || mn_DummyAnswerSize > (int)sizeof(T))
				mn_DummyAnswerSize = sizeof(T);
				
			m_Pipes = (PipeInst*)calloc(mn_MaxCount,sizeof(*m_Pipes));
			if (m_Pipes == NULL)
			{
				_ASSERTEX(m_Pipes!=NULL);  // -V547
				return false;
			}
			//memset(m_Pipes, 0, sizeof(m_Pipes));

			mp_ActivePipe = NULL;

			mb_ReadyCalled = FALSE;
			mh_TermEvent = CreateEvent(NULL,TRUE/*MANUAL - используется в нескольких нитях!*/,FALSE,NULL); ResetEvent(mh_TermEvent);
			mb_Terminate = false;
			mh_ServerSemaphore = CreateSemaphore(NULL, 1, 1, NULL);
			
			wcscpy_c(ms_PipeName, asPipeName);
			m_lParam = alParam;
			mlp_Sec = alpSec;
			
			mfn_PipeServerCommand = apfnPipeServerCommand;
			mfn_PipeServerReady = apfnPipeServerReady;
			mfn_PipeServerConnected = apfnPipeServerConnected;
			mfn_PipeServerDisconnected = apfnPipeServerDisconnected;
			mfn_PipeServerFree = apfnPipeServerFree;
			_ASSERTEX(mb_ReadyCalled==FALSE);
			
			// Необходимо указать как минимум эту функцию
			if (!apfnPipeServerCommand)
			{
				//_ASSERTEX(apfnPipeServerCommand!=NULL);
				DumpError(&(m_Pipes[0]), L"StartPipeServer:apfnPipeServerCommand is NULL");
				return false;
			}

			if (!StartPipeInstance(&(m_Pipes[0])))
			{
				// DumpError уже позвали
				return false;
			}
			mn_PipesToCreate = (mn_MaxCount > 1) ? (mn_MaxCount - 1) : 0;

			mb_Initialized = TRUE;
			
			//#if 0
			//// Overlapped, доделать
			//// Создаем Instances
			//int iRc = InitPipes();
			//return (iRc == 0);
			//#endif
			
			return true;
		};

		void StopPipeServer(bool bFromDllMain, bool& rbForceTerminated)
		{
			if (!mb_Initialized)
				return;

			DWORD nWait;

			PLOG3(-1,"StopPipeServer",0);
			
			mb_StopFromDllMain = bFromDllMain;
			mb_Terminate = true;
			if (mh_TermEvent)
				SetEvent(mh_TermEvent);

			DWORD nTimeout = RELEASEDEBUGTEST(250,5000);
			DEBUGTEST(DWORD nWarnTimeout = 500);
			DWORD nStartTick = GetTickCount(), nEndTick = 0;
			int nLeft = 0, nWaitLeft = 0, nLoops = 0;
			const DWORD nSingleThreadWait = 5;

			PLOG3(-1,"WaitFor TERMINATED_STATE",0);

			// Waiting for TERMINATED_STATE for all threads
			do {
				nLoops++;
				nLeft = nWaitLeft = 0;
				for (int i = 0; i < mn_MaxCount; i++)
				{
					if (m_Pipes[i].hThread)
					{
						if (m_Pipes[i].dwState == STARTING_STATE)
						{
							// CreateThread was called, but thread routine was not entered yet.
							PLOG3(i,"TerminateThread/STARTING_STATE",0);
							DWORD nRet = (DWORD)-1; BOOL bRetGet = (BOOL)-1;
							if (WaitForSingleObject(m_Pipes[i].hThread, nSingleThreadWait) == WAIT_OBJECT_0)
							{
								bRetGet = GetExitCodeThread(m_Pipes[i].hThread, &nRet);
								_ASSERTEX((m_Pipes[i].dwState != STARTING_STATE) && "Thread was terminated outside?");
								m_Pipes[i].dwState = THREAD_FINISHED_STATE;
							}
							else
							{
								nLeft++;
								nWaitLeft++;
							}
							PLOG3(i,"TerminateThread/STARTING_STATE done",0);
						}
						else if ((m_Pipes[i].dwState == TERMINATED_STATE) && mb_StopFromDllMain)
						{
							// Don't wait for "normal" termination.
							// Its deadlock when StopPipeServer is called from DllMain.
							PLOG3(i,"TerminateThread/TERMINATED_STATE",TERMINATED_STATE);
							nLeft++;
						}
						else if (m_Pipes[i].dwState != THREAD_FINISHED_STATE)
						{
							nWait = WaitForSingleObject(m_Pipes[i].hThread, nSingleThreadWait);
							if (nWait == WAIT_OBJECT_0)
							{
								m_Pipes[i].dwState = THREAD_FINISHED_STATE;
							}
							else
							{
								nLeft++;
								nWaitLeft++;
							}
						}
					}
				}
				#ifdef _DEBUG
				if (nWarnTimeout && ((GetTickCount() - nStartTick) >= nWarnTimeout))
				{
					nWarnTimeout = 0;
					for (int i = 0; i < mn_MaxCount; i++)
					{
						if (m_Pipes[i].hThread)
							SuspendThread(m_Pipes[i].hThread);  // -V720
					}
					_ASSERTE(FALSE && "StopPipeServer takes more than 500ms (debug mode), normal termination fails");
					for (int i = 0; i < mn_MaxCount; i++)
					{
						if (m_Pipes[i].hThread)
							ResumeThread(m_Pipes[i].hThread);
					}
				}
				#endif
			} while ((nWaitLeft > 0) && (((nEndTick = GetTickCount()) - nStartTick) < nTimeout));

			// Non terminated threads exists?
			if (nLeft > 0)
			{
				PLOG3(-1,"Timeout TERMINATED_STATE",nLeft);

				// Unfortunately, waiting for server thread failed with timeout. Forcing termination
				for (int i = 0; i < mn_MaxCount; i++)
				{
					if (m_Pipes[i].hThread && (m_Pipes[i].dwState != THREAD_FINISHED_STATE))
					{
						DWORD nTimeout = (m_Pipes[i].dwState == TERMINATED_STATE) ? 250 : mb_StopFromDllMain ? 0 : 1;
						nWait = WaitForSingleObject(m_Pipes[i].hThread, nTimeout);
						if (nWait != WAIT_OBJECT_0)
						{
							PLOG3(i,"TerminateThread/Timeout",m_Pipes[i].dwState);

							#ifdef _DEBUG
							// When StopPipeServer is called from DllMain
							// we possibly get in deadlock on "normal" thread termination
							// LdrShutdownThread in PIPE thread fails to enter into critical section,
							// because it already locked by THIS thread (DllMain)
							// So, StopPipeServer will fail on wait for pipe threads
							if (!mb_StopFromDllMain)
							{
								SuspendThread(m_Pipes[i].hThread);  // -V720
								_ASSERTEX(nWait == WAIT_OBJECT_0 && ("Waiting for pipe thread termination failed"));
								ResumeThread(m_Pipes[i].hThread);
							}
							#endif

							// When "C:\Program Files (x86)\F-Secure\apps\ComputerSecurity\HIPS\fshook64.dll"
							// is loaded, possible TerminateThread locks together
							_ASSERTE(mb_UseForceTerminate && "Do not use TerminateThread?");
							TerminatePipeThread(m_Pipes[i].hThread, m_Pipes[i], rbForceTerminated);
							m_Pipes[i].dwState = THREAD_FINISHED_STATE;
							PLOG3(i,"TerminateThread/Timeout done",m_Pipes[i].dwState);
						}
					}
				}
			}

			PLOG3(-1,"FinalizingPipes",0);

			// Finalizing pipes...
			for (int i = 0; i < mn_MaxCount; i++)
			{
				// Done. Closing pipe instances...
				if (m_Pipes[i].hPipeInst && (m_Pipes[i].hPipeInst != INVALID_HANDLE_VALUE))
				{
					_Disconnect(&(m_Pipes[i]), true, true, false);
				}
				m_Pipes[i].hPipeInst = NULL;
				
				// And handles
				if (m_Pipes[i].hEvent)
				{
					SafeCloseHandle(m_Pipes[i].hEvent);
				}
				
				if (m_Pipes[i].hThread)
				{
					PLOG3(i,"CloseThreadHandle",m_Pipes[i].dwState);
					SafeCloseHandle(m_Pipes[i].hThread);
					PLOG3(i,"CloseThreadHandle done",m_Pipes[i].dwState);
				}

				// Release buffers memory
				if (m_Pipes[i].ptrRequest)
				{
					free(m_Pipes[i].ptrRequest);
					m_Pipes[i].ptrRequest = NULL;
				}
				
				// Callbacks
				if (m_Pipes[i].ptrReply && mfn_PipeServerFree)
				{
					mfn_PipeServerFree(m_Pipes[i].ptrReply, m_lParam);
					m_Pipes[i].ptrReply = NULL;
				}
			}

			PLOG3(-1,"FinalizingPipes done",0);
			
			// Final release
			if (mh_ServerSemaphore)
			{
				SafeCloseHandle(mh_ServerSemaphore);
			}
			if (mh_TermEvent)
			{
				SafeCloseHandle(mh_TermEvent);
			}

			free(m_Pipes);
			m_Pipes = NULL;

			mb_Initialized = FALSE;

			PLOG3(-1,"StopPipeServer done",0);
		};

		bool DelayedWrite(LPVOID pInstance, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite)
		{
			PipeInst* pPipe = (PipeInst*)pInstance;
			DWORD dwWritten = 0;

			_ASSERTEX((int)nNumberOfBytesToWrite>=mn_DummyAnswerSize);
			int iRc = PipeServerWrite(pPipe, lpBuffer, nNumberOfBytesToWrite, &dwWritten, TRUE);

			return (iRc == 1);
		};

		void BreakConnection(LPVOID pInstance)
		{
			PipeInst* pPipe = (PipeInst*)pInstance;
			PLOG("BreakConnection");
			pPipe->bBreakConnection = TRUE;
		};

		HANDLE GetPipeHandle(LPVOID pInstance)
		{
			return pInstance ? ((PipeInst*)pInstance)->hPipeInst : NULL;
		};

		bool IsPipeThread(DWORD TID)
		{
			if (!mb_Initialized)
				return false;

			for (int i = 0; i < mn_MaxCount; i++)
			{
				if (m_Pipes[i].hThread && m_Pipes[i].nThreadId == TID)
					return true;
			}

			return false;
		};
};

#undef PLOG
#undef PLOG2
#undef PLOG3
