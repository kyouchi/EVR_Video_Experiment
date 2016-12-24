#include "stdafx.h"
#include "helpers.h"
#pragma comment(lib, "rpcrt4.lib")//ないと未解決のエラー
//------------------------------------------------------------------------------
HRESULT GetUnconnectedPin(
	IBaseFilter *pFilter,   // フィルタへのポインタ
	PIN_DIRECTION PinDir,   // 探すピンの向き
	IPin **ppPin)           // ピンへのポインタが格納される
{
	*ppPin = 0;
	IEnumPins *pEnum = 0;
	IPin *pPin = 0;
	HRESULT hr = pFilter->EnumPins(&pEnum);
	if (FAILED(hr))
	{
		return hr;
	}
	while (pEnum->Next(1, &pPin, NULL) == S_OK)
	{
		PIN_DIRECTION ThisPinDir;
		pPin->QueryDirection(&ThisPinDir);
		if (ThisPinDir == PinDir)
		{
			IPin *pTmp = 0;
			hr = pPin->ConnectedTo(&pTmp);
			if (SUCCEEDED(hr))  // 接続済み。われわれの探しているピンではない
			{
				pTmp->Release();
			}
			else  // 適合するピンは見つからなかった
			{
				pEnum->Release();
				*ppPin = pPin;
				return S_OK;
			}
		}
		pPin->Release();
	}
	pEnum->Release();
	// Didn't find a matching pin.
	return E_FAIL;
}

HRESULT ConnectFilters(IGraphBuilder *pGraph, IBaseFilter *pSrc,
	IBaseFilter *pDest)
{
	IPin *pOut = 0, *pIn = 0;
	HRESULT hr = GetUnconnectedPin(pSrc, PINDIR_OUTPUT, &pOut);
	if (FAILED(hr))
	{
		return hr;
	}
	hr = GetUnconnectedPin(pDest, PINDIR_INPUT, &pIn);
	if (FAILED(hr))
	{
		pIn->Release();
		return hr;
	}
	hr = pGraph->Connect(pOut, pIn);
	pOut->Release();
	pIn->Release();
	return hr;
}

HRESULT AddFilter(
	IGraphBuilder *pGraph,  // Filter Graph Manager へのポインタ
	const GUID& clsid,      // 作成するフィルタの CLSID
	LPCWSTR wszName,        // フィルタの名前
	IBaseFilter **ppF)      // フィルタへのポインタが格納される
{
	*ppF = 0;
	IBaseFilter *pF = 0;
	HRESULT hr = CoCreateInstance(clsid, 0, CLSCTX_INPROC_SERVER,
		IID_IBaseFilter, reinterpret_cast<LPVOID*>(&pF));
	if (SUCCEEDED(hr))
	{
		hr = pGraph->AddFilter(pF, wszName);
		if (SUCCEEDED(hr))
			*ppF = pF;
		else
			pF->Release();
	}
	return hr;
}

std::wstring GuidToString
(
	GUID* pGuid
)
{
	std::wstring oGuidString;
	RPC_WSTR waString;

	// GUIDを文字列へ変換する
	if (RPC_S_OK == ::UuidToString(pGuid, &waString)) {

		// GUIDを結果にセット
		oGuidString = (WCHAR*)waString;

		// GUID文字列の破棄
		RpcStringFree(&waString);
	}

	// GUIDを返す
	return(oGuidString);
}

BOOL GuidFromString
(
	GUID* pGuid
	, std::wstring oGuidString
)
{
	// 文字列をGUIDに変換する
	if (RPC_S_OK == ::UuidFromString((RPC_WSTR)oGuidString.c_str(), (UUID*)pGuid)) {

		// 変換できました。
		return(TRUE);
	}
	return(FALSE);
}