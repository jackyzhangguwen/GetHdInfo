//  #include "stdafx.h"
#include "CString.h"

TCHAR afxChNil = '/0';
// 初始化CStringData的地址
int _afxInitData[] = { -1, 0, 0, 0 }; 
// 地址转化为CStringData*
CStringData* _afxDataNil = (CStringData*)&_afxInitData;
LPCTSTR _afxPchNil = (LPCTSTR)(((BYTE*)&_afxInitData) + sizeof(CStringData));

// 建立一个空的CString
const CString&  AfxGetEmptyString()
{
	return *(CString*)&_afxPchNil;
}

BOOL AfxIsValidString(LPCTSTR lpsz, int nLength /* = -1 */)
{
	if (lpsz == NULL) { return FALSE; }
	return ::IsBadStringPtr(lpsz, nLength) == 0;
}

BOOL AfxIsValidAddress(const void* lp,
	UINT nBytes, 
	BOOL bReadWrite /* = TRUE */)
{
	return (lp != NULL && 
		!IsBadReadPtr(lp, nBytes) &&
		(!bReadWrite || !IsBadWritePtr((LPVOID)lp, nBytes)));
}

void CString::Init()
{
	m_pchData = AfxGetEmptyString().m_pchData;
}

CString::CString()
{
	Init();
}

int CString::GetLength() const
{
	return GetData()->nDataLength;
}

int CString::GetAllocLength() const
{
	return GetData()->nAllocLength;
}

BOOL CString::IsEmpty() const
{
	return GetData()->nDataLength == 0;
}

CStringData* CString::GetData() const
{
	assert(m_pchData != NULL);
	return ((CStringData*)m_pchData) - 1;
}

CString::operator LPCTSTR() const
{
	return m_pchData;
}

int CString::SafeStrlen(LPCTSTR lpsz)
{
	return (lpsz == NULL) ? 0 : lstrlen(lpsz);
}

void CString::AllocBuffer(int nLen)
{
	assert(nLen >= 0);
	//  (signed) int 的最大值
	assert(nLen <= 2147483647 - 1);
	if (nLen == 0) {
		Init();
	} else
	{
		CStringData* pData;
		{
			pData = (CStringData*)
				new BYTE[sizeof(CStringData) + (nLen + 1) * sizeof(TCHAR)];
			pData->nAllocLength = nLen;
		}
		pData->nRefs = 1;
		pData->data()[nLen] = '/0';
		pData->nDataLength = nLen;
		m_pchData = pData->data();
	}
}

void CString::FreeData(CStringData* pData)
{
	delete[](BYTE*)pData;
}

void CString::CopyBeforeWrite()
{
	if (GetData()->nRefs > 1)
	{
		CStringData* pData = GetData();
		Release();
		AllocBuffer(pData->nDataLength);
		memcpy(m_pchData, pData->data(), (pData->nDataLength + 1) * sizeof(TCHAR));
	}
	assert(GetData()->nRefs <= 1);
}

void CString::AllocBeforeWrite(int nLen)
{
	if (GetData()->nRefs > 1 || nLen > GetData()->nAllocLength)
	{
		Release();
		AllocBuffer(nLen);
	}
	assert(GetData()->nRefs <= 1);
}

void CString::AssignCopy(int nSrcLen, LPCTSTR lpszSrcData)
{
	AllocBeforeWrite(nSrcLen);
	memcpy(m_pchData, lpszSrcData, nSrcLen * sizeof(TCHAR));
	GetData()->nDataLength = nSrcLen;
	m_pchData[nSrcLen] = '/0';
}

void CString::AllocCopy(CString& dest, 
	int nCopyLen, 
	int nCopyIndex,
	int nExtraLen) const
{
	int nNewLen = nCopyLen + nExtraLen;
	if (nNewLen == 0)
	{
		dest.Init();
	}
	else
	{
		dest.AllocBuffer(nNewLen);
		memcpy(dest.m_pchData, m_pchData + nCopyIndex, nCopyLen * sizeof(TCHAR));
	}
}

CString::~CString()
{
	if (GetData() != _afxDataNil) {
		if (InterlockedDecrement(&GetData()->nRefs) <= 0) {
			FreeData(GetData());
		}
	}
}

CString::CString(TCHAR ch, int nLength)
{
	Init();
	int nLen = 1;
	if (nLen != 0)
	{
		AllocBuffer(nLen);
		memcpy(m_pchData, &ch, nLen * sizeof(TCHAR));
	}
}

CString::CString(const CString& stringSrc)
{
	assert(stringSrc.GetData()->nRefs != 0);
	if (stringSrc.GetData()->nRefs >= 0) {
		assert(stringSrc.GetData() != _afxDataNil);
		m_pchData = stringSrc.m_pchData;
		InterlockedIncrement(&GetData()->nRefs);
	} else {
		Init();
		*this = stringSrc.m_pchData;
	}
}

CString::CString(LPCTSTR lpsz)
{
	Init();
	int nLen = SafeStrlen(lpsz);
	if (nLen != 0)
	{
		AllocBuffer(nLen);
		memcpy(m_pchData, lpsz, nLen * sizeof(TCHAR));
	}
}

CString::CString(LPCTSTR lpch, int nLength)
{
	Init();
	if (nLength != 0)
	{
		assert(AfxIsValidAddress(lpch, nLength, FALSE));
		AllocBuffer(nLength);
		memcpy(m_pchData, lpch, nLength * sizeof(TCHAR));
	}
}

void CString::Release()
{
	if (GetData() != _afxDataNil)
	{
		assert(GetData()->nRefs != 0);
		if (InterlockedDecrement(&GetData()->nRefs) <= 0) {
			FreeData(GetData());
		}
		Init();
	}
}

void CString::Release(CStringData* pData)
{
	if (pData != _afxDataNil)
	{
		assert(pData->nRefs != 0);
		if (InterlockedDecrement(&pData->nRefs) <= 0) {
			FreeData(pData);
		}
	}
}

void CString::Empty()
{
	if (GetData()->nDataLength == 0) { return; }
	if (GetData()->nRefs >= 0) {
		Release();
	} else {
		*this = &afxChNil;
	}
	assert(GetData()->nDataLength == 0);
	assert(GetData()->nRefs < 0 || GetData()->nAllocLength == 0);
}

const CString& CString::operator=(const CString& stringSrc)
{
	if (m_pchData != stringSrc.m_pchData)
	{
		if ((GetData()->nRefs < 0 && GetData() != _afxDataNil) ||
			stringSrc.GetData()->nRefs < 0)
		{
			//  新建一块数据
			AssignCopy(stringSrc.GetData()->nDataLength, stringSrc.m_pchData);
		}
		else
		{
			//  只拷贝指针
			Release();
			assert(stringSrc.GetData() != _afxDataNil);
			m_pchData = stringSrc.m_pchData;
			InterlockedIncrement(&GetData()->nRefs);
		}
	}
	return *this;
}

const CString& CString::operator=(LPCTSTR lpsz)
{
	assert(lpsz == NULL || AfxIsValidString(lpsz));
	AssignCopy(SafeStrlen(lpsz), lpsz);
	return *this;
}

const CString& CString::operator=(TCHAR ch)
{
	AssignCopy(1, &ch);
	return *this;
}

int CString::Delete(int nIndex, int nCount /* = 1 */)
{
	if (nIndex < 0) {
		nIndex = 0;
	}
	int nNewLength = GetData()->nDataLength;
	if (nCount > 0 && nIndex < nNewLength)
	{
		//  脱离共享数据块，
		CopyBeforeWrite();
		int nBytesToCopy = nNewLength - (nIndex + nCount) + 1;
		//  移动数据
		memcpy(m_pchData + nIndex,
			m_pchData + nIndex + nCount, nBytesToCopy * sizeof(TCHAR));
		GetData()->nDataLength = nNewLength - nCount;
	}
	return nNewLength;
}

int CString::Insert(int nIndex, TCHAR ch)
{
	//  脱离共享数据
	CopyBeforeWrite();
	if (nIndex < 0) {
		nIndex = 0;
	}
	int nNewLength = GetData()->nDataLength;
	if (nIndex > nNewLength) {
		nIndex = nNewLength;
	}
	nNewLength++;
	if (GetData()->nAllocLength < nNewLength)
	{ 
		//  动态分配内存,并拷贝原来的数据
		CStringData* pOldData = GetData();
		LPTSTR pstr = m_pchData;
		AllocBuffer(nNewLength);
		memcpy(m_pchData, pstr, (pOldData->nDataLength + 1) * sizeof(TCHAR));
		CString::Release(pOldData);
	}

	//  插入数据
	memcpy(m_pchData + nIndex + 1,
		m_pchData + nIndex, (nNewLength - nIndex) * sizeof(TCHAR));
	m_pchData[nIndex] = ch;
	GetData()->nDataLength = nNewLength;
	return nNewLength;
}

int CString::Insert(int nIndex, LPCTSTR pstr)
{
	if (nIndex < 0) {
		nIndex = 0;
	}
	int nInsertLength = SafeStrlen(pstr);
	int nNewLength = GetData()->nDataLength;
	if (nInsertLength > 0)
	{
		//  脱离共享数据
		CopyBeforeWrite();
		if (nIndex > nNewLength) {
			nIndex = nNewLength;
		}
		nNewLength += nInsertLength;
		if (GetData()->nAllocLength < nNewLength)
		{
			//  动态分配内存,并拷贝原来的数据
			CStringData* pOldData = GetData();
			LPTSTR pstr = m_pchData;
			AllocBuffer(nNewLength);
			memcpy(m_pchData, pstr, (pOldData->nDataLength + 1) * sizeof(TCHAR));
			CString::Release(pOldData);
		}
		//  移动数据，留出插入的位子move也可以
		memcpy(m_pchData + nIndex + nInsertLength,
			m_pchData + nIndex,
			(nNewLength - nIndex - nInsertLength + 1) * sizeof(TCHAR));
		//  插入数据
		memcpy(m_pchData + nIndex,
			pstr, nInsertLength * sizeof(TCHAR));
		GetData()->nDataLength = nNewLength;
	}
	return nNewLength;
}

int CString::Replace(TCHAR chOld, TCHAR chNew)
{
	int nCount = 0;
	if (chOld != chNew)
	{
		//  替换的不能相同
		CopyBeforeWrite();
		LPTSTR psz = m_pchData;
		LPTSTR pszEnd = psz + GetData()->nDataLength;
		while (psz < pszEnd)
		{
			if (*psz == chOld) 
			{
				//  替换
				*psz = chNew;
				nCount++;
			}
			//  相当于++psz,考虑要UNICODE下版本才用的
			psz = _tcsinc(psz);
		}
	}
	return nCount;
}

int CString::Replace(LPCTSTR lpszOld, LPCTSTR lpszNew)
{
	int nSourceLen = SafeStrlen(lpszOld);
	//  要替换的不能为空
	if (nSourceLen == 0) { return 0; }
	int nReplacementLen = SafeStrlen(lpszNew);
	int nCount = 0;
	LPTSTR lpszStart = m_pchData;
	LPTSTR lpszEnd = m_pchData + GetData()->nDataLength;
	LPTSTR lpszTarget;
	//  检索要替换的个数
	while (lpszStart < lpszEnd)
	{
		while ((lpszTarget = _tcsstr(lpszStart, lpszOld)) != NULL)
		{
			nCount++;
			lpszStart = lpszTarget + nSourceLen;
		}
		lpszStart += lstrlen(lpszStart) + 1;
	}

	if (nCount > 0)
	{
		CopyBeforeWrite();
		int nOldLength = GetData()->nDataLength;
		// 替换以后的长度
		int nNewLength = nOldLength + (nReplacementLen - nSourceLen)*nCount; 
		if (GetData()->nAllocLength < nNewLength || GetData()->nRefs > 1)
		{
			// 超出原来的内存长度动态分配
			CStringData* pOldData = GetData();
			LPTSTR pstr = m_pchData;
			AllocBuffer(nNewLength);
			memcpy(m_pchData, pstr, pOldData->nDataLength * sizeof(TCHAR));
			CString::Release(pOldData);
		}

		lpszStart = m_pchData;
		lpszEnd = m_pchData + GetData()->nDataLength;
		// 这个循环好象没什么用
		while (lpszStart < lpszEnd) 
		{
			// 开始替换
			while ((lpszTarget = _tcsstr(lpszStart, lpszOld)) != NULL)
			{
				// 要往后移的长度
				// 移动数据，留出插入的位子
				int nBalance = nOldLength - (lpszTarget - m_pchData + nSourceLen);
				memmove(lpszTarget + nReplacementLen, lpszTarget + nSourceLen,
					nBalance * sizeof(TCHAR));
				// 插入替换数据
				memcpy(lpszTarget, lpszNew, nReplacementLen * sizeof(TCHAR));
				lpszStart = lpszTarget + nReplacementLen;
				lpszStart[nBalance] = '/0';
				// 现有数据长度
				nOldLength += (nReplacementLen - nSourceLen);
			}
			lpszStart += lstrlen(lpszStart) + 1;
		}
		assert(m_pchData[nNewLength] == '/0');
		GetData()->nDataLength = nNewLength;
	}
	return nCount;
}

int CString::Remove(TCHAR chRemove)
{
	CopyBeforeWrite();
	LPTSTR pstrSource = m_pchData;
	LPTSTR pstrDest = m_pchData;
	LPTSTR pstrEnd = m_pchData + GetData()->nDataLength;
	while (pstrSource < pstrEnd)
	{
		if (*pstrSource != chRemove)
		{
			// 把不移除的数据拷贝
			*pstrDest = *pstrSource;
			pstrDest = _tcsinc(pstrDest);
		}
		// ++pstrSource
		pstrSource = _tcsinc(pstrSource);
	}
	*pstrDest = '/0';
	// 比较变态的计算替换个数,
	int nCount = pstrSource - pstrDest;
	GetData()->nDataLength -= nCount;
	return nCount;
}

CString CString::Mid(int nFirst) const
{
	return Mid(nFirst, GetData()->nDataLength - nFirst);
}

CString CString::Mid(int nFirst, int nCount) const
{
	if (nFirst < 0) {
		nFirst = 0;
	}
	if (nCount < 0) {
		nCount = 0;
	}
	if (nFirst + nCount > GetData()->nDataLength) {
		nCount = GetData()->nDataLength - nFirst;
	}
	if (nFirst > GetData()->nDataLength) {
		nCount = 0;
	}
	assert(nFirst >= 0);
	assert(nFirst + nCount <= GetData()->nDataLength);
	// 取去整个数据
	if (nFirst == 0 && 
		nFirst + nCount == GetData()->nDataLength) {
		return *this;
	}

	CString dest;
	AllocCopy(dest, nCount, nFirst, 0);
	return dest;
}

CString CString::Right(int nCount) const
{
	if (nCount < 0) {
		nCount = 0;
	}
	if (nCount >= GetData()->nDataLength) {
		return *this;
	}
	CString dest;
	AllocCopy(dest, nCount, GetData()->nDataLength - nCount, 0);
	return dest;
}

CString CString::Left(int nCount) const
{
	if (nCount < 0) {
		nCount = 0;
	}
	if (nCount >= GetData()->nDataLength) {
		return *this;
	}
	CString dest;
	AllocCopy(dest, nCount, 0, 0);
	return dest;
}

int CString::ReverseFind(TCHAR ch) const
{
	// 从最后查找
	LPTSTR lpsz = _tcsrchr(m_pchData, (_TUCHAR)ch);
	return (lpsz == NULL) ? -1 : (int)(lpsz - m_pchData);
}

int CString::Find(TCHAR ch) const
{
	return Find(ch, 0);
}

int CString::Find(TCHAR ch, int nStart) const
{
	int nLength = GetData()->nDataLength;
	if (nStart >= nLength) { return -1;	}
	LPTSTR lpsz = _tcschr(m_pchData + nStart, (_TUCHAR)ch);
	return (lpsz == NULL) ? -1 : (int)(lpsz - m_pchData);
}

int CString::Find(LPCTSTR lpszSub) const
{
	return Find(lpszSub, 0);
}

int CString::Find(LPCTSTR lpszSub, int nStart) const
{
	assert(AfxIsValidString(lpszSub));
	int nLength = GetData()->nDataLength;
	if (nStart > nLength) { return -1; }
	LPTSTR lpsz = _tcsstr(m_pchData + nStart, lpszSub);
	return (lpsz == NULL) ? -1 : (int)(lpsz - m_pchData);
}

int CString::FindOneOf(LPCTSTR lpszCharSet) const
{
	assert(AfxIsValidString(lpszCharSet));
	LPTSTR lpsz = _tcspbrk(m_pchData, lpszCharSet);
	return (lpsz == NULL) ? -1 : (int)(lpsz - m_pchData);
}

void CString::MakeUpper()
{
	CopyBeforeWrite();
	_tcsupr(m_pchData);
}

void CString::MakeLower()
{
	CopyBeforeWrite();
	_tcslwr(m_pchData);
}

void CString::MakeReverse()
{
	CopyBeforeWrite();
	_tcsrev(m_pchData);
}

void CString::SetAt(int nIndex, TCHAR ch)
{
	assert(nIndex >= 0);
	assert(nIndex < GetData()->nDataLength);
	CopyBeforeWrite();
	m_pchData[nIndex] = ch;
}

void CString::TrimRight(LPCTSTR lpszTargetList)
{
	CopyBeforeWrite();
	LPTSTR lpsz = m_pchData;
	LPTSTR lpszLast = NULL;
	while (*lpsz != '/0')
	{
		if (_tcschr(lpszTargetList, *lpsz) != NULL)
		{
			if (lpszLast == NULL) {
				lpszLast = lpsz;
			}
		} else {
			lpszLast = NULL;
		}
		lpsz = _tcsinc(lpsz);
	}
	if (lpszLast != NULL)
	{
		*lpszLast = '/0';
		GetData()->nDataLength = lpszLast - m_pchData;
	}
}

void CString::TrimRight(TCHAR chTarget)
{
	CopyBeforeWrite();
	LPTSTR lpsz = m_pchData;
	LPTSTR lpszLast = NULL;
	while (*lpsz != '/0')
	{
		if (*lpsz == chTarget)
		{
			if (lpszLast == NULL) {
				lpszLast = lpsz;
			}
		} else {
			lpszLast = NULL;
		}
		lpsz = _tcsinc(lpsz);
	}
	if (lpszLast != NULL)
	{
		*lpszLast = '/0';
		GetData()->nDataLength = lpszLast - m_pchData;
	}
}

void CString::TrimRight()
{
	CopyBeforeWrite();
	LPTSTR lpsz = m_pchData;
	LPTSTR lpszLast = NULL;
	while (*lpsz != '/0')
	{
		if (_istspace(*lpsz))
		{
			if (lpszLast == NULL) {
				lpszLast = lpsz;
			}
		}
		else {
			lpszLast = NULL;
		}
		lpsz = _tcsinc(lpsz);
	}
	if (lpszLast != NULL)
	{
		//  truncate at trailing space start
		*lpszLast = '/0';
		GetData()->nDataLength = lpszLast - m_pchData;
	}
}

void CString::TrimLeft(LPCTSTR lpszTargets)
{
	//  if we're not trimming anything, we're not doing any work
	if (SafeStrlen(lpszTargets) == 0) { return;	}
	CopyBeforeWrite();
	LPCTSTR lpsz = m_pchData;
	while (*lpsz != '/0')
	{
		if (_tcschr(lpszTargets, *lpsz) == NULL) { break; }
		lpsz = _tcsinc(lpsz);
	}
	if (lpsz != m_pchData)
	{
		//  fix up data and length
		int nDataLength = GetData()->nDataLength - (lpsz - m_pchData);
		memmove(m_pchData, lpsz, (nDataLength + 1) * sizeof(TCHAR));
		GetData()->nDataLength = nDataLength;
	}
}

void CString::TrimLeft(TCHAR chTarget)
{
	CopyBeforeWrite();
	LPCTSTR lpsz = m_pchData;
	while (chTarget == *lpsz) {
		lpsz = _tcsinc(lpsz);
	}
	if (lpsz != m_pchData)
	{
		int nDataLength = GetData()->nDataLength - (lpsz - m_pchData);
		memmove(m_pchData, lpsz, (nDataLength + 1) * sizeof(TCHAR));
		GetData()->nDataLength = nDataLength;
	}
}

void CString::TrimLeft()
{
	CopyBeforeWrite();
	LPCTSTR lpsz = m_pchData;
	while (_istspace(*lpsz)) {
		lpsz = _tcsinc(lpsz);
	}
	if (lpsz != m_pchData)
	{
		int nDataLength = GetData()->nDataLength - (lpsz - m_pchData);
		memmove(m_pchData, lpsz, (nDataLength + 1) * sizeof(TCHAR));
		GetData()->nDataLength = nDataLength;
	}
}

#define TCHAR_ARG   TCHAR
#define WCHAR_ARG   WCHAR
#define CHAR_ARG    char
struct _AFX_DOUBLE { BYTE doubleBits[sizeof(double)]; };
#ifdef _X86_
#define DOUBLE_ARG  _AFX_DOUBLE
#else
#define DOUBLE_ARG  double
#endif
#define FORCE_ANSI      0x10000
#define FORCE_UNICODE   0x20000
#define FORCE_INT64     0x40000
void CString::FormatV(LPCTSTR lpszFormat, va_list argList)
{
	assert(AfxIsValidString(lpszFormat));
	va_list argListSave = argList;
	//  make a guess at the maximum length of the resulting string
	int nMaxLen = 0;
	for (LPCTSTR lpsz = lpszFormat; *lpsz != '/0'; lpsz = _tcsinc(lpsz))
	{
		// 查找%,对%%不在查找范围
		if (*lpsz != '%' || *(lpsz = _tcsinc(lpsz)) == '%')
		{
			nMaxLen += _tclen(lpsz);
			continue;
		}
		int nItemLen = 0;
		// %后面的格式判断
		int nWidth = 0;
		for (; *lpsz != '/0'; lpsz = _tcsinc(lpsz))
		{
			if (*lpsz == '#') {
				//  16进制 '0x'
				nMaxLen += 2;
			}
			else if (*lpsz == '*') {
				nWidth = va_arg(argList, int);
			}
			else if (*lpsz == '-' || *lpsz == '+' || *lpsz == '0' || 
				*lpsz == ' ') {
				;
			}
			else {
				//  hit non-flag character
				break;
			}
		}
		//  get width and skip it
		if (nWidth == 0)
		{
			//  width indicated by
			nWidth = _ttoi(lpsz);
			for (; *lpsz != '/0' && _istdigit(*lpsz); lpsz = _tcsinc(lpsz)) {
				;
			}
		}
		assert(nWidth >= 0);
		int nPrecision = 0;
		if (*lpsz == '.')
		{
			//  skip past '.' separator (width.precision)
			lpsz = _tcsinc(lpsz);
			//  get precision and skip it
			if (*lpsz == '*')
			{
				nPrecision = va_arg(argList, int);
				lpsz = _tcsinc(lpsz);
			}
			else
			{
				nPrecision = _ttoi(lpsz);
				for (; *lpsz != '/0' && _istdigit(*lpsz); lpsz = _tcsinc(lpsz))
					;
			}
			assert(nPrecision >= 0);
		}
		//  should be on type modifier or specifier
		int nModifier = 0;
		if (_tcsncmp(lpsz, _T("I64"), 3) == 0)
		{
			lpsz += 3;
			nModifier = FORCE_INT64;
#if !defined(_X86_) && !defined(_ALPHA_)
			//  __int64 is only available on X86 and ALPHA platforms
			ASSERT(FALSE);
#endif
		}
		else
		{
			switch (*lpsz)
			{
				//  modifiers that affect size
			case 'h':
				nModifier = FORCE_ANSI;
				lpsz = _tcsinc(lpsz);
				break;
			case 'l':
				nModifier = FORCE_UNICODE;
				lpsz = _tcsinc(lpsz);
				break;
				//  modifiers that do not affect size
			case 'F':
			case 'N':
			case 'L':
				lpsz = _tcsinc(lpsz);
				break;
			}
		}
		//  now should be on specifier
		switch (*lpsz | nModifier)
		{
			//  single characters
		case 'c':
		case 'C':
			nItemLen = 2;
			va_arg(argList, TCHAR_ARG);
			break;
		case 'c' | FORCE_ANSI:
		case 'C' | FORCE_ANSI:
			nItemLen = 2;
			va_arg(argList, CHAR_ARG);
			break;
		case 'c' | FORCE_UNICODE:
		case 'C' | FORCE_UNICODE:
			nItemLen = 2;
			va_arg(argList, WCHAR_ARG);
			break;
			//  strings
		case 's':
		{
			LPCTSTR pstrNextArg = va_arg(argList, LPCTSTR);
			if (pstrNextArg == NULL) {
				nItemLen = 6;  //  "(null)"
			}
			else
			{
				nItemLen = lstrlen(pstrNextArg);
				nItemLen = max(1, nItemLen);
			}
		}
		break;
		case 'S':
		{
#ifndef _UNICODE
			LPWSTR pstrNextArg = va_arg(argList, LPWSTR);
			if (pstrNextArg == NULL)
				nItemLen = 6;  //  "(null)"
			else
			{
				nItemLen = wcslen(pstrNextArg);
				nItemLen = max(1, nItemLen);
			}
#else
			LPCSTR pstrNextArg = va_arg(argList, LPCSTR);
			if (pstrNextArg == NULL) {
				nItemLen = 6; //  "(null)"
			}
			else
			{
				nItemLen = lstrlenA(pstrNextArg);
				nItemLen = max(1, nItemLen);
			}
#endif
		}
		break;
		case 's' | FORCE_ANSI:
		case 'S' | FORCE_ANSI:
		{
			LPCSTR pstrNextArg = va_arg(argList, LPCSTR);
			if (pstrNextArg == NULL) {
				nItemLen = 6; //  "(null)"
			}
			else
			{
				nItemLen = lstrlenA(pstrNextArg);
				nItemLen = max(1, nItemLen);
			}
		}
		break;
		case 's' | FORCE_UNICODE:
		case 'S' | FORCE_UNICODE:
		{
			LPWSTR pstrNextArg = va_arg(argList, LPWSTR);
			if (pstrNextArg == NULL) {
				//  "(null)"
				nItemLen = 6;
			}
			else
			{
				nItemLen = wcslen(pstrNextArg);
				nItemLen = max(1, nItemLen);
			}
		}
		break;
		}
		//  adjust nItemLen for strings
		if (nItemLen != 0)
		{
			if (nPrecision != 0) {
				nItemLen = min(nItemLen, nPrecision);
			}
			nItemLen = max(nItemLen, nWidth);
		}
		else
		{
			switch (*lpsz)
			{
			//  integers
			case 'd':
			case 'i':
			case 'u':
			case 'x':
			case 'X':
			case 'o':
				if (nModifier & FORCE_INT64) {
					va_arg(argList, __int64);
				}
				else {
					va_arg(argList, int);
				}
				nItemLen = 32;
				nItemLen = max(nItemLen, nWidth + nPrecision);
				break;
			case 'e':
			case 'g':
			case 'G':
				va_arg(argList, DOUBLE_ARG);
				nItemLen = 128;
				nItemLen = max(nItemLen, nWidth + nPrecision);
				break;
			case 'f':
			{
				double f;
				LPTSTR pszTemp;
				//  312 == strlen("-1+(309 zeroes).")
				//  309 zeroes == max precision of a double
				//  6 == adjustment in case precision is not specified,
				//    which means that the precision defaults to 6
				pszTemp = (LPTSTR)_alloca(max(nWidth, 312 + nPrecision + 6));
				f = va_arg(argList, double);
				_stprintf(pszTemp, _T("%*.*f"), nWidth, nPrecision + 6, f);
				nItemLen = _tcslen(pszTemp);
			}
			break;
			case 'p':
				va_arg(argList, void*);
				nItemLen = 32;
				nItemLen = max(nItemLen, nWidth + nPrecision);
				break;
				//  no output
			case 'n':
				va_arg(argList, int*);
				break;
			default:
				assert(FALSE);  //  unknown formatting option
			}
		}
		//  adjust nMaxLen for output nItemLen
		nMaxLen += nItemLen;
	}
	GetBuffer(nMaxLen);
	// VERIFY(_vstprintf(m_pchData, lpszFormat, argListSave) <= GetAllocLength());
	_vstprintf(m_pchData, lpszFormat, argListSave);
	ReleaseBuffer();
	va_end(argListSave);
}

void CString::Format(LPCTSTR lpszFormat, ...)
{
	assert(AfxIsValidString(lpszFormat));
	va_list argList;
	va_start(argList, lpszFormat);
	FormatV(lpszFormat, argList);
	va_end(argList);
}

void CString::ConcatCopy(int nSrc1Len, 
	LPCTSTR lpszSrc1Data, 
	int nSrc2Len, 
	LPCTSTR lpszSrc2Data)
{
	int nNewLen = nSrc1Len + nSrc2Len;
	if (nNewLen != 0)
	{
		AllocBuffer(nNewLen);
		memcpy(m_pchData, lpszSrc1Data, nSrc1Len * sizeof(TCHAR));
		memcpy(m_pchData + nSrc1Len, lpszSrc2Data, nSrc2Len * sizeof(TCHAR));
	}
}

void CString::ConcatInPlace(int nSrcLen, LPCTSTR lpszSrcData)
{
	if (nSrcLen == 0) { return; }

	if (GetData()->nRefs > 1 || 
		GetData()->nDataLength + nSrcLen > GetData()->nAllocLength)
	{
		// 动态分配
		CStringData* pOldData = GetData();
		ConcatCopy(GetData()->nDataLength, m_pchData, nSrcLen, lpszSrcData);
		assert(pOldData != NULL);
		CString::Release(pOldData);
	}
	else
	{
		// 直接往后添加
		memcpy(m_pchData + GetData()->nDataLength, lpszSrcData, nSrcLen * sizeof(TCHAR));
		GetData()->nDataLength += nSrcLen;
		assert(GetData()->nDataLength <= GetData()->nAllocLength);
		m_pchData[GetData()->nDataLength] = '/0';
	}
}

const CString& CString::operator+=(LPCTSTR lpsz)
{
	assert(lpsz == NULL || AfxIsValidString(lpsz));
	ConcatInPlace(SafeStrlen(lpsz), lpsz);
	return *this;
}

const CString& CString::operator+=(TCHAR ch)
{
	ConcatInPlace(1, &ch);
	return *this;
}

const CString& CString::operator+=(const CString& string)
{
	ConcatInPlace(string.GetData()->nDataLength, string.m_pchData);
	return *this;
}

LPTSTR CString::GetBuffer(int nMinBufLength)
{
	assert(nMinBufLength >= 0);
	if (GetData()->nRefs > 1 || 
		nMinBufLength > GetData()->nAllocLength)
	{ 
		// 重新动态分配
		CStringData* pOldData = GetData();
		//  AllocBuffer will tromp it
		int nOldLen = GetData()->nDataLength;
		if (nMinBufLength < nOldLen) {
			nMinBufLength = nOldLen;
		}
		AllocBuffer(nMinBufLength);
		memcpy(m_pchData, pOldData->data(), (nOldLen + 1) * sizeof(TCHAR));
		GetData()->nDataLength = nOldLen;
		CString::Release(pOldData);
	}
	assert(GetData()->nRefs <= 1);
	assert(m_pchData != NULL);
	return m_pchData;
}

void CString::ReleaseBuffer(int nNewLength)
{
	// 脱离共享数据块，
	CopyBeforeWrite();
	if (nNewLength == -1) {
		nNewLength = lstrlen(m_pchData);
		//  zero terminated
	}
	assert(nNewLength <= GetData()->nAllocLength);
	GetData()->nDataLength = nNewLength;
	m_pchData[nNewLength] = '/0';
}

LPTSTR CString::GetBufferSetLength(int nNewLength)
{
	assert(nNewLength >= 0);
	GetBuffer(nNewLength);
	GetData()->nDataLength = nNewLength;
	m_pchData[nNewLength] = '/0';
	return m_pchData;
}

void CString::FreeExtra()
{
	assert(GetData()->nDataLength <= GetData()->nAllocLength);
	if (GetData()->nDataLength != GetData()->nAllocLength)
	{
		CStringData* pOldData = GetData();
		AllocBuffer(GetData()->nDataLength);
		memcpy(m_pchData, pOldData->data(), pOldData->nDataLength * sizeof(TCHAR));
		assert(m_pchData[GetData()->nDataLength] == '/0');
		CString::Release(pOldData);
	}
	assert(GetData() != NULL);
}

LPTSTR CString::LockBuffer()
{
	LPTSTR lpsz = GetBuffer(0);
	GetData()->nRefs = -1;
	return lpsz;
}

void CString::UnlockBuffer()
{
	assert(GetData()->nRefs == -1);
	if (GetData() != _afxDataNil) {
		GetData()->nRefs = 1;
	}
}

int CString::Compare(LPCTSTR lpsz) const
{
	assert(AfxIsValidString(lpsz));
	return _tcscmp(m_pchData, lpsz);
}

int CString::CompareNoCase(LPCTSTR lpsz) const
{
	assert(AfxIsValidString(lpsz));
	return _tcsicmp(m_pchData, lpsz);
}

//  CString::Collate is often slower than Compare but is MBSC/Unicode
//   aware as well as locale-sensitive with respect to sort order.
int CString::Collate(LPCTSTR lpsz) const
{
	assert(AfxIsValidString(lpsz));
	return _tcscoll(m_pchData, lpsz);
}

int CString::CollateNoCase(LPCTSTR lpsz) const
{
	assert(AfxIsValidString(lpsz));
	return _tcsicoll(m_pchData, lpsz);
}

TCHAR CString::GetAt(int nIndex) const
{
	assert(nIndex >= 0);
	assert(nIndex < GetData()->nDataLength);
	return m_pchData[nIndex];
}

TCHAR CString::operator[](int nIndex) const
{
	assert(nIndex >= 0);
	assert(nIndex < GetData()->nDataLength);
	return m_pchData[nIndex];
}

CString operator+(const CString& string1, const CString& string2)
{
	CString s;
	s.ConcatCopy(string1.GetData()->nDataLength, string1.m_pchData,
		string2.GetData()->nDataLength, string2.m_pchData);
	return s;
}

CString operator+(const CString& string, LPCTSTR lpsz)
{
	assert(lpsz == NULL || AfxIsValidString(lpsz));
	CString s;
	s.ConcatCopy(string.GetData()->nDataLength, 
		string.m_pchData,
		CString::SafeStrlen(lpsz), 
		lpsz);
	return s;
}

CString operator+(LPCTSTR lpsz, const CString& string)
{
	assert(lpsz == NULL || AfxIsValidString(lpsz));
	CString s;
	s.ConcatCopy(CString::SafeStrlen(lpsz), 
		lpsz, 
		string.GetData()->nDataLength,
		string.m_pchData);
	return s;
}

bool operator==(const CString& s1, const CString& s2)
{
	return s1.Compare(s2) == 0;
}

bool operator==(const CString& s1, LPCTSTR s2)
{
	return s1.Compare(s2) == 0;
}

bool operator==(LPCTSTR s1, const CString& s2)
{
	return s2.Compare(s1) == 0;
}

bool operator!=(const CString& s1, const CString& s2)
{
	return s1.Compare(s2) != 0;
}

bool operator!=(const CString& s1, LPCTSTR s2)
{
	return s1.Compare(s2) != 0;
}

bool operator!=(LPCTSTR s1, const CString& s2)
{
	return s2.Compare(s1) != 0;
}

bool operator<(const CString& s1, const CString& s2)
{
	return s1.Compare(s2) < 0;
}

bool operator<(const CString& s1, LPCTSTR s2)
{
	return s1.Compare(s2) < 0;
}

bool operator<(LPCTSTR s1, const CString& s2)
{
	return s2.Compare(s1) > 0;
}

bool operator>(const CString& s1, const CString& s2)
{
	return s1.Compare(s2) > 0;
}

bool operator>(const CString& s1, LPCTSTR s2)
{
	return s1.Compare(s2) > 0;
}

bool operator>(LPCTSTR s1, const CString& s2)
{
	return s2.Compare(s1) < 0;
}

bool operator<=(const CString& s1, const CString& s2)
{
	return s1.Compare(s2) <= 0;
}

bool operator<=(const CString& s1, LPCTSTR s2)
{
	return s1.Compare(s2) <= 0;
}

bool operator<=(LPCTSTR s1, const CString& s2)
{
	return s2.Compare(s1) >= 0;
}

bool operator>=(const CString& s1, const CString& s2)
{
	return s1.Compare(s2) >= 0;
}

bool operator>=(const CString& s1, LPCTSTR s2)
{
	return s1.Compare(s2) >= 0;
}

bool operator>=(LPCTSTR s1, const CString& s2)
{
	return s2.Compare(s1) <= 0;
}

void ConstructElements(CString* pElements, int nCount)
{
	assert(nCount == 0 ||
		AfxIsValidAddress(pElements, nCount * sizeof(CString)));
	for (; nCount--; ++pElements) {
		memcpy(pElements, &AfxGetEmptyString(), sizeof(*pElements));
	}
}

void DestructElements(CString* pElements, int nCount)
{
	assert(nCount == 0 ||
		AfxIsValidAddress(pElements, nCount * sizeof(CString)));
	for (; nCount--; ++pElements) {
		pElements->~CString();
	}
}

void CopyElements(CString* pDest, const CString* pSrc, int nCount)
{
	assert(nCount == 0 ||
		AfxIsValidAddress(pDest, nCount * sizeof(CString)));
	assert(nCount == 0 ||
		AfxIsValidAddress(pSrc, nCount * sizeof(CString)));
	for (; nCount--; ++pDest, ++pSrc) {
		*pDest = *pSrc;
	}
}
