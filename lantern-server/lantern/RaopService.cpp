//
//  RaopService.cpp
//  lantern
//
//  Adapted from shairplay: https://github.com/juhovh/shairplay
//
//  Created by Eric Miller on 2/12/17.
//  Copyright © 2017 patternleaf LLC.
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
// CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
// SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//




#include <iostream>

#include "RaopService.hpp"
#include "AudioHandler.hpp"

#include <math.h>

using namespace std;
using namespace std::chrono;

static raop_callbacks_s sCallbacks;

#define RSA_KEY \
"-----BEGIN RSA PRIVATE KEY-----\n"\
"MIIEpQIBAAKCAQEA59dE8qLieItsH1WgjrcFRKj6eUWqi+bGLOX1HL3U3GhC/j0Qg90u3sG/1CUt\n"\
"wC5vOYvfDmFI6oSFXi5ELabWJmT2dKHzBJKa3k9ok+8t9ucRqMd6DZHJ2YCCLlDRKSKv6kDqnw4U\n"\
"wPdpOMXziC/AMj3Z/lUVX1G7WSHCAWKf1zNS1eLvqr+boEjXuBOitnZ/bDzPHrTOZz0Dew0uowxf\n"\
"/+sG+NCK3eQJVxqcaJ/vEHKIVd2M+5qL71yJQ+87X6oV3eaYvt3zWZYD6z5vYTcrtij2VZ9Zmni/\n"\
"UAaHqn9JdsBWLUEpVviYnhimNVvYFZeCXg/IdTQ+x4IRdiXNv5hEewIDAQABAoIBAQDl8Axy9XfW\n"\
"BLmkzkEiqoSwF0PsmVrPzH9KsnwLGH+QZlvjWd8SWYGN7u1507HvhF5N3drJoVU3O14nDY4TFQAa\n"\
"LlJ9VM35AApXaLyY1ERrN7u9ALKd2LUwYhM7Km539O4yUFYikE2nIPscEsA5ltpxOgUGCY7b7ez5\n"\
"NtD6nL1ZKauw7aNXmVAvmJTcuPxWmoktF3gDJKK2wxZuNGcJE0uFQEG4Z3BrWP7yoNuSK3dii2jm\n"\
"lpPHr0O/KnPQtzI3eguhe0TwUem/eYSdyzMyVx/YpwkzwtYL3sR5k0o9rKQLtvLzfAqdBxBurciz\n"\
"aaA/L0HIgAmOit1GJA2saMxTVPNhAoGBAPfgv1oeZxgxmotiCcMXFEQEWflzhWYTsXrhUIuz5jFu\n"\
"a39GLS99ZEErhLdrwj8rDDViRVJ5skOp9zFvlYAHs0xh92ji1E7V/ysnKBfsMrPkk5KSKPrnjndM\n"\
"oPdevWnVkgJ5jxFuNgxkOLMuG9i53B4yMvDTCRiIPMQ++N2iLDaRAoGBAO9v//mU8eVkQaoANf0Z\n"\
"oMjW8CN4xwWA2cSEIHkd9AfFkftuv8oyLDCG3ZAf0vrhrrtkrfa7ef+AUb69DNggq4mHQAYBp7L+\n"\
"k5DKzJrKuO0r+R0YbY9pZD1+/g9dVt91d6LQNepUE/yY2PP5CNoFmjedpLHMOPFdVgqDzDFxU8hL\n"\
"AoGBANDrr7xAJbqBjHVwIzQ4To9pb4BNeqDndk5Qe7fT3+/H1njGaC0/rXE0Qb7q5ySgnsCb3DvA\n"\
"cJyRM9SJ7OKlGt0FMSdJD5KG0XPIpAVNwgpXXH5MDJg09KHeh0kXo+QA6viFBi21y340NonnEfdf\n"\
"54PX4ZGS/Xac1UK+pLkBB+zRAoGAf0AY3H3qKS2lMEI4bzEFoHeK3G895pDaK3TFBVmD7fV0Zhov\n"\
"17fegFPMwOII8MisYm9ZfT2Z0s5Ro3s5rkt+nvLAdfC/PYPKzTLalpGSwomSNYJcB9HNMlmhkGzc\n"\
"1JnLYT4iyUyx6pcZBmCd8bD0iwY/FzcgNDaUmbX9+XDvRA0CgYEAkE7pIPlE71qvfJQgoA9em0gI\n"\
"LAuE4Pu13aKiJnfft7hIjbK+5kyb3TysZvoyDnb3HOKvInK7vXbKuU4ISgxB2bB3HcYzQMGsz1qJ\n"\
"2gG0N5hvJpzwwhbhXqFKA4zaaSrw622wDniAK5MlIE0tIAKKP4yxNGjoD2QYjhBGuhvkWKY=\n"\
"-----END RSA PRIVATE KEY-----\n"


RaopService* RaopService::sService = nullptr;

RaopService* RaopService::shared()
{
	if (sService == nullptr) {
		sService = new RaopService();
	}
	return sService;
}

// MARK: -- raop audio handling

using time_stamp = std::chrono::time_point<std::chrono::system_clock, std::chrono::microseconds>;

typedef struct AudioSession {
	tthread::thread* consumerThread;
	mutex mutex;
	time_stamp lastCycleTime;
//	duration<microseconds> processPeriod;	// ??
	
	bool isRunning = true;
	int sampleSize;
	int channels;
	int sampleRate;
	
	int windowSize;
	
	float volume;

	RaopService::AudioBuffer buffer;
	
	RaopService* service;
} AudioSession;

// tthread func

void consumerThreadFunc(void* ctx)
{
	AudioSession* session = (AudioSession*)ctx;
	RaopService* service = session->service;
	cout << "session " << (session->isRunning ? "is running" : "is not running") << endl;
	while (session->isRunning) {
		size_t bufferSize = 0;
		{
			lock_guard<mutex> guard(session->mutex);
			bufferSize = session->buffer.size();
		}
		
//		tthread::this_thread::sleep_for(tthread::chrono::microseconds());
//		auto latency = system_clock::now() - session->lastCycleTime;
		
//		if (latency > duration_values<microseconds>::zero()) {
		if (bufferSize < session->windowSize) {
			tthread::this_thread::sleep_for(tthread::chrono::milliseconds(1));
		}
		else {
			lock_guard<mutex> guard(session->mutex);
			service->handleAudio(session->buffer, session->volume);
			session->buffer.clear();
		}
		
//		session->lastCycleTime = time_point_cast<microseconds>(system_clock::now());
	}
}

void* raop_service_audio_init(void *cls, int bits, int channels, int samplerate)
{
	RaopService* service = (RaopService*)cls;
	AudioSession* session = new AudioSession;
	
	session->service = (RaopService*)cls;
	session->sampleSize = bits;
	session->channels = channels;
	session->sampleRate = samplerate;
	session->volume = 0;
	session->buffer.set_capacity((bits / 8) * samplerate * channels);	// one second of buffer ?
	
	session->windowSize = floor(((bits / 8) * samplerate * channels) * 0.01);
	
	// ??
//	auto windowPeriod = seconds(1 / samplerate) * 256;
//	session->processPeriod = microseconds(windowPeriod);
	
	service->mSession = session;
	
	cout << "initing audio for " << bits << "-bit " << samplerate << " sample rate" << endl;
	
	session->consumerThread = new tthread::thread(&consumerThreadFunc, session);
	
	service->handleAudioStarted();
	
	return session;
}

void raop_service_audio_process(void *cls, void *session, const void *buffer, int buflen)
{
//	RaopService* service = (RaopService*)cls;
	AudioSession* audioSession = (AudioSession*)session;
	
	lock_guard<mutex> guard(audioSession->mutex);
	
	for (uint i = 0; i < buflen; i++) {
		audioSession->buffer.push_back(((uint8_t*)buffer)[i]);
	}
}

void raop_service_audio_destroy(void *cls, void *session)
{
	RaopService* service = (RaopService*)cls;
	AudioSession* audioSession = (AudioSession*)session;
	
	audioSession->buffer.clear();
	audioSession->isRunning = false;
	audioSession->consumerThread->join();
	delete audioSession->consumerThread;
	
	service->mSession = nullptr;
	service->handleAudioEnded();
	delete audioSession;
}

void raop_service_audio_flush(void *cls, void *session)
{
	RaopService* service = (RaopService*)cls;
	AudioSession* audioSession = (AudioSession*)session;
	audioSession->buffer.clear();
	service->handleAudioEnded();
}

void raop_service_audio_set_volume(void *cls, void *session, float volume)
{
	((AudioSession*)session)->volume = volume;
}

void raop_service_raop_log_callback(void *cls, int level, const char *msg)
{
//	cout << "[raop] " << msg << endl;
}

// MARK: RaopService

RaopService::RaopService()
: mSession(nullptr)
{
	
	sCallbacks.cls = this;
	sCallbacks.audio_init = &raop_service_audio_init;
	sCallbacks.audio_process = &raop_service_audio_process;
	sCallbacks.audio_flush = &raop_service_audio_flush;
	sCallbacks.audio_set_volume = &raop_service_audio_set_volume;
	sCallbacks.audio_destroy = &raop_service_audio_destroy;
	sCallbacks.audio_remote_control_id = nullptr;
	sCallbacks.audio_set_coverart = nullptr;
	sCallbacks.audio_set_metadata = nullptr;
	sCallbacks.audio_set_progress = nullptr;
	
	int error = 0;
	
	mRaopRef = raop_init(10, &sCallbacks, RSA_KEY, &error);
	
	if (error || !mRaopRef) {
		// TODO: throw on error
		cerr << "Error initializing raop: " << error << endl;
	}
	
	raop_set_log_level(mRaopRef, RAOP_LOG_DEBUG);
	raop_set_log_callback(mRaopRef, &raop_service_raop_log_callback, this);
	
	mDnssdRef = dnssd_init(&error);
	
	if (error || !mDnssdRef) {
		// TODO: throw on error
		cerr << "Error initializing dnssd: " << error << endl;
	}
	

}

void RaopService::expose(string serverName, unsigned short port)
{
	unsigned char chwaddr[] = { 0x01, 0x23, 0x45, 0x67, 0x89, 0xAB };
	raop_start(mRaopRef, &port, (char*)chwaddr, sizeof(chwaddr), nullptr);
	dnssd_register_raop(mDnssdRef, serverName.c_str(), port, (char*)chwaddr, sizeof(chwaddr), 0);
}

bool RaopService::isExposed()
{
	return raop_is_running(mRaopRef);
}

void RaopService::hide()
{
	if (raop_is_running(mRaopRef)) {
		dnssd_unregister_raop(mDnssdRef);
		raop_stop(mRaopRef);
	}
}

void RaopService::registerAudioHandler(AudioHandler* handler)
{
	mAudioHandlers.insert(handler);
}

void RaopService::unregisterAudioHandler(AudioHandler* handler)
{
	mAudioHandlers.erase(handler);
}

int RaopService::getSampleSize()
{
	if (mSession) {
		return mSession->sampleSize;
	}
	// TODO: throw?
	return 0;
}

int RaopService::getSampleRate()
{
	if (mSession) {
		return mSession->sampleRate;
	}
	// TODO: throw?
	return 0;
}

void RaopService::handleAudio(AudioBuffer buffer, float volume)
{
	for (auto handler : mAudioHandlers) {
		handler->handleAudio(buffer, volume);
	}
	
}

void RaopService::handleAudioStarted()
{
	for (auto handler : mAudioHandlers) {
		handler->handleAudioStreamStarted();
	}
}

void RaopService::handleAudioEnded()
{
	for (auto handler : mAudioHandlers) {
		handler->handleAudioStreamEnded();
	}
}
