#pragma once
#include "stdafx.h"
#include "resource.h"
extern int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow);
extern void FullScreen(HWND hWnd);
extern HRESULT OpenFile(HWND hWnd, LPCWSTR pszFile);
extern HRESULT InitEvr(HWND hWnd);
extern HRESULT SetVideoPos(HWND hWnd, int nMode);
extern LRESULT CALLBACK MainWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);