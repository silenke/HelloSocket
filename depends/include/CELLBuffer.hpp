#ifndef _CELL_BUFFER_HPP_
#define _CELL_BUFFER_HPP_


#include "Cell.hpp"


class CELLBuffer
{
public:
	CELLBuffer(int nSize = 8192)
	{
		_nSize = nSize;
		_pBuff = new char[_nSize];
	}

	~CELLBuffer()
	{
		if (_pBuff)
		{
			delete[] _pBuff;
			_pBuff = nullptr;
		}
	}

	char* data()
	{
		return _pBuff;
	}

	bool push(const char* pData, int nLen)
	{
		//if (_nLast + nLen > _nSize)
		//{	
		//	// 写入大量数据不一定放到内存中
		//	// 也可以写入数据库或者磁盘中
		//	int n = _nLast + nLen - _nSize;
		//	if (n < 8192) n = 8192;
		//	char* pBuff = new char[_nSize + n];
		//	memcpy(pBuff, _pBuff, _nLast);
		//	delete[] _pBuff;
		//	_pBuff = pBuff;
		//	_nSize = _nSize + n;
		//}

		if (_nLast + nLen <= _nSize)
		{
			memcpy(_pBuff + _nLast, pData, nLen);
			_nLast += nLen;

			if (_nLast == _nSize)
			{
				_nBuffFullCount++;
			}

			return true;
		}

		_nBuffFullCount++;
		return false;
	}

	void pop(int nLen)
	{
		int n = _nLast - nLen;
		if (n > 0)
		{
			memcpy(_pBuff, _pBuff + nLen, n);
		}
		_nLast = n;
		if (_nBuffFullCount > 0)
		{
			_nBuffFullCount--;
		}
	}

	int write2socket(SOCKET sockfd)
	{
		int ret = 0;
		if (_nLast > 0 && INVALID_SOCKET != sockfd)
		{
			ret = send(sockfd, _pBuff, _nLast, 0);
			_nLast = 0;
			_nBuffFullCount = 0;
		}
		return ret;
	}

	int read4socket(SOCKET sockfd)
	{
		if (_nLast < _nSize)
		{
			char* szRecv = _pBuff + _nLast;
			int nLen = recv(sockfd, szRecv, _nSize - _nLast, 0);
			if (nLen <= 0)
			{
				return nLen;
			}
			_nLast += nLen;
			return nLen;
		}
		return 0;
	}

	bool hasMsg()
	{
		if (_nLast >= sizeof(netmsg_DataHeader))
		{
			netmsg_DataHeader* header = (netmsg_DataHeader*)_pBuff;
			return _nLast >= header->len;
		}
		return false;
	}

	bool needWrite()
	{
		return _nLast > 0;
	}

private:
	char* _pBuff = nullptr;	// 发送缓冲区
	int _nLast = 0;	// 缓冲区尾部位置
	int _nSize;		// 缓冲区大小
	int _nBuffFullCount = 0;	// 缓冲区写满计数
};

#endif // !_CELL_BUFFER_HPP_
