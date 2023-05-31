
#include "ws2tcpip.h"
#include "windows.h"
#include "tchar.h"
#include "CSHServiceEstablishServer.h"
#include "CSHServiceLogger.h"
#include "wincrypt.h"
#include "atlstr.h"


#ifndef _UNICODE
	#include "stdio.h"
#endif


typedef struct _SOCKET_DATA_SET
{
	SOCKET clientSocket;


	SOCKADDR_IN clientAddress;


	INT clientAddressLength;


	CHAR clientBuffer[4096];


	WSABUF clientBufferPointer;


	DWORD dwClientBuffer;


	DWORD dwFlag;


	OVERLAPPED overLapped;

} SOCKETDATASET;


DWORD TranslasteMessage(VOID *lpVoid)
{
	DWORD returnCode = 1;


	HANDLE hSharedIOCPort = *((HANDLE *)lpVoid);


	SOCKETDATASET sharedSocketDataSet;

	memset(&sharedSocketDataSet, NULL, sizeof(sharedSocketDataSet));


	DWORD dwBytesTransferred = NULL;


	while (TRUE)
	{
		GetQueuedCompletionStatus(hSharedIOCPort, &dwBytesTransferred, (PULONG_PTR)&sharedSocketDataSet, (LPOVERLAPPED *)&sharedSocketDataSet.overLapped, INFINITE);


		if (dwBytesTransferred == NULL)
		{
			closesocket(sharedSocketDataSet.clientSocket);


			continue;
		} 


		BYTE bitScanner = sharedSocketDataSet.clientBufferPointer.buf[0];


		if (bitScanner == 136)
		{
			closesocket(sharedSocketDataSet.clientSocket);


			memset(&sharedSocketDataSet, NULL, sizeof(sharedSocketDataSet));


			break;
		}


		if (bitScanner != 129)
		{
			break;
		}


		bitScanner = sharedSocketDataSet.clientBufferPointer.buf[1];


		BYTE sizeProtocol = bitScanner & 0x7F;


		INT payLoadSize = NULL;


		INT maskOffSet = NULL;


		if (sizeProtocol <= 125)
		{
			payLoadSize = sizeProtocol;


			maskOffSet = 2;
		}
		else break;


		BYTE mask[4];

		memcpy_s(mask, 4, sharedSocketDataSet.clientBufferPointer.buf + maskOffSet, 4);


		CHAR *unMaskedData = (CHAR *)calloc(1, payLoadSize + 1);


		memcpy_s(unMaskedData, payLoadSize, (sharedSocketDataSet.clientBufferPointer.buf + maskOffSet + 4), payLoadSize);


		for (INT index = NULL; index < payLoadSize; index++)
		{
			unMaskedData[index] = (unMaskedData[index] ^ mask[index % 4]);
		}

		TCHAR UTF8Conveter[4096];

		memset(UTF8Conveter, NULL, sizeof(UTF8Conveter));


		INT unMaskedDatalength = MultiByteToWideChar(CP_ACP, NULL, unMaskedData, -1, NULL, NULL);


		MultiByteToWideChar(CP_ACP, NULL, unMaskedData, -1, UTF8Conveter, unMaskedDatalength);


		INT unicodeLength = WideCharToMultiByte(CP_UTF8, NULL, UTF8Conveter, -1, NULL, NULL, NULL, NULL);


		WideCharToMultiByte(CP_UTF8, NULL, UTF8Conveter, -1,  unMaskedData, unicodeLength, NULL, NULL);


		WSASend(sharedSocketDataSet.clientSocket, &sharedSocketDataSet.clientBufferPointer, 1, &sharedSocketDataSet.dwClientBuffer, sharedSocketDataSet.dwFlag, &sharedSocketDataSet.overLapped, NULL);


		WSARecv(sharedSocketDataSet.clientSocket, &sharedSocketDataSet.clientBufferPointer, 1, &sharedSocketDataSet.dwClientBuffer, &sharedSocketDataSet.dwFlag, &sharedSocketDataSet.overLapped, NULL);
	}


	return returnCode;
}


DWORD WebsocketHandshaking(VOID *lpVoid) 
{
	DWORD returnCode = 1;


	SOCKETDATASET *pSocketDataSet = (SOCKETDATASET *)lpVoid;


	if (recv(pSocketDataSet->clientSocket, pSocketDataSet->clientBuffer, 4096, NULL) == SOCKET_ERROR) 
	{
		returnCode = WSAGetLastError();


		CSHServiceLogger(_T("[%s:%d] recv Failed, Current WSAErrorCode = %ld"), _T(__FUNCTION__), __LINE__, returnCode);


		return returnCode;
	}


	CString acceptedMessageBuffer(pSocketDataSet->clientBuffer);


	INT startPosition = acceptedMessageBuffer.Find(_T("Sec-WebSocket-Key:"));


	INT endPosition = acceptedMessageBuffer.Find(_T("\r\n"), startPosition);


	CString acceptedKeyBuffer(acceptedMessageBuffer.Mid(startPosition, (endPosition - startPosition)));


	startPosition = acceptedKeyBuffer.Find(_T(":"));


	endPosition = acceptedKeyBuffer.GetLength() - startPosition;


	CString acceptedKey(acceptedKeyBuffer.Mid((startPosition + 2), endPosition));


	acceptedKey.Append(_T("258EAFA5-E914-47DA-95CA-C5AB0DC85B11"));


	HCRYPTPROV hCryptProv;


	HCRYPTHASH hCryptHash;


	CryptAcquireContext(&hCryptProv, NULL, MS_ENHANCED_PROV, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT);


	CryptCreateHash(hCryptProv, CALG_SHA1, NULL, NULL, &hCryptHash);


	TCHAR encodedData[MAX_PATH];

	memset(encodedData, NULL, sizeof(encodedData));


	_tcscpy_s(encodedData, MAX_PATH - 1, acceptedKey);


	CryptHashData(hCryptHash, (BYTE *)encodedData, acceptedKey.GetLength(), NULL);


	acceptedKey.ReleaseBuffer();


	BYTE bHashData[8];

	memset(bHashData, NULL, sizeof(bHashData));


	DWORD bHashDataLength = sizeof(DWORD);


	CryptGetHashParam(hCryptHash, HP_HASHSIZE, bHashData, &bHashDataLength, NULL);


	DWORD encodedDataLength = sizeof(encodedData);


	CryptGetHashParam(hCryptHash, HP_HASHVAL, (BYTE *)encodedData, &encodedDataLength, NULL);


	CryptDestroyHash(hCryptHash);


	CryptReleaseContext(hCryptProv, NULL);


	DWORD dwBasedCode = NULL;


	CryptBinaryToString((BYTE *)encodedData, encodedDataLength, CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, NULL, &dwBasedCode);


	TCHAR basedCode[MAX_PATH];

	memset(basedCode, NULL, sizeof(basedCode));


	CryptBinaryToString((BYTE *)basedCode, encodedDataLength, CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, basedCode, &dwBasedCode);


	TCHAR handShakingMessage[4096];

	memset(handShakingMessage, NULL, sizeof(handShakingMessage));


	_stprintf_s(handShakingMessage, _T("HTTP/1.1 101 Switching Protocols\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: %s\r\nUpgrade: websocket\r\n\r\n"), basedCode);


	CStringA converter(handShakingMessage);


	if (send(pSocketDataSet->clientSocket, converter.GetBuffer(), converter.GetLength(), NULL) == SOCKET_ERROR) 
	{
		returnCode = WSAGetLastError();


		CSHServiceLogger(_T("[%s:%d] send Failed, Current WSAErrorCode = %ld"), _T(__FUNCTION__), __LINE__, returnCode);
	}


	converter.ReleaseBuffer();


	return returnCode;
}


DWORD CreateCSHSocketServer(VOID *lpVoid)
{
	DWORD returnCode = 1;


	WSADATA wsaData;

	memset(&wsaData, NULL, sizeof(wsaData));


	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != NO_ERROR)
	{
		returnCode = WSAGetLastError();


		CSHServiceLogger(_T("[%s:%d] WSAStartup Failed, Current WSAErrorCode = %ld"), _T(__FUNCTION__), __LINE__, returnCode);


		return returnCode;
	}


	SOCKET wsaSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, NULL, WSA_FLAG_OVERLAPPED);


	if (wsaSocket == INVALID_SOCKET)
	{
		returnCode = WSAGetLastError();


		CSHServiceLogger(_T("[%s:%d] WSASocket Failed, Current WSAErrorCode = %ld"), _T(__FUNCTION__), __LINE__, returnCode);


		WSACleanup();


		return returnCode;
	}


	SOCKADDR_IN serverAddress;

	memset(&serverAddress, NULL, sizeof(serverAddress));


	serverAddress.sin_family = AF_INET;


	CONST USHORT port = 9090;


	serverAddress.sin_port = htons(port);


	CONST TCHAR *ipv4 = _T("127.0.0.1");


	InetPton(AF_INET, ipv4, &(serverAddress.sin_addr));


	if (bind(wsaSocket, (SOCKADDR *)(&serverAddress), sizeof(serverAddress)) == SOCKET_ERROR)
	{
		returnCode = WSAGetLastError();


		CSHServiceLogger(_T("[%s:%d] bind Failed, Current WSAErrorCode = %ld"), _T(__FUNCTION__), __LINE__, returnCode);


		closesocket(wsaSocket);


		WSACleanup();


		return returnCode;
	}


	if (listen(wsaSocket, SOMAXCONN) == SOCKET_ERROR)
	{
		returnCode = WSAGetLastError();


		CSHServiceLogger(_T("[%s:%d] listen Failed, Current ErrorCode = %ld"), _T(__FUNCTION__), __LINE__, returnCode);


		closesocket(wsaSocket);


		WSACleanup();


		return returnCode;
	}


	HANDLE hIOCPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, NULL);


	if (hIOCPort != NULL)
	{
		CSHServiceLogger(_T("[%s:%d] CreateCSHSocketServer Done, Keep Waiting For Accept to Connect WebSocket or HTTP Communication [%s : %ld]"), _T(__FUNCTION__), __LINE__, ipv4, port);
	}


	SYSTEM_INFO systemInfomation;

	memset(&systemInfomation, NULL, sizeof(systemInfomation));


	GetSystemInfo(&systemInfomation);


	for (INT index = NULL; index < (systemInfomation.dwNumberOfProcessors * 2); index++) 
	{
		CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)TranslasteMessage, &hIOCPort, NULL, NULL);
	}


	while (TRUE) 
	{
		SOCKETDATASET currentConnection;

		memset(&currentConnection, NULL, sizeof(currentConnection));


		currentConnection.clientBufferPointer.buf = currentConnection.clientBuffer;


		currentConnection.clientBufferPointer.len = 4096;


		currentConnection.clientAddressLength = sizeof(currentConnection.clientAddress);


		currentConnection.clientSocket = WSAAccept(wsaSocket, (sockaddr *)&currentConnection.clientAddress, &currentConnection.clientAddressLength, NULL, NULL);


		if (currentConnection.clientSocket != INVALID_SOCKET)
		{
			HANDLE hHandShakingThread = CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)WebsocketHandshaking, &currentConnection, NULL, NULL);


			if (hHandShakingThread != NULL)
			{
				if (WAIT_OBJECT_0 == WaitForSingleObject(hHandShakingThread, 2000))
				{
					CreateIoCompletionPort((HANDLE)&currentConnection.clientSocket, hIOCPort, (ULONG_PTR)&currentConnection, NULL);


					WSARecv(currentConnection.clientSocket, &currentConnection.clientBufferPointer, 1, &currentConnection.dwClientBuffer, &currentConnection.dwFlag, &currentConnection.overLapped, NULL);
				}
				else
				{
					if (currentConnection.clientSocket != NULL)
					{
						closesocket(currentConnection.clientSocket);
					}
				}
			}
			else
			{
				if (currentConnection.clientSocket != NULL)
				{
					closesocket(currentConnection.clientSocket);
				}
			}
		}
		else 
		{
			CSHServiceLogger(_T("[%s:%d] WSAAccept Failed, Current WSAErrorCode = %ld"), _T(__FUNCTION__), __LINE__, WSAGetLastError());
		}
	}


	closesocket(wsaSocket);


	WSACleanup();


	return returnCode;
}
