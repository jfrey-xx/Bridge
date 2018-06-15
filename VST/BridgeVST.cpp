#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsuggest-override"
#include "public.sdk/source/vst2.x/audioeffectx.h"
#pragma GCC diagnostic pop


#include "../common/client.cpp"


struct BridgeVST : public AudioEffectX {
	BridgeClient *client;
	bool lastPlaying = false;

	BridgeVST(audioMasterCallback audioMaster) : AudioEffectX(audioMaster, 0, 2 + BRIDGE_NUM_PARAMS) {
		isSynth(true);
		setNumInputs(BRIDGE_INPUTS);
		setNumOutputs(BRIDGE_OUTPUTS);

		setUniqueID('VCVB');
		canProcessReplacing();
		noTail(false);
		client = new BridgeClient();
	}

	~BridgeVST() {
		delete client;
	}

	void processReplacing(float **inputs, float **outputs, VstInt32 sampleFrames) override {
		VstTimeInfo *timeInfo = getTimeInfo(0);
		// log("%f %x", timeInfo->ppqPos, timeInfo->flags);
		// if (timeInfo->flags & kVstPpqPosValid == 0)
		// 	log("Host does not set PPQ in VstTimeInfo. Clocks and transport may be incorrect.")

		// MIDI transport
		bool playing = (timeInfo->flags & kVstTransportPlaying) != 0;
		if (playing && !lastPlaying) {
			if (timeInfo->ppqPos == 0.0)
				client->pushStart();
			client->pushContinue();
		}
		if (!playing && lastPlaying) {
			client->pushStop();
		}
		lastPlaying = playing;

		// MIDI clock
		if (playing) {
			double timeDuration = sampleFrames / timeInfo->sampleRate;
			double ppqDuration = timeDuration * (timeInfo->tempo / 60.0) * 24;
			int ppqStart = (int) ceil(timeInfo->ppqPos * 24);
			int ppqEnd = (int) ceil(timeInfo->ppqPos * 24 + ppqDuration);
			for (int ppqIndex = ppqStart; ppqIndex < ppqEnd; ppqIndex++) {
				client->pushClock();
			}
		}

		// Interleave samples
		float input[BRIDGE_INPUTS * sampleFrames];
		float output[BRIDGE_OUTPUTS * sampleFrames];
		for (int i = 0; i < sampleFrames; i++) {
			for (int c = 0; c < BRIDGE_INPUTS; c++) {
				input[BRIDGE_INPUTS*i + c] = inputs[c][i];
			}
		}
		// Process audio
		client->processStream(input, output, sampleFrames);
		// Uninterleave samples
		for (int i = 0; i < sampleFrames; i++) {
			// To prevent the DAW from pausing the processReplacing() calls, add a noise floor so the DAW thinks audio is still being processed.
			float r = (float) rand() / RAND_MAX;
			r = 1.f - 2.f * r;
			// Ableton Live's threshold is 1e-5 or -100dB
			r *= 1.5e-5f; // -96dB
			for (int c = 0; c < BRIDGE_OUTPUTS; c++) {
				outputs[c][i] = output[BRIDGE_OUTPUTS*i + c] + r;
			}
		}
	}

	void setParameter(VstInt32 index, float value) override {
		if (index == 0) {
			client->setTCPPort((int) roundf(TCP_PORT_MIN + value * (TCP_PORT_RANGE)));
		}
		if (index == 1) {
			client->setPort((int) roundf(value * (BRIDGE_NUM_PARAMS - 1.f)));
		}
		else if (index > 1) {
			client->setParam(index - 2, value);
		}
	}

	float getParameter(VstInt32 index) override {
		if (index == 0) {
			return (client->getTCPPort() - TCP_PORT_MIN) / (float) (TCP_PORT_RANGE);
		}
		else if (index == 1) {
			return client->getPort() / (BRIDGE_NUM_PARAMS - 1.f);
		}
		else if (index > 1) {
			return client->getParam(index - 2);
		}
		return 0.f;
	}

	void getParameterLabel(VstInt32 index, char *label) override {
		snprintf(label, kVstMaxParamStrLen, "");
	}

	void getParameterDisplay(VstInt32 index, char *text) override {
		if (index == 0) {
			snprintf(text, kVstMaxParamStrLen, "%d", client->getTCPPort());
		}
		else if (index == 1) {
			snprintf(text, kVstMaxParamStrLen, "%d", client->getPort() + 1);
		}
		else if (index > 1) {
			snprintf(text, kVstMaxParamStrLen, "%0.2f V", 10.f * client->getParam(index - 2));
		}
	}

	void getParameterName(VstInt32 index, char *text) override {
		if (index == 0) {
			// TCP Port selector
			snprintf(text, kVstMaxParamStrLen, "TCP Port");
		}
		else if (index == 1) {
			// Brige Port selector
			snprintf(text, kVstMaxParamStrLen, "Port");
		}
		else if (index > 1) {
			// Automation parameters
			snprintf(text, kVstMaxParamStrLen, "CC %d", index - 2);
		}
	}

	bool getEffectName(char *name) override {
		snprintf(name, kVstMaxEffectNameLen, "VCV Bridge");
		return true;
	}

	bool getVendorString(char *text) override {
		snprintf(text, kVstMaxProductStrLen, "VCV");
		return true;
	}

	bool getProductString(char *text) override {
		snprintf(text, kVstMaxVendorStrLen, "VCV Bridge");
		return true;
	}

	VstInt32 getVendorVersion() override {
		return 0;
	}

	void open() override {
	}
	void close() override {
	}
	void suspend() override {
	}
	void resume() override {
	}

	void setSampleRate(float sampleRate) override {
		AudioEffectX::setSampleRate(sampleRate);
		client->setSampleRate((int) sampleRate);
	}

	VstInt32 processEvents(VstEvents *events) override {
		for (int i = 0; i < events->numEvents; i++) {
			VstEvent *event = events->events[i];
			if (event->type == kVstMidiType) {
				VstMidiEvent *midiEvent = (VstMidiEvent*) event;
				MidiMessage msg;
				msg.cmd = midiEvent->midiData[0];
				msg.data1 = midiEvent->midiData[1];
				msg.data2 = midiEvent->midiData[2];
				// log("%02x %02x %02x", msg.cmd, msg.data1, msg.data2);
				client->pushMidi(msg);
			}
		}
		return 0;
	}
};


AudioEffect *createEffectInstance (audioMasterCallback audioMaster) {
	return new BridgeVST(audioMaster);
}
