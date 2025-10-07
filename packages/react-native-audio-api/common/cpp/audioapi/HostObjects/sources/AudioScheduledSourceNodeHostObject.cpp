#include <audioapi/HostObjects/sources/AudioScheduledSourceNodeHostObject.h>

#include <audioapi/core/sources/AudioScheduledSourceNode.h>

namespace audioapi {

AudioScheduledSourceNodeHostObject::AudioScheduledSourceNodeHostObject(
    const std::shared_ptr<AudioScheduledSourceNode> &node)
    : AudioNodeHostObject(node) {
  
  // Validate the node
  if (!node) {
    throw std::runtime_error("AudioScheduledSourceNode cannot be null");
  }

  try {
    addSetters(
        JSI_EXPORT_PROPERTY_SETTER(AudioScheduledSourceNodeHostObject, onEnded));

    addFunctions(
        JSI_EXPORT_FUNCTION(AudioScheduledSourceNodeHostObject, start),
        JSI_EXPORT_FUNCTION(AudioScheduledSourceNodeHostObject, stop));
  } catch (const std::exception& e) {
    throw std::runtime_error(std::string("Failed to initialize AudioScheduledSourceNodeHostObject: ") + e.what());
  }
}

AudioScheduledSourceNodeHostObject::~AudioScheduledSourceNodeHostObject() {
  try {
    // Validate node_ exists before accessing
    if (!node_) {
      return;
    }

    auto audioScheduledSourceNode =
        std::static_pointer_cast<AudioScheduledSourceNode>(node_);

    // Validate the cast succeeded
    if (!audioScheduledSourceNode) {
      return;
    }

    // When JSI object is garbage collected (together with the eventual callback),
    // underlying source node might still be active and try to call the
    // non-existing callback.
    audioScheduledSourceNode->clearOnEndedCallback();
  } catch (const std::exception& e) {
    // Don't throw from destructor, just log or ignore
    // std::cerr << "Error in ~AudioScheduledSourceNodeHostObject: " << e.what() << std::endl;
  }
}

JSI_PROPERTY_SETTER_IMPL(AudioScheduledSourceNodeHostObject, onEnded) {
  // Validate node_ exists
  if (!node_) {
    throw jsi::JSError(runtime, "AudioScheduledSourceNode is not initialized");
  }

  // Validate value is a string
  if (!value.isString()) {
    throw jsi::JSError(runtime, "onEnded callback ID must be a string");
  }

  try {
    auto audioScheduleSourceNode =
        std::static_pointer_cast<AudioScheduledSourceNode>(node_);

    if (!audioScheduleSourceNode) {
      throw jsi::JSError(runtime, "Failed to cast to AudioScheduledSourceNode");
    }

    auto callbackIdStr = value.getString(runtime).utf8(runtime);
    
    // Validate the string is not empty
    if (callbackIdStr.empty()) {
      throw jsi::JSError(runtime, "onEnded callback ID cannot be empty");
    }

    // Convert string to uint64_t with error handling
    uint64_t callbackId = std::stoull(callbackIdStr);
    audioScheduleSourceNode->setOnEndedCallbackId(callbackId);
  } catch (const std::invalid_argument& e) {
    throw jsi::JSError(runtime, "Invalid callback ID format: must be a valid number");
  } catch (const std::out_of_range& e) {
    throw jsi::JSError(runtime, "Callback ID is out of range");
  } catch (const jsi::JSError&) {
    throw; // Re-throw JSErrors as-is
  } catch (const std::exception& e) {
    throw jsi::JSError(runtime, std::string("Failed to set onEnded callback: ") + e.what());
  }
}

JSI_HOST_FUNCTION_IMPL(AudioScheduledSourceNodeHostObject, start) {
  // Validate node_ exists
  if (!node_) {
    throw jsi::JSError(runtime, "AudioScheduledSourceNode is not initialized");
  }

  // Validate argument count
  if (count < 1) {
    throw jsi::JSError(runtime, "start() requires 1 argument (when)");
  }

  // Validate argument is a number
  if (!args[0].isNumber()) {
    throw jsi::JSError(runtime, "start() argument (when) must be a number");
  }

  try {
    auto when = args[0].getNumber();
    
    // Validate 'when' is not negative
    if (when < 0) {
      throw jsi::JSError(runtime, "start() time cannot be negative");
    }

    auto audioScheduleSourceNode =
        std::static_pointer_cast<AudioScheduledSourceNode>(node_);

    if (!audioScheduleSourceNode) {
      throw jsi::JSError(runtime, "Failed to cast to AudioScheduledSourceNode");
    }

    audioScheduleSourceNode->start(when);
    return jsi::Value::undefined();
  } catch (const jsi::JSError&) {
    throw; // Re-throw JSErrors as-is
  } catch (const std::exception& e) {
    throw jsi::JSError(runtime, std::string("Failed to start: ") + e.what());
  }
}

JSI_HOST_FUNCTION_IMPL(AudioScheduledSourceNodeHostObject, stop) {
  // Validate node_ exists
  if (!node_) {
    throw jsi::JSError(runtime, "AudioScheduledSourceNode is not initialized");
  }

  // Validate argument count
  if (count < 1) {
    throw jsi::JSError(runtime, "stop() requires 1 argument (time)");
  }

  // Validate argument is a number
  if (!args[0].isNumber()) {
    throw jsi::JSError(runtime, "stop() argument (time) must be a number");
  }

  try {
    auto time = args[0].getNumber();
    
    // Validate 'time' is not negative
    if (time < 0) {
      throw jsi::JSError(runtime, "stop() time cannot be negative");
    }

    auto audioScheduleSourceNode =
        std::static_pointer_cast<AudioScheduledSourceNode>(node_);

    if (!audioScheduleSourceNode) {
      throw jsi::JSError(runtime, "Failed to cast to AudioScheduledSourceNode");
    }

    audioScheduleSourceNode->stop(time);
    return jsi::Value::undefined();
  } catch (const jsi::JSError&) {
    throw; // Re-throw JSErrors as-is
  } catch (const std::exception& e) {
    throw jsi::JSError(runtime, std::string("Failed to stop: ") + e.what());
  }
}

} // namespace audioapi