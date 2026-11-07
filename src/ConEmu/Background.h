
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

#pragma once

#define BACKGROUND_FILE_POLL 5000

class MSection;

class CBackground
{
public:
	COORD    bgSize;
	POINT    VConOffset;
	HDC      hBgDc;

	CBackground();
	virtual ~CBackground();

	bool CreateField(int anWidth, int anHeight);
	bool FillBackground(const BITMAPFILEHEADER* apBkImgData, LONG X, LONG Y, LONG Width, LONG Height, BackgroundOp Operation, bool abFade, bool preserveColors = false);

	SetBackgroundResult SetPluginBackgroundImageData(CESERVER_REQ_SETBACKGROUND* apImgData, bool&/*OUT*/ bUpdate);
	bool HasPluginBackgroundImage(LONG* pnBgWidth, LONG* pnBgHeight);
	bool PrepareBackground(CVirtualConsole* pVCon, HDC&/*OUT*/ phBgDc, COORD&/*OUT*/ pbgBmpSize);

	void NeedBackgroundUpdate();

	static UINT IsBackgroundValid(const CESERVER_REQ_SETBACKGROUND* apImgData, bool* rpIsEmf);

private:
	HBITMAP  hBgBitmap, hOldBitmap;
	void Destroy();

	#ifdef __GNUC__
	AlphaBlend_t GdiAlphaBlend;
	#endif

	bool mb_NeedBgUpdate;


	bool PutPluginBackgroundImage(/*CBackground* pBack,*/ LONG X, LONG Y, LONG Width, LONG Height);

	// Данные, которые приходят из плагина ("ConEmu Background" и пр.)
	bool mb_BgLastFade;
	//CBackground* mp_PluginBg;
	MSection *mcs_BkImgData;
	size_t mn_BkImgDataMax;
	CESERVER_REQ_SETBACKGROUND* mp_BkImgData; // followed by image data
	bool mb_BkImgChanged; // Данные в mp_BkImgData были изменены плагином, требуется отрисовка
	bool mb_BkImgExist; //, mb_BkImgDelete;
	LONG mn_BkImgWidth, mn_BkImgHeight;
	// Поддержка EMF
	size_t mn_BkEmfDataMax;
	CESERVER_REQ_SETBACKGROUND* mp_BkEmfData; // followed by EMF data
	bool mb_BkEmfChanged; // Данные в mp_BkEmfData были изменены плагином, требуется отрисовка
};


#ifdef APPDISTINCTBACKGROUND

#include "RefRelease.h"
#include "../common/MArray.h"

class CBackgroundInfo : public CRefRelease
{
protected:
	wchar_t   ms_BgImage[MAX_PATH]; // tBgImage
	FILETIME  ftBgModified;
	DWORD     nBgModifiedTick;
	//bool      mb_NeedBgUpdate;

	bool mb_IsBackgroundImageValid;

	BITMAPFILEHEADER* mp_BgImgData; // Результат LoadImageEx

protected:
	CBackgroundInfo(LPCWSTR inPath);
	virtual ~CBackgroundInfo();

	static MArray<CBackgroundInfo*> g_Backgrounds;
public:
	static CBackgroundInfo* CreateBackgroundObject(LPCWSTR inPath, bool abShowErrors);

	void FinalRelease();

	const wchar_t* BgImage();
	//bool IsImageExist();
	const BITMAPFILEHEADER* GetBgImgData();
	//bool IsFade();

	//void NeedBackgroundUpdate();

	bool PollBackgroundFile(/* const Settings::AppSettings* pApp */);
	bool LoadBackgroundFile(bool abShowErrors);
	//bool PrepareBackground(HDC* phBgDc, COORD* pbgBmpSize);
};

#endif
