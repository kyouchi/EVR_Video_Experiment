#include "stdafx.h"
HRESULT GetUnconnectedPin(IBaseFilter *pFilter, PIN_DIRECTION PinDir, IPin **ppPin);
HRESULT ConnectFilters(IGraphBuilder *pGraph, IBaseFilter *pSrc, IBaseFilter *pDest);
HRESULT AddFilter(IGraphBuilder *pGraph,const GUID& clsid,LPCWSTR wszName,IBaseFilter **ppF);
std::wstring GuidToString(GUID* pGuid);
BOOL GuidFromString(GUID* pGuid, std::wstring oGuidString);