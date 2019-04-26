#pragma once
#include "../DuiLib/UIlib.h"
#include "resource2.h"

#include "PUID.h"
#include "BaseDialog.h"

using namespace DuiLib;

#pragma comment(lib, "../Lib/DuiLib.lib")

class CFrameWindowWnd : public WindowImplBase
{
public:
	CFrameWindowWnd() { };
protected:
	LPCTSTR GetWindowClassName() const { return _T("DUIMainFrame"); };
	UINT GetClassStyle() const { return UI_CLASSSTYLE_FRAME | CS_DBLCLKS; };
	
	void OnFinalMessage(HWND /*hWnd*/) { delete this; };

	void Init() {
		CPUID* hdInfoTool = new CPUID();

		// 获取CPU序列号
		CString cpuSn = hdInfoTool->GetCpuSerialNumber();
		// 获取硬盘序列号
		CString hdSn = hdInfoTool->getHDSerialNum();
		// 获取硬盘型号
		CString hdMod = hdInfoTool->getHDModelNum();
		// 获取路由Mac
		CString routeMac = hdInfoTool->getRouteMac();

		// 硬盘序列号
		CTextUI* pTextHdSn = (CTextUI*)m_paintManager.FindControl(_T("editHdSn"));
		pTextHdSn->SetText(hdSn);

		// 硬盘型号
		CTextUI* pTextHdMod = (CTextUI*)m_paintManager.FindControl(_T("editHdMod"));
		pTextHdMod->SetText(hdMod);

		// CPU序列号
		CTextUI* pTextCpuId = (CTextUI*)m_paintManager.FindControl(_T("editCPUID"));
		pTextCpuId->SetText(cpuSn);

		// 路由器MAC
		CTextUI* pTextRouteMac = (CTextUI*)m_paintManager.FindControl(_T("editRouteMac"));
		pTextRouteMac->SetText(routeMac);

		delete hdInfoTool;
		hdInfoTool = NULL;
	}

	void Notify(TNotifyUI& msg)
	{

	}

	LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		if (uMsg == WM_CREATE) {
			m_paintManager.Init(m_hWnd);
			CDialogBuilder builder;
			CControlUI* pRoot = builder.Create(_T("duilib.xml"), (UINT)0, NULL, &m_paintManager);
			ASSERT(pRoot && "Failed to parse XML");
			m_paintManager.AttachDialog(pRoot);
			m_paintManager.AddNotifier(this);
			Init();
			return 0;
		} else if (uMsg == WM_DESTROY) {
			::PostQuitMessage(0L);
		} else if (uMsg == WM_ERASEBKGND) {
			return 1;
		}
		LRESULT lRes = 0;
		if (m_paintManager.MessageHandler(uMsg, wParam, lParam, lRes)) { return lRes; }
		return CWindowWnd::HandleMessage(uMsg, wParam, lParam);
	}

public:
	CPaintManagerUI m_paintManager;
};


int APIENTRY _tWinMain(HINSTANCE hInstance, 
						HINSTANCE hPrevInstance, 
						LPTSTR lpCmdLine, 
						int nCmdShow)
{
	CPaintManagerUI::SetInstance(hInstance);
	CPaintManagerUI::SetResourcePath(CPaintManagerUI::GetInstancePath());

	HRESULT Hr = ::CoInitialize(NULL);
	if (FAILED(Hr)) { return 0; }

	BaseDialog* pFrame = new BaseDialog();
	if (pFrame == NULL) { return 0; }
	pFrame->Create(NULL, _T("本机硬件信息"), UI_WNDSTYLE_FRAME, WS_EX_WINDOWEDGE);
	pFrame->SetIcon(IDI_ICON1);
	pFrame->CenterWindow();
	pFrame->ShowWindow(true);
	CPaintManagerUI::MessageLoop();

	// BaseDialog baseDlg;
	// baseDlg.Create(NULL, _T("机器硬件信息"), UI_WNDSTYLE_FRAME, WS_EX_WINDOWEDGE);
	// baseDlg.SetIcon(IDI_ICON1);
	// baseDlg.CenterWindow();
	// baseDlg.ShowModal();

	::CoUninitialize();
	return 0;
}