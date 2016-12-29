#include "stdafx.h"
#include "EVR_Video_Experiment.h"
#include "helpers.h"
#include "GUID.cpp"


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
const LPCWSTR const VIDEOpass(L"C:\\Downloads\\sakura2.mkv");
//↑動画のパスをここに入力して、実行してみよう!('\'はエスケープシーケンスが働いているので注意)
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

// 外部変数構造体
static struct {
	IBaseFilter *pEvr;
	IGraphBuilder *pGraph;
	IMediaControl *pControl;
	IMediaSeeking *pSeek;
	IMediaEventEx *pEvent;
	IMFVideoDisplayControl *pVideo;
	ICaptureGraphBuilder2 *pCGB2;
	SIZE size;
	int nPlay;
	int Cusor = 0;
	unsigned int Cusor_time;
	bool FullScreen_Flag;
} g;

#define WM_DIRECTSHOWMESSAGE (WM_APP + 1)
#define SAFE_RELEASE(x) { if (x) { x->Release(); x = NULL; } }
const LPCWSTR const CLASSname(L"DirectShow_EVR");
const LPCWSTR const WINDOWname(L"DirectShow_EVR");

// 関数プロトタイプ宣言
HRESULT OpenFile(HWND hWnd, LPCWSTR pszFileName);
HRESULT InitEvr(HWND hWnd);
HRESULT SetVideoPos(HWND hWnd, int nMode);
LRESULT CALLBACK MainWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

//------------------------------------------------------------------------------
void FullScreen(HWND hWnd)
{
	MFVideoNormalizedRect mvnr = { 0.0f, 0.0f, 1.0f, 1.0f };
	RECT rcDst;
	if(!(g.FullScreen_Flag)){
		g.FullScreen_Flag = true;
		// ウィンドウスタイル変更(メニューバーなし、最前面)
		SetWindowLongPtr(hWnd, GWL_STYLE, WS_POPUP);
		SetWindowLongPtr(hWnd, GWL_EXSTYLE, WS_EX_TOPMOST);

		// 最大化する
		ShowWindow(hWnd, SW_MAXIMIZE);

		// ディスプレイサイズを取得
		int mainDisplayWidth = GetSystemMetrics(SM_CXSCREEN);
		int mainDisplayHeight = GetSystemMetrics(SM_CYSCREEN);

		// クライアント領域をディスプレーに合わせる
		SetWindowPos(hWnd, NULL,0, 0, mainDisplayWidth, mainDisplayHeight,SWP_FRAMECHANGED | SWP_NOZORDER | SWP_NOMOVE);

		SetRect(&rcDst, 0, 0, g.size.cx, g.size.cy);
		GetClientRect(hWnd, &rcDst);
		//g.pVideo->SetVideoPosition(&mvnr, &rcDst);(必要ない)
	}
	else {
		g.FullScreen_Flag = false;
		SetWindowLongPtr(hWnd, GWL_STYLE, WS_OVERLAPPEDWINDOW);
		//SetWindowLongPtr(hWnd, GWL_EXSTYLE, WS_EX_TOPMOST);

		// 普通に戻す
		ShowWindow(hWnd, SW_RESTORE); 
		SetVideoPos(hWnd,1);
	}
}

//------------------------------------------------------------------------------
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	WNDCLASSEX wcx;
	HWND hWnd = 0;
	MSG msg = { NULL };
	HRESULT hr = 0;

	BOOL bIsComplete = 0;
	long lEventCode = 0;
	LONG_PTR lEvParam1, lEvParam2 = 0;
	bIsComplete = FALSE;

	g.FullScreen_Flag = false;//Flagの初期化
	// COMライブラリの初期化
	hr = CoInitialize(NULL);
	if (FAILED(hr)) {
		return 0;
	}

	// ウィンドウクラスの登録
	ZeroMemory(&wcx, sizeof wcx);
	wcx.cbSize = sizeof wcx;
	wcx.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
	wcx.lpfnWndProc = MainWndProc;
	wcx.hInstance = hInstance;
	wcx.hCursor = LoadCursor(NULL, MAKEINTRESOURCE(IDC_ARROW));
	wcx.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);	// 黒がいいかも
	wcx.lpszClassName = CLASSname;
	if (RegisterClassEx(&wcx) == 0) {
		goto Exit;
	}

	// ウィンドウの作成
	hWnd = CreateWindow(CLASSname, WINDOWname,
		WS_OVERLAPPEDWINDOW,
		//		CW_USEDEFAULT, 0, CW_USEDEFAULT, 0,
		100, 100, 640, 480,
		NULL, NULL, hInstance, NULL);
	if (hWnd == NULL) {
		goto Exit;
	}

	// DirectShowフィルタの準備

	hr = OpenFile(hWnd, VIDEOpass);
	if (FAILED(hr)) {

		goto Exit;
	}
	hr = g.pEvent->SetNotifyWindow((OAHWND)hWnd, WM_DIRECTSHOWMESSAGE, (LPARAM)NULL);//IMediaEventExの準備
	ShowWindow(hWnd, nCmdShow);

	// 動画再生
	hr = g.pControl->Run();
	g.nPlay = 1;

	// メッセージループ
	do {
		Sleep(1);
		if (g.Cusor >= 0) {
			if (g.Cusor_time > 0) {
				g.Cusor_time--;
			}
			else{//(g.Cusor_time == 0)と同意義 
				g.Cusor = ShowCursor(false);
			}
		}
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		do{
			//イベントを取得
			hr = g.pEvent->GetEvent(&lEventCode, &lEvParam1, &lEvParam2, 0);
			if (hr == S_OK) {
				//再生終了であったときフラグを立てる
				if (lEventCode == EC_COMPLETE) bIsComplete = TRUE;

				//イベントを削除
				g.pEvent->FreeEventParams(lEventCode, lEvParam1, lEvParam2);
			}
		} while (hr == S_OK);
		if (bIsComplete) {
			g.pControl->Stop();
			SendMessage(hWnd, WM_CLOSE, 0, 0);
		}
	} while (msg.message != WM_QUIT);

	// 動画停止
	hr = g.pControl->Stop();
	Sleep(1000);
Exit:
	SAFE_RELEASE(g.pSeek);
	SAFE_RELEASE(g.pEvr);
	SAFE_RELEASE(g.pEvent);
	SAFE_RELEASE(g.pVideo);
	SAFE_RELEASE(g.pCGB2);
	SAFE_RELEASE(g.pControl);
	SAFE_RELEASE(g.pGraph);
	CoUninitialize();
	return msg.wParam;
}

//------------------------------------------------------------------------------
HRESULT OpenFile(HWND hWnd, LPCWSTR pszFile)
{
	// フィルタグラフの作成
	HRESULT hr = CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&g.pGraph));

	// メディアコントロールインターフェイスの取得
	if (SUCCEEDED(hr)) {
		hr = g.pGraph->QueryInterface(IID_PPV_ARGS(&g.pControl));
	}

	if (SUCCEEDED(hr)) {
		hr = CoCreateInstance(CLSID_CaptureGraphBuilder2, NULL, CLSCTX_INPROC_SERVER, IID_ICaptureGraphBuilder2, (void **)&g.pCGB2);
	}

	if (SUCCEEDED(hr)) {
		hr = g.pGraph->QueryInterface(IID_IMediaSeeking, (void**)&g.pSeek);
	}

	if (SUCCEEDED(hr)) {
		hr = g.pGraph->QueryInterface(IID_IMediaEventEx, (void**)&g.pEvent);
	}

	if (SUCCEEDED(hr)) {
		hr = g.pCGB2->SetFiltergraph(g.pGraph);//キャプチャ グラフ ビルダが使うフィルタ グラフを指定する。
	}

	// ビデオの作成
	if (SUCCEEDED(hr)) {

		hr = InitEvr(hWnd);
	}

	if (SUCCEEDED(hr)) {
		//hr = g.pCGB2->RenderStream(0, 0, g.pSource, 0, g.pEvr);

		IBaseFilter *pSrc = 0, *pDec = 0, *pRawfilter = 0, *pVSFilter = 0;
		IPin *pSubtitle = 0;
		GUID CLSID_LAVVideo, CLSID_ffdshow_raw, CLSID_VSFilter, CLSID_MEDIATYPE_Subtitle, CLSID_ffdshow_Video;
		HRESULT Connect;//Directshowのフィルタの接続安否(最終的には字幕の接続安否に使われる)

		try {
			GuidFromString(&CLSID_MEDIATYPE_Subtitle, MEDIATYPE_Subtitle);
			hr = g.pGraph->AddSourceFilter(pszFile, pszFile, &pSrc);
			if (SUCCEEDED(hr)) {
				GuidFromString(&CLSID_LAVVideo, LAV_Video);
				Connect = AddFilter(g.pGraph, CLSID_LAVVideo, L"LAV Video Decoder", &pDec);
				if (SUCCEEDED(Connect)) {
					hr = g.pCGB2->RenderStream(0, 0, pSrc, 0, pDec);
					if (FAILED(hr)) {
						throw "LAV_Video_Decoder didn't Connect";
					}
					GuidFromString(&CLSID_ffdshow_raw, ffdshow_Raw);
					Connect = AddFilter(g.pGraph, CLSID_ffdshow_raw, L"ffdshow raw video filter", &pRawfilter);
					if (SUCCEEDED(Connect)) {
						hr = g.pCGB2->RenderStream(0, 0, pDec, 0, pRawfilter);
						if (FAILED(hr)) {
							throw "ffdshow_raw_video_filter didn't Connect";
						}
						GuidFromString(&CLSID_VSFilter, VSFilter);
						Connect = AddFilter(g.pGraph, CLSID_VSFilter, L"VSFilter", &pVSFilter);
						if (SUCCEEDED(Connect)) {
							//Full installed.
							hr = g.pCGB2->RenderStream(0, 0, pRawfilter, 0, pVSFilter);
							if (FAILED(hr)) {
								throw "VSFilter didn't Connect";
							}
							hr = g.pCGB2->RenderStream(0, 0, pVSFilter, 0, g.pEvr);
							if (FAILED(hr)) {
								throw "Enhaced_Video_Renderer didn't Connect";
							}
							HRESULT hr_SubTitle = g.pCGB2->FindPin(pSrc,
								PINDIR_OUTPUT,
								NULL,
								&CLSID_MEDIATYPE_Subtitle,
								true,
								NULL,
								&pSubtitle
							);//字幕のピンを探す
							if (SUCCEEDED(hr_SubTitle)) {
								Connect = g.pCGB2->RenderStream(0, &CLSID_MEDIATYPE_Subtitle, pSrc, 0, pVSFilter);//字幕の接続
							}
						}
						else {
							//VSFilter is not installed.
							hr = g.pCGB2->RenderStream(0, 0, pRawfilter, 0, g.pEvr);
							HRESULT hr_SubTitle = g.pCGB2->FindPin(pSrc,
								PINDIR_OUTPUT,
								NULL,
								&CLSID_MEDIATYPE_Subtitle,
								true,
								NULL,
								&pSubtitle
							);//字幕のピンを探す
							if (SUCCEEDED(hr_SubTitle)) {
								Connect = g.pCGB2->RenderStream(0, &CLSID_MEDIATYPE_Subtitle, pSrc, 0, pRawfilter);//字幕の接続
							}
						}
					}
					else {
						//LAV_Filters is installed only.
						hr = g.pCGB2->RenderStream(0, 0, pDec, 0, g.pEvr);
						if (FAILED(hr)) {
							throw "Enhaced_Video_Renderer didn't Connect";
						}
					}
				}
				else {
					//LAV_Video_Decorderがないということなので、代案でffdshowに接続
					GuidFromString(&CLSID_ffdshow_Video, ffdshow_Video);
					Connect = AddFilter(g.pGraph, CLSID_ffdshow_Video, L"ffdshow Decoder", &pDec);
					if (SUCCEEDED(Connect)) {
						hr = g.pCGB2->RenderStream(0, 0, pSrc, 0, pDec);
						if (FAILED(hr)) {
							throw "ffdshow Decoder didn't Connect";
						}
						GuidFromString(&CLSID_VSFilter, VSFilter);
						Connect = AddFilter(g.pGraph, CLSID_VSFilter, L"VSFilter", &pVSFilter);
						if (SUCCEEDED(Connect)) {
							//LAV_Filter is not installed.
							hr = g.pCGB2->RenderStream(0, 0, pDec, 0, pVSFilter);
							if (FAILED(hr)) {
								throw "VSFilter didn't Connect";
							}
							hr = g.pCGB2->RenderStream(0, 0, pVSFilter, 0, g.pEvr);
							if (FAILED(hr)) {
								throw "Enhaced_Video_Renderer didn't Connect";
							}
							HRESULT hr_SubTitle = g.pCGB2->FindPin(pSrc,
								PINDIR_OUTPUT,
								NULL,
								&CLSID_MEDIATYPE_Subtitle,
								true,
								NULL,
								&pSubtitle
							);//字幕のピンを探す
							if (SUCCEEDED(hr_SubTitle)) {
								Connect = g.pCGB2->RenderStream(0, &CLSID_MEDIATYPE_Subtitle, pSrc, 0, pVSFilter);//字幕の接続
							}
						}
						else {
							//ffdshow is installed only. 
							hr = g.pCGB2->RenderStream(0, 0, pDec, 0, g.pEvr);
							if (FAILED(hr)) {
								throw "Enhaced_Video_Renderer didn't Connect";
							}
							HRESULT hr_SubTitle = g.pCGB2->FindPin(pSrc,
								PINDIR_OUTPUT,
								NULL,
								&CLSID_MEDIATYPE_Subtitle,
								true,
								NULL,
								&pSubtitle
							);//字幕のピンを探す
							if (SUCCEEDED(hr_SubTitle)) {
								Connect = g.pCGB2->RenderStream(0, &CLSID_MEDIATYPE_Subtitle, pSrc, 0, pDec);//字幕の接続
							}
						}
					}
					else {
						//LAV_Filters or ffdshow are not installed.
						std::cout << "foo_video[Warning]:Your PC is not install LAV_Filters or ffdshow. You should install LAV_Filters or ffdshow";
						hr = g.pCGB2->RenderStream(0, 0, pSrc, 0, g.pEvr);
					}
				}
			}
			else {
				throw "AddSorceFilter Failed";
			}
		}
		catch (char *str) {
			std::cout << "foo_video[Fatal_Error]:" << str;
			std::cout << "foo_video[Error_Info]: Reason:" << hr;
		}

		//オーディオの接続
		hr = g.pCGB2->RenderStream(0, &MEDIATYPE_Audio, pSrc, 0, 0);
		hr = 0;
		SAFE_RELEASE(pSubtitle);
		SAFE_RELEASE(pSrc);
		SAFE_RELEASE(pDec);
		SAFE_RELEASE(pRawfilter);
		SAFE_RELEASE(pVSFilter);
	}

	// 描画領域の設定
	if (SUCCEEDED(hr)) {
		g.pVideo->GetNativeVideoSize(&g.size, NULL);
	}

	if (SUCCEEDED(hr)) {
		hr = SetVideoPos(hWnd, 1);
	}

	return hr;
}

//------------------------------------------------------------------------------
HRESULT InitEvr(HWND hWnd)
{

	// EVRの作成
	HRESULT hr = CoCreateInstance(CLSID_EnhancedVideoRenderer, NULL, CLSCTX_INPROC_SERVER,
		IID_PPV_ARGS(&g.pEvr));

	// フィルタグラフにEVRを追加
	if (SUCCEEDED(hr)) {
		hr = g.pGraph->AddFilter(g.pEvr, L"EVR");
	}

	IMFGetService *pService = NULL;
	if (SUCCEEDED(hr)) {
		hr = g.pEvr->QueryInterface(IID_PPV_ARGS(&pService));
	}
	if (SUCCEEDED(hr)) {
		hr = pService->GetService(MR_VIDEO_RENDER_SERVICE, IID_PPV_ARGS(&g.pVideo));
	}
	SAFE_RELEASE(pService);

	if (SUCCEEDED(hr)) {
		hr = g.pVideo->SetVideoWindow(hWnd);
	}
	return hr;
}

//------------------------------------------------------------------------------
HRESULT SetVideoPos(HWND hWnd, int nMode)
{
	MFVideoNormalizedRect mvnr = { 0.0f, 0.0f, 1.0f, 1.0f };
	RECT rcDst;

	if (1 <= nMode && nMode <= 4) {
		SetRect(&rcDst, 0, 0, g.size.cx * nMode / 2, g.size.cy * nMode / 2);
		AdjustWindowRectEx(&rcDst, WS_OVERLAPPEDWINDOW, FALSE, 0);
		SetWindowPos(hWnd, NULL, 0, 0, rcDst.right - rcDst.left, rcDst.bottom - rcDst.top,
			SWP_NOZORDER | SWP_NOMOVE);
	}
	GetClientRect(hWnd, &rcDst);
	return g.pVideo->SetVideoPosition(&mvnr, &rcDst);
}

//------------------------------------------------------------------------------
LRESULT CALLBACK MainWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_CHAR:
		switch (wParam) {
		case VK_SPACE:
			if (g.nPlay) {
				g.pControl->Pause();
				g.nPlay = 0;
			}
			else {
				g.pControl->Run();
				g.nPlay = 1;
			}
			break;
		case 's':
			g.pControl->StopWhenReady();
			g.nPlay = 0;
			break;
		case '1': case '2': case '3': case '4':
			SetVideoPos(hWnd, wParam - '0');
			break;
		case VK_ESCAPE:
			DestroyWindow(hWnd);
			break;
		}
		break;
	case WM_SIZE:
		SetVideoPos(hWnd, 0);
		break;	
	case WM_LBUTTONDOWN:
		if(!(g.FullScreen_Flag)){
			PostMessage(hWnd, WM_NCLBUTTONDOWN, HTCAPTION, lParam);//クライアントの上でウィンドウを動かす。
		}
		break;
	case WM_LBUTTONDBLCLK:
		FullScreen(hWnd);
		break;
	case WM_DESTROY:
		while (g.Cusor < 0) {
			g.Cusor = ShowCursor(true);
		}
		PostQuitMessage(0);
		break;
	case WM_CREATE:
		g.Cusor_time = 1000;
		break;
	case WM_MOUSEMOVE:
		g.Cusor_time = 1000;
		while (g.Cusor < 0) {
			g.Cusor=ShowCursor(true);
		}
		break;
	case WM_DIRECTSHOWMESSAGE:
		//g.pControl->Stop();
		break;
	default:
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}
	return 0;
}