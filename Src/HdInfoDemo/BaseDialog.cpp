#include "BaseDialog.h"
#include "PUID.h"

#include <memory>

// 消息通知
void BaseDialog::Notify(TNotifyUI& msg)
{     
     // 如为点击消息则通过控件名字判断是哪个控件
    if ( msg.sType == _T("click")) {
        if( msg.pSender == static_cast<CButtonUI*>(m_paintMgr.FindControl(_T("minbtn"))) ) {
            SendMessage(WM_SYSCOMMAND, SC_MINIMIZE, 0);
		}
  
        if( msg.pSender == static_cast<CButtonUI*>(m_paintMgr.FindControl(_T("closebtn"))) ) {
            PostQuitMessage(0);
		}
  
        if( msg.pSender == static_cast<CButtonUI*>(m_paintMgr.FindControl(_T("maxbtn"))) ) {
            ::IsZoomed(*this) ?
				SendMessage(WM_SYSCOMMAND, SC_RESTORE, 0) : 
				SendMessage(WM_SYSCOMMAND, SC_MAXIMIZE, 0);
		}

        if (msg.pSender == static_cast<CButtonUI*>(m_paintMgr.FindControl(_T("check_normal")))) {
            ::MessageBox(NULL, L"开始扫描", L"提示", MB_OK);
        }
    }
}
  
// 首先启动消息循环会进入此虚函数进行消息处理
LRESULT BaseDialog::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    // 初始化消息映射MAP,利用auto_ptr维护指针,static保证只创建一次
    static std::auto_ptr<MessageMap> customMessageMap(InitMessageMap());
  
    BOOL bHandled = TRUE;
    LRESULT lRes = 0;
  
    // 将消息在消息映射map中进行查找 找到响应的消息处理函数
    if ( customMessageMap->find(uMsg) != customMessageMap->end() ) {
        // typedef HRESULT (BaseDialog::*CustomMsgHandler)(WPARAM, LPARAM, BOOL&);
        // 如果找到,查找相应的消息响应函数
        CustomMsgHandler handler = (*customMessageMap)[uMsg];
        // 通过this->(*handler)进行消息响应函数的调用
        lRes = (this->*handler)(wParam, lParam, bHandled);
        // 如果bHandled返回True没有被修改那么说明消息已经被处理,返回
		if (bHandled) { return lRes; }
    }
    // CPaintManagerUI丢给PaintManagerUI进行处理
	// 如果处理了返回True,否则返回false继续走
	if (m_paintMgr.MessageHandler(uMsg, wParam, lParam, lRes)) { return lRes; }
    // 最后丢给默认的windows消息处理函数
    return CWindowWnd::HandleMessage(uMsg, wParam, lParam);
}
  
// 初始化消息循环对应的消息响应函数
BaseDialog::MessageMap* BaseDialog::InitMessageMap()
{
    MessageMap* map = new MessageMap;
    (*map)[WM_CREATE] = &BaseDialog::OnCreate;
    (*map)[WM_DESTROY] = &BaseDialog::OnDestory;
    (*map)[WM_ERASEBKGND] = &BaseDialog::OnErasebkgnd;
    (*map)[WM_NCPAINT] = &BaseDialog::OnNcPaint;
    // 以下三个消息用于屏蔽系统标题栏
    (*map)[WM_NCACTIVATE] = &BaseDialog::OnNcActive;
    (*map)[WM_NCCALCSIZE] = &BaseDialog::OnNcCalcSize;
    (*map)[WM_NCHITTEST] = &BaseDialog::OnNcHit;
    (*map)[WM_SYSCOMMAND] = &BaseDialog::OnSysCommand;
    return map;
}
  
// 窗口创建时候
HRESULT BaseDialog::OnCreate( WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
    // 获取当前窗口风格
    LONG styleValue = ::GetWindowLong(*this, GWL_STYLE);
    styleValue &= ~WS_CAPTION;
    // 设置STYLE
    ::SetWindowLong(*this, GWL_STYLE, styleValue | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
    // 初始化界面渲染器
	m_paintMgr.Init(m_hWnd);

	CDialogBuilder builder;
    // 通过xml以及渲染器渲染界面UI
    CControlUI* pRoot = builder.Create(_T("duilib.xml"), (UINT)0, NULL, &m_paintMgr);
    // 附加界面UI到对话框容器
	m_paintMgr.AttachDialog(pRoot);
    // 增加消息处理,因为实现了INotifyUI接口
	m_paintMgr.AddNotifier(this);

	// 业务相关初始化
	Init();

    return 0;
}
  
HRESULT BaseDialog::OnDestory( WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
    // 发送退出消息
    ::PostQuitMessage(0L);
    return 0;
}
  
// 擦除背景
HRESULT BaseDialog::OnErasebkgnd( WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
    return 1;
}
  
HRESULT BaseDialog::OnNcPaint( WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
    return 0;
}
  
HRESULT BaseDialog::OnNcActive( WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
	if (::IsIconic(*this)) { bHandled = FALSE; }
    return (wParam == 0) ? TRUE : FALSE;
}
  
// 如果不处理那么就会导致DUILIB不停调用系统消息进行处理
// 屏蔽系统标题栏 似乎不屏蔽一定会出问题
HRESULT BaseDialog::OnNcCalcSize( WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
    return 0;
}
  
HRESULT BaseDialog::OnNcHit( WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
    return HTCLIENT;
}
  
// 系统命令处理
LRESULT BaseDialog::OnSysCommand(WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if( wParam == SC_CLOSE ) {
        ::PostQuitMessage(0L);
        bHandled = TRUE;
        return 0;
    }
  
    BOOL bZoomed = ::IsZoomed(*this);
    LRESULT lRes = CWindowWnd::HandleMessage(WM_SYSCOMMAND, wParam, lParam);
  
    return 1L;
}  

// 窗口初始化动作
void BaseDialog::Init() {
	CPUID* hdInfoTool = new CPUID();

	// 获取CPU序列号
	// CString cpuSn = hdInfoTool->GetCpuSerialNumber();
	CString cpuSn = hdInfoTool->GetCpuSn();
	// 获取硬盘序列号
	CString hdSn = hdInfoTool->getHDSerialNum();
	// 获取硬盘型号
	CString hdMod = hdInfoTool->getHDModelNum();
	// 获取路由Mac
	CString routeMac = hdInfoTool->getRouteMac();
	// 获取本机Mac
	CString localMac = hdInfoTool->GetMacInfo();

	// 硬盘序列号
	CTextUI* pTextHdSn = (CTextUI*)m_paintMgr.FindControl(_T("editHdSn"));
	pTextHdSn->SetText(hdSn);

	// 硬盘型号
	CTextUI* pTextHdMod = (CTextUI*)m_paintMgr.FindControl(_T("editHdMod"));
	pTextHdMod->SetText(hdMod);

	// CPU序列号
	CTextUI* pTextCpuId = (CTextUI*)m_paintMgr.FindControl(_T("editCPUID"));
	pTextCpuId->SetText(cpuSn);

	// 路由器MAC
	CTextUI* pTextRouteMac = (CTextUI*)m_paintMgr.FindControl(_T("editRouteMac"));
	pTextRouteMac->SetText(routeMac);

	// 本机MAC
	CTextUI* pTextLocalMac = (CTextUI*)m_paintMgr.FindControl(_T("editLocalMac"));
	pTextLocalMac->SetText(localMac);
}