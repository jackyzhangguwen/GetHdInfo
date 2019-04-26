#include "BaseDialog.h"
#include "PUID.h"

#include <memory>

// ��Ϣ֪ͨ
void BaseDialog::Notify(TNotifyUI& msg)
{     
     // ��Ϊ�����Ϣ��ͨ���ؼ������ж����ĸ��ؼ�
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
            ::MessageBox(NULL, L"��ʼɨ��", L"��ʾ", MB_OK);
        }
    }
}
  
// ����������Ϣѭ���������麯��������Ϣ����
LRESULT BaseDialog::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    // ��ʼ����Ϣӳ��MAP,����auto_ptrά��ָ��,static��ֻ֤����һ��
    static std::auto_ptr<MessageMap> customMessageMap(InitMessageMap());
  
    BOOL bHandled = TRUE;
    LRESULT lRes = 0;
  
    // ����Ϣ����Ϣӳ��map�н��в��� �ҵ���Ӧ����Ϣ������
    if ( customMessageMap->find(uMsg) != customMessageMap->end() ) {
        // typedef HRESULT (BaseDialog::*CustomMsgHandler)(WPARAM, LPARAM, BOOL&);
        // ����ҵ�,������Ӧ����Ϣ��Ӧ����
        CustomMsgHandler handler = (*customMessageMap)[uMsg];
        // ͨ��this->(*handler)������Ϣ��Ӧ�����ĵ���
        lRes = (this->*handler)(wParam, lParam, bHandled);
        // ���bHandled����Trueû�б��޸���ô˵����Ϣ�Ѿ�������,����
		if (bHandled) { return lRes; }
    }
    // CPaintManagerUI����PaintManagerUI���д���
	// ��������˷���True,���򷵻�false������
	if (m_paintMgr.MessageHandler(uMsg, wParam, lParam, lRes)) { return lRes; }
    // ��󶪸�Ĭ�ϵ�windows��Ϣ������
    return CWindowWnd::HandleMessage(uMsg, wParam, lParam);
}
  
// ��ʼ����Ϣѭ����Ӧ����Ϣ��Ӧ����
BaseDialog::MessageMap* BaseDialog::InitMessageMap()
{
    MessageMap* map = new MessageMap;
    (*map)[WM_CREATE] = &BaseDialog::OnCreate;
    (*map)[WM_DESTROY] = &BaseDialog::OnDestory;
    (*map)[WM_ERASEBKGND] = &BaseDialog::OnErasebkgnd;
    (*map)[WM_NCPAINT] = &BaseDialog::OnNcPaint;
    // ����������Ϣ��������ϵͳ������
    (*map)[WM_NCACTIVATE] = &BaseDialog::OnNcActive;
    (*map)[WM_NCCALCSIZE] = &BaseDialog::OnNcCalcSize;
    (*map)[WM_NCHITTEST] = &BaseDialog::OnNcHit;
    (*map)[WM_SYSCOMMAND] = &BaseDialog::OnSysCommand;
    return map;
}
  
// ���ڴ���ʱ��
HRESULT BaseDialog::OnCreate( WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
    // ��ȡ��ǰ���ڷ��
    LONG styleValue = ::GetWindowLong(*this, GWL_STYLE);
    styleValue &= ~WS_CAPTION;
    // ����STYLE
    ::SetWindowLong(*this, GWL_STYLE, styleValue | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
    // ��ʼ��������Ⱦ��
	m_paintMgr.Init(m_hWnd);

	CDialogBuilder builder;
    // ͨ��xml�Լ���Ⱦ����Ⱦ����UI
    CControlUI* pRoot = builder.Create(_T("duilib.xml"), (UINT)0, NULL, &m_paintMgr);
    // ���ӽ���UI���Ի�������
	m_paintMgr.AttachDialog(pRoot);
    // ������Ϣ����,��Ϊʵ����INotifyUI�ӿ�
	m_paintMgr.AddNotifier(this);

	// ҵ����س�ʼ��
	Init();

    return 0;
}
  
HRESULT BaseDialog::OnDestory( WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
    // �����˳���Ϣ
    ::PostQuitMessage(0L);
    return 0;
}
  
// ��������
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
  
// �����������ô�ͻᵼ��DUILIB��ͣ����ϵͳ��Ϣ���д���
// ����ϵͳ������ �ƺ�������һ���������
HRESULT BaseDialog::OnNcCalcSize( WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
    return 0;
}
  
HRESULT BaseDialog::OnNcHit( WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
    return HTCLIENT;
}
  
// ϵͳ�����
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

// ���ڳ�ʼ������
void BaseDialog::Init() {
	CPUID* hdInfoTool = new CPUID();

	// ��ȡCPU���к�
	// CString cpuSn = hdInfoTool->GetCpuSerialNumber();
	CString cpuSn = hdInfoTool->GetCpuSn();
	// ��ȡӲ�����к�
	CString hdSn = hdInfoTool->getHDSerialNum();
	// ��ȡӲ���ͺ�
	CString hdMod = hdInfoTool->getHDModelNum();
	// ��ȡ·��Mac
	CString routeMac = hdInfoTool->getRouteMac();
	// ��ȡ����Mac
	CString localMac = hdInfoTool->GetMacInfo();

	// Ӳ�����к�
	CTextUI* pTextHdSn = (CTextUI*)m_paintMgr.FindControl(_T("editHdSn"));
	pTextHdSn->SetText(hdSn);

	// Ӳ���ͺ�
	CTextUI* pTextHdMod = (CTextUI*)m_paintMgr.FindControl(_T("editHdMod"));
	pTextHdMod->SetText(hdMod);

	// CPU���к�
	CTextUI* pTextCpuId = (CTextUI*)m_paintMgr.FindControl(_T("editCPUID"));
	pTextCpuId->SetText(cpuSn);

	// ·����MAC
	CTextUI* pTextRouteMac = (CTextUI*)m_paintMgr.FindControl(_T("editRouteMac"));
	pTextRouteMac->SetText(routeMac);

	// ����MAC
	CTextUI* pTextLocalMac = (CTextUI*)m_paintMgr.FindControl(_T("editLocalMac"));
	pTextLocalMac->SetText(localMac);
}