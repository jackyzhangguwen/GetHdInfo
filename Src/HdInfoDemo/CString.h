#include <windows.h>
#include <assert.h>
#include <stdlib.h>
#include <malloc.h>
#include <tchar.h>

#ifndef __JONES__STRING__
#define __JONES__STRING__

struct CStringData
{
	// ���ü���
	long nRefs;
	// �ַ�ʹ�ó���
	int nDataLength;
	// ���䳤��
	int nAllocLength;
					  
	// ����ַ����ĵط� 
	// this+1 �൱����CStringData[1];
	// ����TCHAR* data()ָ����CStringData[1]�ĵ�ַ
	TCHAR* data() { return (TCHAR*)(this + 1); }
};

class CString
{
public:
	// ���캯��
	CString();
	CString(const CString& stringSrc);
	CString(TCHAR ch, int nLength = 1);
	CString(LPCTSTR lpsz);
	CString(LPCTSTR lpch, int nLength);
	// CString(const unsigned char* psz);

	~CString();
	// CStringData������
	// �õ��ַ�����
	int GetLength() const; 
	// �õ�������ڴ泤��
	int GetAllocLength() const;
	// �ж��ַ������Ƿ�Ϊ0
	BOOL IsEmpty() const;
	// ����ת��
	operator LPCTSTR() const;
	// ���CStringData
	// ����������
	void Empty();
	const CString& operator=(const CString& stringSrc);
	const CString& operator=(LPCTSTR lpsz);
	const CString& operator=(TCHAR ch);
	const CString& operator+=(const CString& string);
	const CString& operator+=(TCHAR ch);
	const CString& operator+=(LPCTSTR lpsz);
	TCHAR operator[](int nIndex) const;
	friend CString operator+(const CString& string1, const CString& string2);
	// friend CString operator+(const CString& string, TCHAR ch);
	// friend CString operator+(TCHAR ch, const CString& string);
	friend CString operator+(const CString& string, LPCTSTR lpsz);
	friend CString operator+(LPCTSTR lpsz, const CString& string);
	// ����,���빲�����ݿ�
	// ɾ����nIndex��ʼ����ΪnCount������
	int Delete(int nIndex, int nCount = 1);
	// ����һ���ַ�
	int Insert(int nIndex, TCHAR ch);
	// ����һ���ַ���
	int Insert(int nIndex, LPCTSTR pstr);
	// �滻����
	int Replace(LPCTSTR lpszOld, LPCTSTR lpszNew);
	// �滻����
	int Replace(TCHAR chOld, TCHAR chNew);
	// �Ƴ�һ���ַ�
	int Remove(TCHAR chRemove);
	void TrimRight(LPCTSTR lpszTargetList);
	// ȥ���ұ�chTarget
	void TrimRight(TCHAR chTarget);
	// ȥ���ұ߿ո�
	void TrimRight();
	void TrimLeft(LPCTSTR lpszTargets);
	// ȥ�����chTarget
	void TrimLeft(TCHAR chTarget);
	// ȥ����߿ո�
	// ȡĳ���ַ���
	void TrimLeft();
	void SetAt(int nIndex, TCHAR ch);
	TCHAR GetAt(int nIndex) const;
	// ȡĳ���ַ���
	CString Mid(int nFirst) const;
	// ȡĳ���ַ���
	CString Mid(int nFirst, int nCount) const;
	// ȡ�ұ��ַ���
	CString Right(int nCount) const;
	// ȡ����ַ���
	CString Left(int nCount) const;
	// ��д
	void CString::MakeUpper();
	// Сд
	void CString::MakeLower();
	// ����
	void CString::MakeReverse();
	int Find(TCHAR ch) const;
	int Find(TCHAR ch, int nStart) const;
	int ReverseFind(TCHAR ch) const;
	int Find(LPCTSTR lpszSub) const;
	int Find(LPCTSTR lpszSub, int nStart) const;
	// �õ���һ��ƥ��lpszCharSet������һ���ַ���λ�� ����_tcspbrk
	// �߼�����
	int FindOneOf(LPCTSTR lpszCharSet) const;
	// ���·����ڴ�,�ڿ���ԭ��������
	LPTSTR GetBuffer(int nMinBufLength);
	// ʹ[nNewLength]='/0',���ڴ��Сû�иı�
	void ReleaseBuffer(int nNewLength = -1);
	// ���·����ڴ�,�ٿ���ԭ��������
	LPTSTR GetBufferSetLength(int nNewLength);
	// ����Լ�,Ȼ��--ԭ�������ü�����
	void FreeExtra();
	// ���ü�����=-1,����
	LPTSTR LockBuffer();
	// ����,���ü�����=1
	void UnlockBuffer();
						 
	// �Ƚ�
	// ���ִ�Сд�Ƚ�
	int Compare(LPCTSTR lpsz) const;
	// �����ִ�Сд�Ƚ�
	// �Ƚ��ٶ�û��Compare��
	int CompareNoCase(LPCTSTR lpsz) const;
	// ���ִ�Сд�Ƚ�
	// �����ִ�Сд�Ƚ�
	// ��ʽ���ַ���
	int Collate(LPCTSTR lpsz) const;
	int CollateNoCase(LPCTSTR lpsz) const; 
										  
    // CSting����ĺ�����,��ȫ���Լ�������(ţ��)
	void Format(LPCTSTR lpszFormat, ...);
private:
	void Init();
	// ͨ��m_pchData-1 �õ�CStringData
	CStringData* GetData() const;
	// ��CStringData�����ڴ�,����������
	void AllocBuffer(int nLen);
	// �����ü����ĸ����Լ����
	void CopyBeforeWrite();
	// ��CStringData�����ڴ�,��������
	void AllocBeforeWrite(int nLen);
	// �����ڴ�,������lpszSrcData����
	// ��nCopyIndex��ʼ��nCopyLen���ȵ����ݿ�����dest,
	// nExtraLen����ĳ���,�˺�������û��
	void AssignCopy(int nSrcLen, LPCTSTR lpszSrcData);
	void AllocCopy(CString& dest, int nCopyLen, int nCopyIndex, int nExtraLen) const;
	// --���ü��������ж��Ƿ�ɾ���ڴ�,��ɾ������ʼ��
	void Release();
	// ��ʽ���ַ���
	void FormatV(LPCTSTR lpszFormat, va_list argList);
	// ��������lpszSrc1Data+lpszSrc2Data
	void ConcatCopy(int nSrc1Len, LPCTSTR lpszSrc1Data, int nSrc2Len, LPCTSTR lpszSrc2Data);
	// �����ַ���
	void ConcatInPlace(int nSrcLen, LPCTSTR lpszSrcData);
	// --���ü��������ж��Ƿ�ɾ���ڴ�
	static void  Release(CStringData* pData);
	// �ͷ��ڴ�
	static void FreeData(CStringData* pData);
	// �õ�����
	static int SafeStrlen(LPCTSTR lpsz);
	// ָ��CStringData��������
	LPTSTR m_pchData;
};

//
// ����CString::Compare�Ƚϴ�С,����Ƚ�����CStirng�Ļ���
// ����operator LPCTSTR()ת������ΪLPCTSTR
//
bool operator==(const CString& s1, const CString& s2);
bool operator==(const CString& s1, LPCTSTR s2);
bool operator==(LPCTSTR s1, const CString& s2);
bool operator!=(const CString& s1, const CString& s2);
bool operator!=(const CString& s1, LPCTSTR s2);
bool operator!=(LPCTSTR s1, const CString& s2);
bool operator<(const CString& s1, const CString& s2);
bool operator<(const CString& s1, LPCTSTR s2);
bool operator<(LPCTSTR s1, const CString& s2);
bool operator>(const CString& s1, const CString& s2);
bool operator>(const CString& s1, LPCTSTR s2);
bool operator>(LPCTSTR s1, const CString& s2);
bool operator<=(const CString& s1, const CString& s2);
bool operator<=(const CString& s1, LPCTSTR s2);
bool operator<=(LPCTSTR s1, const CString& s2);
bool operator>=(const CString& s1, const CString& s2);
bool operator>=(const CString& s1, LPCTSTR s2);
bool operator>=(LPCTSTR s1, const CString& s2);
//////////////////////////////////////////////////////////////////////
// ���lpsz�Ƿ���Ч,������IsBadStringPtr
BOOL AfxIsValidString(LPCTSTR lpsz, int nLength = -1);
// ���lp�Ƿ��ܶ�дȨ��,������IsBadReadPtr,IsBadStringPtr
BOOL AfxIsValidAddress(const void* lp, UINT nBytes, BOOL bReadWrite = TRUE);
// CStirng�������
// ��ʼ��CStirng����
void ConstructElements(CString* pElements, int nCount);
// ɾ��CStirng����
void DestructElements(CString* pElements, int nCount);
// CString���鿽��
void CopyElements(CString* pDest, const CString* pSrc, int nCount);

#endif
