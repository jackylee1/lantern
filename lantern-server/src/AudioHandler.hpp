//
//  AudioHandler.hpp
//  lantern
//
//  Created by Eric Miller on 2/12/17.
//  Copyright © 2017 patternleaf LLC. All rights reserved.
//

#ifndef AudioHandler_hpp
#define AudioHandler_hpp

#include "RaopService.hpp"

class AudioHandler {
public:
	virtual void handleAudioStreamStarted();
	virtual void handleAudio(RaopService::AudioBuffer buffer, float volume) = 0;
	virtual void handleAudioStreamEnded();
	
	// in the event multiple consumers have different window requirements ...
//	virtual int getWindowSize() = 0;
};

#endif /* AudioHandler_hpp */
