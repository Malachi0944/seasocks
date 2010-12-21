#include "server.h"

/*
 * TODOs:
 * * Add logging
 * * Connection class does *everything*. Work out better division of labour; server should do more, connection should just be connection stuff, handlers for data?
 * * sort out buffers and buffering
 * * work out what state to hang on to; handle multiple WS endpoints
 * * work out threading model for asynch message passing.
 * * Fix bloody SO_REUSEADDR
 */

using namespace SeaSocks;

int main(int argc, const char* argv[]) {
	Server server;
	server.serve("src/web", 9090);
	return 0;
}

