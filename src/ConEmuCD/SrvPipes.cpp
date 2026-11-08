
/*
Copyright (c) 2009-present Maximus5
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

#include "ConsoleMain.h"
#include "ConEmuSrv.h"
#include "InputLogger.h"
#include "SrvCommands.h"
#include "Queue.h"

#define DEBUGSTRINPUTPIPE(s) //DEBUGSTR(s) // ConEmuC: Received key... / ConEmuC: Received input

// Forwards
BOOL WINAPI InputServerCommand(LPVOID pInst, MSG64* pCmd, MSG64* &ppReply, DWORD &pcbReplySize, DWORD &pcbMaxReplySize, LPARAM lParam);
BOOL WINAPI CmdServerCommand(LPVOID pInst, CESERVER_REQ* pIn, CESERVER_REQ* &ppReply, DWORD &pcbReplySize, DWORD &pcbMaxReplySize, LPARAM lParam);
void WINAPI CmdServerFree(CESERVER_REQ* pReply, LPARAM lParam);
BOOL WINAPI DataServerCommand(LPVOID pInst, CESERVER_REQ* pIn, CESERVER_REQ* &ppReply, DWORD &pcbReplySize, DWORD &pcbMaxReplySize, LPARAM lParam);
void WINAPI DataServerFree(CESERVER_REQ* pReply, LPARAM lParam);

// Helpers
bool InputServerStart()
{
	if (!gpSrv || !*gpSrv->szInputname)
	{
		_ASSERTE(gpSrv!=NULL && *gpSrv->szInputname);
		return false;
	}

	gpSrv->InputServer.SetOverlapped(true);
	gpSrv->InputServer.SetLoopCommands(true);
	gpSrv->InputServer.SetPriority(THREAD_PRIORITY_ABOVE_NORMAL);
	gpSrv->InputServer.SetInputOnly(true);
	//gpSrv->InputServer.SetDummyAnswerSize(sizeof(CESERVER_REQ_HDR));

	if (!gpSrv->InputServer.StartPipeServer(false, gpSrv->szInputname, NULL, LocalSecurity(), InputServerCommand))
		return false;

	return true;
}

bool CmdServerStart()
{
	if (!gpSrv || !*gpSrv->szPipename)
	{
		_ASSERTE(gpSrv!=NULL && *gpSrv->szPipename);
		return false;
	}

	gpSrv->CmdServer.SetMaxCount(3);
	gpSrv->CmdServer.SetOverlapped(true);
	gpSrv->CmdServer.SetLoopCommands(false);
	//gpSrv->CmdServer.SetPriority(THREAD_PRIORITY_ABOVE_NORMAL);
	gpSrv->CmdServer.SetDummyAnswerSize(sizeof(CESERVER_REQ_HDR));

	if (!gpSrv->CmdServer.StartPipeServer(false, gpSrv->szPipename, 0, LocalSecurity(), CmdServerCommand, CmdServerFree))
		return false;

	return true;
}

bool DataServerStart()
{
	if (!gpSrv || !*gpSrv->szGetDataPipe)
	{
		_ASSERTE(gpSrv!=NULL && *gpSrv->szGetDataPipe);
		return false;
	}

	gpSrv->DataServer.SetOverlapped(true);
	gpSrv->DataServer.SetLoopCommands(true);
	gpSrv->DataServer.SetPriority(THREAD_PRIORITY_ABOVE_NORMAL);
	gpSrv->DataServer.SetDummyAnswerSize(sizeof(CESERVER_REQ_HDR));

	if (!gpSrv->DataServer.StartPipeServer(false, gpSrv->szGetDataPipe, NULL, LocalSecurity(), DataServerCommand, DataServerFree))
		return false;

	return true;
}



// Bodies
BOOL WINAPI InputServerCommand(LPVOID pInst, MSG64* pCmd, MSG64* &ppReply, DWORD &pcbReplySize, DWORD &pcbMaxReplySize, LPARAM lParam)
{
	if (!pCmd || !pCmd->nCount || pCmd->cbSize < sizeof(*pCmd)
		|| (pCmd->cbSize - offsetof(MSG64, msg)) / sizeof(pCmd->msg[0]) != pCmd->nCount
		|| (pCmd->cbSize - offsetof(MSG64, msg)) % sizeof(pCmd->msg[0]))
	{
		return FALSE;
	}
	else
	{
		_ASSERTE(pCmd->cbSize == (sizeof(*pCmd)+(pCmd->nCount-1)*sizeof(pCmd->msg[0])));

		#ifdef _DEBUG
		wchar_t* pszPasting = (wchar_t*)malloc((pCmd->nCount+1)*sizeof(wchar_t));
		if (pszPasting)
			*pszPasting = 0;
		wchar_t* pszDbg = pszPasting;
		#endif

		for (DWORD i = 0; i < pCmd->nCount; i++)
		{
			// При посылке массовых нажатий клавиш (вставка из буфера)
			// очередь может "не успевать"
			if (i && !(i & 31))
			{
				InputLogger::Log(InputLogger::Event::evt_InputQueueFlush);
				gpSrv->InputQueue.WriteInputQueue(NULL, TRUE);
				Sleep(10);
			}

			#ifdef _DEBUG
			switch (pCmd->msg[i].message)
			{
			case WM_KEYDOWN: case WM_SYSKEYDOWN: DEBUGSTRINPUTPIPE(L"ConEmuC: Received key down\n"); break;
			case WM_KEYUP: case WM_SYSKEYUP: DEBUGSTRINPUTPIPE(L"ConEmuC: Received key up\n"); break;
			default: DEBUGSTRINPUTPIPE(L"ConEmuC: Recieved input\n");
			}
			#endif

			INPUT_RECORD r;

			// Сбросим флаг
			InputLogger::Log(InputLogger::Event::evt_ResetEvent, gpSrv->InputQueue.nUsedLen);
			ResetEvent(gpSrv->hInputWasRead);

			// Некорректные события - отсеиваются,
			// некоторые события (CtrlC/CtrlBreak) не пишутся в буферном режиме
			InputLogger::Log(InputLogger::Event::evt_ProcessInputMessage, gpSrv->InputQueue.nUsedLen);
			if (ProcessInputMessage(pCmd->msg[i], r))
			{
				//SendConsoleEvent(&r, 1);
				InputLogger::Log(InputLogger::Event::evt_WriteInputQueue1, r, gpSrv->InputQueue.nUsedLen);
				BOOL bWritten = gpSrv->InputQueue.WriteInputQueue(&r, FALSE);
				// Облом с записью в наш внутренний буфер может быть только при его переполнении
				// Т.е. InputThread еще не успел считать данные из внутреннего буфера
				// и еще не перекинул считанное в консольный буфер.
				if (!bWritten)
				{
					// Передернуть на всякий случай очередь
					InputLogger::Log(InputLogger::Event::evt_InputQueueFlush);
					gpSrv->InputQueue.WriteInputQueue(NULL, TRUE);

					// Подождем чуть-чуть
					InputLogger::Log(InputLogger::Event::evt_WaitInputReady, gpSrv->InputQueue.nUsedLen);
					DWORD nReadyWait = WaitForSingleObject(gpSrv->hInputWasRead, 250);
					if (nReadyWait == WAIT_OBJECT_0)
					{
						InputLogger::Log(InputLogger::Event::evt_WriteInputQueue2, gpSrv->InputQueue.nUsedLen);
						bWritten = gpSrv->InputQueue.WriteInputQueue(&r, FALSE);
					}

					if (!bWritten)
					{
						InputLogger::Log(InputLogger::Event::evt_Overflow, gpSrv->InputQueue.nUsedLen);
						_InterlockedIncrement(&InputLogger::g_overflow);
						OutputDebugString(L"\n!!!ConEmuC input buffer overflow!!!\n");
						_ASSERTE(FALSE && "Input buffer overflow");
					}
					WARNING("Если буфер переполнен - ждать? Хотя если будем ждать здесь - может повиснуть GUI на записи в pipe...");
				}

				#ifdef _DEBUG
				if (bWritten && pszDbg && (r.EventType == KEY_EVENT) && r.Event.KeyEvent.bKeyDown)
				{
					*(pszDbg++) = r.Event.KeyEvent.uChar.UnicodeChar;
					*pszDbg = 0;
				}
				#endif
			}

			MCHKHEAP;
		}

		InputLogger::Log(InputLogger::Event::evt_InputQueueFlush);
		gpSrv->InputQueue.WriteInputQueue(NULL, TRUE);

		#ifdef _DEBUG
		SafeFree(pszPasting);
		#endif
	}

	return FALSE; // Inbound only
}

//void WINAPI InputServerFree(MSG64* pReply, LPARAM lParam)
//{
//	SafeFree(pReply);
//}







BOOL WINAPI CmdServerCommand(LPVOID pInst, CESERVER_REQ* pIn, CESERVER_REQ* &ppReply, DWORD &pcbReplySize, DWORD &pcbMaxReplySize, LPARAM lParam)
{
	BOOL lbRc = FALSE;
	CESERVER_REQ *pOut = NULL;
	ExecuteFreeResult(ppReply);

	if (pIn->hdr.nVersion != CESERVER_REQ_VER)
	{
		_ASSERTE(pIn->hdr.nVersion == CESERVER_REQ_VER);
		// переоткрыть пайп!
		gpSrv->CmdServer.BreakConnection(pInst);
		return FALSE;
	}

	if (pIn->hdr.bAsync)
		gpSrv->CmdServer.BreakConnection(pInst);

	if (ProcessSrvCommand(*pIn, &pOut))
	{
		lbRc = TRUE;
		ppReply = pOut;
		pcbReplySize = pOut->hdr.cbSize;
	}

	return lbRc;
}

void WINAPI CmdServerFree(CESERVER_REQ* pReply, LPARAM lParam)
{
	ExecuteFreeResult(pReply);
}



BOOL WINAPI DataServerCommand(LPVOID pInst, CESERVER_REQ* pIn, CESERVER_REQ* &ppReply, DWORD &pcbReplySize, DWORD &pcbMaxReplySize, LPARAM lParam)
{
	BOOL lbRc = FALSE;
	//CESERVER_REQ *pOut = NULL;
	//ExecuteFreeResult(ppReply);

	if ((pIn->hdr.nVersion != CESERVER_REQ_VER)
		|| (pIn->hdr.nCmd != CECMD_CONSOLEDATA))
	{
		_ASSERTE(pIn->hdr.nVersion == CESERVER_REQ_VER);
		_ASSERTE(pIn->hdr.nCmd == CECMD_CONSOLEDATA);
		// переоткрыть пайп!
		gpSrv->DataServer.BreakConnection(pInst);
		return FALSE;
	}


	if (gpSrv->pConsole->bDataChanged == FALSE)
	{
		pcbReplySize = sizeof(gpSrv->pConsole->ConState);
		if (ExecuteNewCmd(ppReply, pcbMaxReplySize, pIn->hdr.nCmd, pcbReplySize))
		{
			// Отдаем только инфу, без текста/атрибутов
			memmove(&ppReply->ConState, &gpSrv->pConsole->ConState, pcbReplySize);
			ppReply->ConState.nDataCount = 0;
			lbRc = TRUE;
		}
	}
	else //if (Command.nCmd == CECMD_CONSOLEDATA)
	{
		_ASSERTE(pIn->hdr.nCmd == CECMD_CONSOLEDATA);
		gpSrv->pConsole->bDataChanged = FALSE;

		SMALL_RECT rc = gpSrv->pConsole->ConState.sbi.srWindow;
		const int iWidth = rc.Right - rc.Left + 1;
		const int iHeight = rc.Bottom - rc.Top + 1;
		if (iWidth < 0 || iHeight < 0)
		{
			_ASSERTE(iWidth >= 0 && iHeight >= 0);
			return FALSE;
		}
		DWORD ccCells = iWidth * iHeight;

		const auto& crMaxSize = gpSrv->pConsole->ConState.crMaxSize;
		_ASSERTE(crMaxSize.X > 0 && crMaxSize.Y > 0);
		// We should fit, ReadConsoleData corrects possible size. But check.
		const DWORD ccMaxSizeCells = static_cast<uint32_t>(static_cast<uint16_t>(crMaxSize.X))
			* static_cast<uint32_t>(static_cast<uint16_t>(crMaxSize.Y));
		if (ccCells > ccMaxSizeCells)
		{
			_ASSERTE(ccCells <= ccMaxSizeCells);
			ccCells = ccMaxSizeCells;
		}

		gpSrv->pConsole->ConState.nDataCount = ccCells;
		const size_t cbDataSize = sizeof(gpSrv->pConsole->ConState) + ccCells * sizeof(CHAR_INFO);
		pcbReplySize = sizeof(CESERVER_REQ_HDR) + cbDataSize;
		if (ExecuteNewCmd(ppReply, pcbMaxReplySize, pIn->hdr.nCmd, pcbReplySize))
		{
			memmove(&ppReply->ConState, &gpSrv->pConsole->ConState, cbDataSize);
			lbRc = TRUE;
		}
		//fSuccess = WriteFile(gpSrv->hGetDataPipe, &(gpSrv->pConsole->info), cbWrite, &cbBytesWritten, NULL);
	}

	return lbRc;
}

void WINAPI DataServerFree(CESERVER_REQ* pReply, LPARAM lParam)
{
	ExecuteFreeResult(pReply);
}
