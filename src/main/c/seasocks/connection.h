#ifndef _SEASOCKS_CONNECTION_H_
#define _SEASOCKS_CONNECTION_H_

#include "websocket.h"
#include "seasocks/ssoauthenticator.h"
#include "seasocks/ResponseCode.h"

#include <inttypes.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <boost/shared_ptr.hpp>
#include <string>
#include <list>
#include <vector>

namespace SeaSocks {

class Logger;
class Server;
class PageRequest;
class Response;

class Connection : public WebSocket {
public:
	Connection(
			boost::shared_ptr<Logger> logger,
			Server* server,
			int fd,
			const sockaddr_in& address,
			boost::shared_ptr<SsoAuthenticator> sso);
	virtual ~Connection();

	bool write(const void* data, size_t size, bool flush);
	void handleDataReadyForRead();
	void handleDataReadyForWrite();

	int getFd() const { return _fd; }

	// From WebSocket.
	virtual void send(const char* webSocketResponse);
	virtual void close();

	// From Request.
	virtual boost::shared_ptr<Credentials> credentials() const;
	virtual const sockaddr_in& getRemoteAddress() const { return _address; }
	virtual const std::string& getRequestUri() const { return _requestUri; }
	virtual Request::Verb verb() const { return Request::WebSocket; }
	// Is this all such a good idea?
	virtual size_t contentLength() const { return 0; }
	virtual const uint8_t* content() const { return NULL; }
	// TODO: At the very least should implement these properly for WS:
	virtual bool hasHeader(const std::string&) const { return false; }
	virtual std::string getHeader(const std::string&) const { return std::string(); }

	void setLinger();

	size_t inputBufferSize() const { return _inBuf.size(); }
	size_t outputBufferSize() const { return _outBuf.size(); }

	size_t bytesReceived() const { return _bytesReceived; }
	size_t bytesSent() const { return _bytesSent; }

	// For testing:
	std::vector<uint8_t>& getInputBuffer() { return _inBuf; }
	void handleHixieWebSocket();
	void handleHybiWebSocket();
	void setHandler(boost::shared_ptr<WebSocket::Handler> handler) {
		_webSocketHandler = handler;
	}

private:
	void finalise();
	bool closed() const;

	void closeWhenEmpty();
	void closeInternal();

	void handleNewData();
	void handleHeaders();
	void handleWebSocketKey3();
	void handleWebSocketMessage(const char* message);
	void handleBufferingPostData();
	bool handlePageRequest();

	bool bufferLine(const char* line);
	bool bufferLine(const std::string& line);
	bool flush();

	bool handleHybiHandshake(int webSocketVersion, const std::string& webSocketKey);

	// Send an error document. Returns 'true' for convenience in handle*() routines.
	bool sendError(ResponseCode errorCode, const std::string& document);

	// Send individual errors. Again all return true for convenience.
	bool sendUnsupportedError(const std::string& reason);
	bool send404(const std::string& path);
	bool sendBadRequest(const std::string& reason);
	bool sendISE(const std::string& error);

	void sendHybi(int opcode, const char* webSocketResponse, size_t messageLength);

	bool sendResponse(boost::shared_ptr<Response> response);

	bool processHeaders(uint8_t* first, uint8_t* last);
	bool sendData(const std::string& type, const char* start, size_t size);

	struct Range {
		long start;
		long end;
		size_t length() const { return end - start + 1; }
	};

	bool parseRange(const std::string& rangeStr, Range& range) const;
	bool parseRanges(const std::string& range, std::list<Range>& ranges) const;
	bool sendStaticData(const char* requestUri, const std::string& rangeHeader);

	int safeSend(const void* data, size_t size);

	void bufferResponseAndCommonHeaders(ResponseCode code);

	std::list<Range> processRangesForStaticData(const std::list<Range>& ranges, long fileSize);

	boost::shared_ptr<Logger> _logger;
	Server* _server;
	int _fd;
	bool _shutdown;
    bool _hadSendError;
	bool _closeOnEmpty;
	bool _registeredForWriteEvents;
	sockaddr_in _address;
	uint32_t _webSocketKeys[2];
	size_t _bytesSent;
	size_t _bytesReceived;
	std::vector<uint8_t> _inBuf;
	std::vector<uint8_t> _outBuf;
	// Populated only during Hixie web socket header parsing.
	std::string _hixieExtraHeaders;
	boost::shared_ptr<WebSocket::Handler> _webSocketHandler;
	boost::shared_ptr<SsoAuthenticator> _sso;
	std::string _requestUri;
	time_t _connectionTime;
	bool _shutdownByUser;
	std::unique_ptr<PageRequest> _request;

	enum State {
		INVALID,
		READING_HEADERS,
		READING_WEBSOCKET_KEY3,
		HANDLING_HIXIE_WEBSOCKET,
		HANDLING_HYBI_WEBSOCKET,
		BUFFERING_POST_DATA,
	};
	State _state;

	Connection(Connection& other) = delete;
	Connection& operator =(Connection& other) = delete;
};

}  // namespace SeaSocks

#endif  // _SEASOCKS_CONNECTION_H_
