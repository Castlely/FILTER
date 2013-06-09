//=============================================================================================
/*
	�ļ���		ProtocolInfo.cpp

	˵��:
	---------------------------------------------------
		TCP/IP ������������� TCP/IP Э��ķ���ṹ��
	���ػ�����ݣ���ȡ��Ҫ�����ݡ�
	---------------------------------------------------

	���̣�		Xfilter ���˷���ǽ
	���ߣ�		����ԣ������
	�������ڣ�	2001/08/21
	��ַ��		http://www.xfilt.com
	�����ʼ���	xstudio@xfilt.com
	��Ȩ���� (c) 2001-2002 X ������

	����:
	---------------------------------------------------
		�����Գ���������Ȩ���ı�����δ����Ȩ������ʹ��
	���޸ı�����ȫ���򲿷�Դ���롣�����Ը��ơ����û�ɢ
	���˳���򲿷ֳ�������������κ�ԽȨ��Ϊ�����⵽��
	���⳥�����µĴ�������������������̷�����׷�ߡ�
	
		��ͨ���Ϸ�;����������Դ������û���Ĭ����Ȩ
	�������Լ��ĳ�����ʹ�ñ������Ĳ��ִ��룬�����߲���
	����������κκ������
	
		ʹ���˱���������ĳ���ֻ���Կ�ִ���ļ���ʽ������
	δ���ر����ɣ����ܽ����б�����Դ�����Դ�������κ�
	��ʽ������
	---------------------------------------------------	
*/
//=============================================================================================

#include "stdafx.h"
#include "ProtocolInfo.h"
#include "CheckAcl.h"

extern CRITICAL_SECTION		gCriticalSection;

//=============================================================================================
// exported public function.

int	CProtocolInfo::GetProtocolInfo(SESSION *session, TCHAR *pBuf, int nBufLenth, BOOL IsSend)
{
	if (pBuf == NULL || nBufLenth == 0)
		return XERR_PROTOCOL_NO_DATA;

	if(IsSend)
		return GetFromSend(session, pBuf, nBufLenth);
	else
		return GetFromRecv(session, pBuf, nBufLenth);

	return XERR_SUCCESS;
}

//=============================================================================================
// private function.

int CProtocolInfo::GetFromSend(SESSION *session, TCHAR *pBuf, int nBufLenth)
{
	if(session->bProtocol == ACL_SERVICE_TYPE_HTTP)
		return GetHttp(session, pBuf, nBufLenth);
	else if(session->bProtocol == ACL_SERVICE_TYPE_FTP)
		return GetFtp(session, pBuf, nBufLenth);
	else if(session->bProtocol == ACL_SERVICE_TYPE_SMTP)
		return GetSmtp(session, pBuf, nBufLenth);
	else if(session->bProtocol == ACL_SERVICE_TYPE_POP3)
		return GetPop3BySend(session, pBuf, nBufLenth);

	return XERR_SUCCESS;
}

int CProtocolInfo::GetFromRecv(SESSION *session, TCHAR *pBuf, int nBufLenth)
{
	if(session->bProtocol == ACL_SERVICE_TYPE_POP3)
		return GetPop3(session, pBuf, nBufLenth);

	return XERR_SUCCESS;
}

int CProtocolInfo::GetHttp(SESSION *session, TCHAR *pBuf, int nBufLenth)
{
	CString tmp, sHost = _T("");

	tmp.Format("%s",pBuf);

	int i = tmp.Find(_T("Host: "));

	if(i != -1)
	{
		int j = i + 6;
		TCHAR c = tmp.GetAt(j);

		while(c != 13)
		{
			sHost += c;
			c = tmp.GetAt(++j);
		}

		if(session->sMemo[0] == '\0')
		{
			if(sHost.GetLength() >= MAX_PATH - 1)
				sHost.SetAt(MAX_PATH - 1, '\0');

			EnterCriticalSection(&gCriticalSection);
			{
				_tcscpy(session->sMemo, sHost);
			}
			LeaveCriticalSection(&gCriticalSection);
		}
	}

	return XERR_SUCCESS;
}

int CProtocolInfo::GetFtp(SESSION *session, TCHAR *pBuf, int nBufLenth)
{
	TCHAR tBuf[MAX_PATH];
	CString tmpStr;
	static TCHAR sUserName[21];
	static TCHAR sPassword[21];
	
	if(_tcsnicmp(pBuf, _T("USER "), 5) == 0)
		_stscanf(pBuf + 4, _T("%*[ ]%s"), sUserName);
	else if(_tcsnicmp(pBuf, _T("PASS "), 5) == 0)
		_stscanf(pBuf + 4, _T("%*[ ]%s"), sPassword);
	else if(_tcsnicmp(pBuf, _T("RETR "), 5) == 0)
	{
		_stscanf(pBuf + 4, _T("%*[ ]%s"), tBuf);
		tmpStr.Format(_T("User Name: %s; Password: %s; Get File: %s"), sUserName, sPassword, tBuf);

		if(tmpStr.GetLength() >= MAX_PATH - 1)
			tmpStr.SetAt(MAX_PATH - 1, '\0');

		EnterCriticalSection(&gCriticalSection);
		{
			_tcscpy(session->sMemo, tmpStr);
		}
		LeaveCriticalSection(&gCriticalSection);

		CCheckAcl::SendSessionToApp(session);
	}
	else if(_tcsnicmp(pBuf, _T("STOR "), 5) == 0)
	{
		_stscanf(pBuf + 4, _T("%*[ ]%s"), tBuf);
		tmpStr.Format(_T("User Name: %s; Password: %s; Put File: %s"), sUserName, sPassword, tBuf);

		if(tmpStr.GetLength() >= MAX_PATH - 1)
			tmpStr.SetAt(MAX_PATH - 1, '\0');

		EnterCriticalSection(&gCriticalSection);
		{
			_tcscpy(session->sMemo, tmpStr);
		}
		LeaveCriticalSection(&gCriticalSection);

		CCheckAcl::SendSessionToApp(session);
	}

	return XERR_SUCCESS;
}

int CProtocolInfo::GetSmtp(SESSION *session, TCHAR *pBuf, int nBufLenth)
{
	static CString sEmail = _T("");

	if(_tcsnicmp(pBuf, _T("MAIL FROM: "), 11) == 0)
		sEmail.Format(_T("%s"), pBuf);
	else if(_tcsnicmp(pBuf, _T("RCPT TO: "), 9) == 0)
	{
		sEmail += pBuf;
		sEmail.Replace(13, ';');
		sEmail.Replace(10, ' ');
		if(sEmail.GetLength() >= MAX_PATH - 1)
			sEmail.SetAt(MAX_PATH - 1, '\0');

		EnterCriticalSection(&gCriticalSection);
		{
			_tcscpy(session->sMemo, sEmail);
		}
		LeaveCriticalSection(&gCriticalSection);
	}

	return XERR_SUCCESS;
}

int CProtocolInfo::GetPop3BySend(SESSION *session, TCHAR *pBuf, int nBufLenth)
{
	CString tmpStr = session->sMemo;
	
	int i = tmpStr.Find(_T("USER "));
	int j = tmpStr.Find(_T("PASS "));
	if(( i != -1) && (j != -1))
		return XERR_SUCCESS;

	if(_tcsnicmp(pBuf, _T("USER "), 5) == 0
		|| _tcsnicmp(pBuf, _T("PASS "), 5) == 0)
	{
		tmpStr += pBuf;
		tmpStr.Replace(13, ';');
		tmpStr.Replace(10, ' ');

		if(tmpStr.GetLength() >= MAX_PATH - 1)
			tmpStr.SetAt(MAX_PATH - 1, '\0');

		EnterCriticalSection(&gCriticalSection);
		{
			_tcscpy(session->sMemo, tmpStr);
		}
		LeaveCriticalSection(&gCriticalSection);
	}

	return XERR_SUCCESS;
}

int CProtocolInfo::GetPop3(SESSION *session, TCHAR *pBuf, int nBufLenth)
{
	CString		tmpStr;
	int			iStart, iOver, iTmp;
	
	tmpStr = session->sMemo;
	iTmp = tmpStr.Find(_T("To: "));
	if(iTmp != -1)
		return XERR_SUCCESS;

	tmpStr = pBuf;
	iStart = tmpStr.Find(_T("From: "));

	if(iStart == -1)
		return XERR_SUCCESS;

	iOver = tmpStr.Find(_T("Subject: "));
		
	if(iOver == -1)
		return XERR_SUCCESS;

	tmpStr = tmpStr.Mid(iStart, iOver - iStart);

	tmpStr.Replace(13, ';');
	tmpStr.Replace(10, ' ');

	int i = MAX_PATH - 1 - _tcslen(session->sMemo);

	if(tmpStr.GetLength() >= i)
		tmpStr.SetAt(i, '\0');

	EnterCriticalSection(&gCriticalSection);
	{
		_tcscat(session->sMemo, tmpStr);
	}
	LeaveCriticalSection(&gCriticalSection);

	return XERR_SUCCESS;
}