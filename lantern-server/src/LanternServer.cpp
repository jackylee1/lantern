//
//  LanternServer.cpp
//  lantern
//
//  Created by Eric Miller on 1/17/17.
//  Copyright © 2017 patternleaf LLC. All rights reserved.
//

#include "LanternServer.hpp"
#include "LanternState.hpp"
#include "lib/json.hpp"

using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::placeholders::_3;
using websocketpp::lib::bind;

using namespace nlohmann;
using namespace std;

LanternServer::LanternServer(int port)
: mPort(port), mIsStopping(false), mBroadcastSleepDuration(100), mState(nullptr)
{
	mServerThread = new tthread::thread(&serverThreadFunc, this);
	mBroadcastThread = new tthread::thread(&broadcastThreadFunc, this);
}

LanternServer::~LanternServer()
{
	mWSServer.stop();
	mIsStopping = true;
	mBroadcastThread->join();
	mServerThread->join();
	delete mServerThread;
	delete mBroadcastThread;
}

void LanternServer::setState(LanternState* state)
{
	mState = state;
}

void LanternServer::onMessage(websocketpp::connection_hdl hdl, WSServer::message_ptr msg)
{
	if (mState) {
		cout << msg->get_payload() << endl;
		json parsedMsg = json::parse(msg->get_payload());
		if (parsedMsg.is_object()) {
			if (parsedMsg["command"] == "setState") {
				mState->setWith(parsedMsg["state"]);
			}
			else if (parsedMsg["command"] == "setBroadcastSleepDuration") {
				mBroadcastSleepDuration = parsedMsg["duration"];
			}
			else if (parsedMsg["command"] == "setFader") {
				int channel = parsedMsg["channel"];
				float value = parsedMsg["value"];
				
				mState->setFader(channel, value);
			}
			else if (parsedMsg["command"] == "setEffect") {
				mState->setEffect(parsedMsg["effectId"], parsedMsg["effect"]);
			}
			else if (parsedMsg["command"] == "sendState") {
				broadcastState();
			}
		}
	}
}

void LanternServer::onOpen(websocketpp::connection_hdl hdl)
{
	lock_guard<mutex> lock(mConnectionMutex);
	mConnections.insert(hdl);
}

void LanternServer::onClose(websocketpp::connection_hdl hdl)
{
	lock_guard<mutex> lock(mConnectionMutex);
	mConnections.erase(hdl);
}

void LanternServer::broadcastState()
{
	if (mState) {
		stringstream ss;
		ss << mState->toJson();
		broadcast(ss.str());
	}
}

void LanternServer::broadcast(const string& message)
{
	lock_guard<mutex> lock(mConnectionMutex);
	
	for (auto it : mConnections) {
		mWSServer.send(it, message, websocketpp::frame::opcode::text);
	}
}

void LanternServer::broadcastThreadFunc(void* ctx)
{
	LanternServer* server = (LanternServer*)ctx;
	
	while (!server->mIsStopping) {
		tthread::this_thread::sleep_for(tthread::chrono::milliseconds(server->mBroadcastSleepDuration));
//		server->broadcastState();
	}
}

void LanternServer::serverThreadFunc(void* ctx)
{
	LanternServer* server = (LanternServer*)ctx;
	
	try {
		// Set logging settings
//		server->mWSServer.set_access_channels(websocketpp::log::alevel::all);
//		server->mWSServer.clear_access_channels(websocketpp::log::alevel::frame_payload);
		
		server->mWSServer.clear_access_channels(websocketpp::log::alevel::all);
		
		// Initialize Asio
		server->mWSServer.init_asio();
		
		// Register our message handler
		server->mWSServer.set_message_handler(
			bind(&LanternServer::onMessage, server, _1, _2)
		);
		
		server->mWSServer.set_open_handler(
			bind(&LanternServer::onOpen, server, _1)
		);
		server->mWSServer.set_close_handler(
			bind(&LanternServer::onClose, server, _1)
		);
		
		// Listen on port 9002
		server->mWSServer.listen(server->mPort);
		
		// Start the server accept loop
		server->mWSServer.start_accept();
		
		// Start the ASIO io_service run loop
		server->mWSServer.run();
		
	} catch (websocketpp::exception const & e) {
		cout << e.what() << endl;
	}
}
