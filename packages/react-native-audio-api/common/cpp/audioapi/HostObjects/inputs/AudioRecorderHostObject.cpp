#include <audioapi/HostObjects/inputs/AudioRecorderHostObject.h>

#include <audioapi/HostObjects/sources/AudioBufferHostObject.h>
#include <audioapi/HostObjects/sources/RecorderAdapterNodeHostObject.h>
#include <audioapi/core/inputs/AudioRecorder.h>
#include <audioapi/core/sources/AudioBuffer.h>
#include <audioapi/events/AudioEventHandlerRegistry.h>
#ifdef ANDROID
#include <audioapi/android/core/AndroidAudioRecorder.h>
#else
#include <audioapi/ios/core/IOSAudioRecorder.h>
#endif

namespace audioapi {

AudioRecorderHostObject::AudioRecorderHostObject(
    const std::shared_ptr<AudioEventHandlerRegistry> &audioEventHandlerRegistry,
    float sampleRate,
    int bufferLength) {
  
  // Validate inputs
  if (!audioEventHandlerRegistry) {
    throw std::runtime_error("AudioEventHandlerRegistry cannot be null");
  }
  
  if (sampleRate <= 0 || sampleRate > 192000) {
    throw std::runtime_error("Invalid sampleRate: must be between 0 and 192000");
  }
  
  if (bufferLength <= 0) {
    throw std::runtime_error("Invalid bufferLength: must be greater than 0");
  }

  try {
#ifdef ANDROID
    audioRecorder_ = std::make_shared<AndroidAudioRecorder>(
        sampleRate, bufferLength, audioEventHandlerRegistry);
#else
    audioRecorder_ = std::make_shared<IOSAudioRecorder>(
        sampleRate, bufferLength, audioEventHandlerRegistry);
#endif

    if (!audioRecorder_) {
      throw std::runtime_error("Failed to create audio recorder instance");
    }

    addSetters(JSI_EXPORT_PROPERTY_SETTER(AudioRecorderHostObject, onAudioReady));

    addFunctions(
        JSI_EXPORT_FUNCTION(AudioRecorderHostObject, start),
        JSI_EXPORT_FUNCTION(AudioRecorderHostObject, stop),
        JSI_EXPORT_FUNCTION(AudioRecorderHostObject, connect),
        JSI_EXPORT_FUNCTION(AudioRecorderHostObject, disconnect));
  } catch (const std::exception& e) {
    throw std::runtime_error(std::string("Failed to initialize AudioRecorder: ") + e.what());
  }
}

JSI_PROPERTY_SETTER_IMPL(AudioRecorderHostObject, onAudioReady) {
  if (!audioRecorder_) {
    throw jsi::JSError(runtime, "AudioRecorder is not initialized");
  }

  if (!value.isString()) {
    throw jsi::JSError(runtime, "onAudioReady callback ID must be a string");
  }

  try {
    auto callbackIdStr = value.getString(runtime).utf8(runtime);
    auto callbackId = std::stoull(callbackIdStr);
    audioRecorder_->setOnAudioReadyCallbackId(callbackId);
  } catch (const std::exception& e) {
    throw jsi::JSError(runtime, std::string("Failed to set onAudioReady callback: ") + e.what());
  }
}

JSI_HOST_FUNCTION_IMPL(AudioRecorderHostObject, connect) {
  if (!audioRecorder_) {
    throw jsi::JSError(runtime, "AudioRecorder is not initialized");
  }

  // Validate arguments
  if (count < 1) {
    throw jsi::JSError(runtime, "connect() requires 1 argument");
  }

  if (!args[0].isObject()) {
    throw jsi::JSError(runtime, "connect() argument must be an object");
  }

  try {
    auto argObject = args[0].getObject(runtime);
    
    // Check if the object has a host object
    if (!argObject.isHostObject(runtime)) {
      throw jsi::JSError(runtime, "connect() argument must be a RecorderAdapterNode");
    }

    auto adapterNodeHostObject = argObject.getHostObject<RecorderAdapterNodeHostObject>(runtime);
    
    if (!adapterNodeHostObject) {
      throw jsi::JSError(runtime, "Failed to get RecorderAdapterNodeHostObject");
    }

    if (!adapterNodeHostObject->node_) {
      throw jsi::JSError(runtime, "RecorderAdapterNode is not initialized");
    }

    audioRecorder_->connect(
        std::static_pointer_cast<RecorderAdapterNode>(
            adapterNodeHostObject->node_));
    
    return jsi::Value::undefined();
  } catch (const jsi::JSError&) {
    throw; // Re-throw JSErrors as-is
  } catch (const std::exception& e) {
    throw jsi::JSError(runtime, std::string("Failed to connect: ") + e.what());
  }
}

JSI_HOST_FUNCTION_IMPL(AudioRecorderHostObject, disconnect) {
  if (!audioRecorder_) {
    throw jsi::JSError(runtime, "AudioRecorder is not initialized");
  }

  try {
    audioRecorder_->disconnect();
    return jsi::Value::undefined();
  } catch (const std::exception& e) {
    throw jsi::JSError(runtime, std::string("Failed to disconnect: ") + e.what());
  }
}

JSI_HOST_FUNCTION_IMPL(AudioRecorderHostObject, start) {
  if (!audioRecorder_) {
    throw jsi::JSError(runtime, "AudioRecorder is not initialized");
  }

  try {
    audioRecorder_->start();
    return jsi::Value::undefined();
  } catch (const std::exception& e) {
    throw jsi::JSError(runtime, std::string("Failed to start recording: ") + e.what());
  }
}

JSI_HOST_FUNCTION_IMPL(AudioRecorderHostObject, stop) {
  if (!audioRecorder_) {
    throw jsi::JSError(runtime, "AudioRecorder is not initialized");
  }

  try {
    audioRecorder_->stop();
    return jsi::Value::undefined();
  } catch (const std::exception& e) {
    throw jsi::JSError(runtime, std::string("Failed to stop recording: ") + e.what());
  }
}

} // namespace audioapi