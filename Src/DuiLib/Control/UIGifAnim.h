#ifndef GifAnimUI_h__
#define GifAnimUI_h__

#pragma once

namespace DuiLib
{
	class CControl;

#define EVENT_TIEM_ID	100

	class DUILIB_API CGifAnimUI : public CControlUI
	{
	public:
		CGifAnimUI(void);
		~CGifAnimUI(void);

		LPCTSTR	GetClass() const;
		LPVOID	GetInterface(LPCTSTR pstrName);
		void	DoInit() override;
		bool	DoPaint(HDC hDC, const RECT& rcPaint, CControlUI* pStopControl);
		void	DoEvent(TEventUI& event);
		void	SetVisible(bool bVisible = true );
		void	SetAttribute(LPCTSTR pstrName, LPCTSTR pstrValue);
		void	SetBkImage(LPCTSTR pStrImage);
		LPCTSTR GetBkImage();

		void	SetAutoPlay(bool bIsAuto = true );
		bool	IsAutoPlay() const;
		void	SetAutoSize(bool bIsAuto = true );
		bool	IsAutoSize() const;
		void	PlayGif();
		void	PauseGif();
		void	StopGif();

	private:
		void	InitGifImage();
		void	DeleteGif();
		void    OnTimer( UINT_PTR idEvent );
		// ����GIFÿ֡
		void	DrawFrame( HDC hDC );
		Gdiplus::Image*	LoadGifFromFile(LPCTSTR pstrGifPath);
		Gdiplus::Image* LoadGifFromMemory( LPVOID pBuf,size_t dwSize );
	private:
		Gdiplus::Image	*m_pGifImage;
		// gifͼƬ��֡��
		UINT			m_nFrameCount;
		// ��ǰ�ŵ��ڼ�֡
		UINT			m_nFramePosition;
		// ֡��֮֡����ʱ��
		Gdiplus::PropertyItem*	m_pPropertyItem;

		CDuiString		m_sBkImage;
		// �Ƿ��Զ�����gif
		bool			m_bIsAutoPlay;
		// �Ƿ��Զ�����ͼƬ���ô�С
		bool			m_bIsAutoSize;
		bool			m_bIsPlaying;
	};
}

#endif // GifAnimUI_h__
