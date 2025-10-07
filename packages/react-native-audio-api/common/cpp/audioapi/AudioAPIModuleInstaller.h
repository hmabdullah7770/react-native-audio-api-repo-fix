#pragma once

#include <audioapi/jsi/JsiPromise.h>
#include <audioapi/core/AudioContext.h>
#include <audioapi/core/OfflineAudioContext.h>
#include <audioapi/core/inputs/AudioRecorder.h>
#include <audioapi/HostObjects/AudioContextHostObject.h>
#include <audioapi/HostObjects/OfflineAudioContextHostObject.h>
#include <audioapi/HostObjects/inputs/AudioRecorderHostObject.h>

#include <audioapi/events/AudioEventHandlerRegistry.h>
#include <audioapi/HostObjects/events/AudioEventHandlerRegistryHostObject.h>

#include <audioapi/core/utils/worklets/SafeIncludes.h>

#include <memory>

namespace audioapi {

using namespace facebook;

class AudioAPIModuleInstaller {
 public:
  static void injectJSIBindings(
    jsi::Runtime *jsiRuntime,
    const std::shared_ptr<react::CallInvoker> &jsCallInvoker,
    const std::shared_ptr<AudioEventHandlerRegistry> &audioEventHandlerRegistry,
    std::shared_ptr<worklets::WorkletRuntime> uiRuntime = nullptr) {

    auto createAudioContext = getCreateAudioContextFunction(jsiRuntime, jsCallInvoker, audioEventHandlerRegistry, uiRuntime);
    auto createAudioRecorder = getCreateAudioRecorderFunction(jsiRuntime, audioEventHandlerRegistry);
    auto createOfflineAudioContext = getCreateOfflineAudioContextFunction(jsiRuntime, jsCallInvoker, audioEventHandlerRegistry, uiRuntime);

    jsiRuntime->global().setProperty(*jsiRuntime, "createAudioContext", createAudioContext);
    jsiRuntime->global().setProperty(*jsiRuntime, "createAudioRecorder", createAudioRecorder);
    jsiRuntime->global().setProperty(*jsiRuntime, "createOfflineAudioContext", createOfflineAudioContext);

    auto audioEventHandlerRegistryHostObject = std::make_shared<AudioEventHandlerRegistryHostObject>(audioEventHandlerRegistry);
    jsiRuntime->global().setProperty(*jsiRuntime, "AudioEventEmitter", jsi::Object::createFromHostObject(*jsiRuntime, audioEventHandlerRegistryHostObject));
  }

 private:
  static jsi::Function getCreateAudioContextFunction(
    jsi::Runtime *jsiRuntime,
    const std::shared_ptr<react::CallInvoker> &jsCallInvoker,
    const std::shared_ptr<AudioEventHandlerRegistry> &audioEventHandlerRegistry,
    const std::weak_ptr<worklets::WorkletRuntime> &uiRuntime) {
    return jsi::Function::createFromHostFunction(
        *jsiRuntime,
        jsi::PropNameID::forAscii(*jsiRuntime, "createAudioContext"),
        0,
        [jsCallInvoker, audioEventHandlerRegistry, uiRuntime](
            jsi::Runtime &runtime,
            const jsi::Value &thisValue,
            const jsi::Value *args,
            size_t count) -> jsi::Value {
          
          // Validate argument count
          if (count < 2) {
            throw jsi::JSError(runtime, "createAudioContext requires at least 2 arguments");
          }

          // Validate arguments
          if (!args[0].isNumber()) {
            throw jsi::JSError(runtime, "First argument (sampleRate) must be a number");
          }
          if (!args[1].isBool()) {
            throw jsi::JSError(runtime, "Second argument (initSuspended) must be a boolean");
          }

          std::shared_ptr<AudioContext> audioContext;
          auto sampleRate = static_cast<float>(args[0].getNumber());
          auto initSuspended = args[1].getBool();

          #if RN_AUDIO_API_ENABLE_WORKLETS
              auto runtimeRegistry = RuntimeRegistry{
                  .uiRuntime = uiRuntime,
                  .audioRuntime = worklets::extractWorkletRuntime(runtime, args[2])
              };
          #else
              auto runtimeRegistry = RuntimeRegistry{};
          #endif

          try {
            audioContext = std::make_shared<AudioContext>(sampleRate, initSuspended, audioEventHandlerRegistry, runtimeRegistry);
            auto audioContextHostObject = std::make_shared<AudioContextHostObject>(
                audioContext, &runtime, jsCallInvoker);

            return jsi::Object::createFromHostObject(
                runtime, audioContextHostObject);
          } catch (const std::exception& e) {
            throw jsi::JSError(runtime, std::string("Failed to create AudioContext: ") + e.what());
          }
        });
  }

  static jsi::Function getCreateOfflineAudioContextFunction(
    jsi::Runtime *jsiRuntime,
    const std::shared_ptr<react::CallInvoker> &jsCallInvoker,
    const std::shared_ptr<AudioEventHandlerRegistry> &audioEventHandlerRegistry,
    const std::weak_ptr<worklets::WorkletRuntime> &uiRuntime) {
    return jsi::Function::createFromHostFunction(
        *jsiRuntime,
        jsi::PropNameID::forAscii(*jsiRuntime, "createOfflineAudioContext"),
        0,
        [jsCallInvoker, audioEventHandlerRegistry, uiRuntime](
            jsi::Runtime &runtime,
            const jsi::Value &thisValue,
            const jsi::Value *args,
            size_t count) -> jsi::Value {
            
            // Validate argument count
            if (count < 3) {
              throw jsi::JSError(runtime, "createOfflineAudioContext requires at least 3 arguments");
            }

            // Validate arguments
            if (!args[0].isNumber()) {
              throw jsi::JSError(runtime, "First argument (numberOfChannels) must be a number");
            }
            if (!args[1].isNumber()) {
              throw jsi::JSError(runtime, "Second argument (length) must be a number");
            }
            if (!args[2].isNumber()) {
              throw jsi::JSError(runtime, "Third argument (sampleRate) must be a number");
            }

            auto numberOfChannels = static_cast<int>(args[0].getNumber());
            auto length = static_cast<size_t>(args[1].getNumber());
            auto sampleRate = static_cast<float>(args[2].getNumber());

            #if RN_AUDIO_API_ENABLE_WORKLETS
                auto runtimeRegistry = RuntimeRegistry{
                    .uiRuntime = uiRuntime,
                    .audioRuntime = worklets::extractWorkletRuntime(runtime, args[3])
                };
            #else
                auto runtimeRegistry = RuntimeRegistry{};
            #endif

            try {
              auto offlineAudioContext = std::make_shared<OfflineAudioContext>(numberOfChannels, length, sampleRate, audioEventHandlerRegistry, runtimeRegistry);
              auto audioContextHostObject = std::make_shared<OfflineAudioContextHostObject>(
                  offlineAudioContext, &runtime, jsCallInvoker);

              return jsi::Object::createFromHostObject(
                  runtime, audioContextHostObject);
            } catch (const std::exception& e) {
              throw jsi::JSError(runtime, std::string("Failed to create OfflineAudioContext: ") + e.what());
            }
        });
  }

  static jsi::Function getCreateAudioRecorderFunction(
    jsi::Runtime *jsiRuntime,
    const std::shared_ptr<AudioEventHandlerRegistry> &audioEventHandlerRegistry) {
    return jsi::Function::createFromHostFunction(
        *jsiRuntime,
        jsi::PropNameID::forAscii(*jsiRuntime, "createAudioRecorder"),
        0,
        [audioEventHandlerRegistry](
            jsi::Runtime &runtime,
            const jsi::Value &thisValue,
            const jsi::Value *args,
            size_t count) -> jsi::Value {
          
          // Validate argument count
          if (count < 1) {
            throw jsi::JSError(runtime, "createAudioRecorder requires 1 argument (options object)");
          }

          // Validate argument is an object
          if (!args[0].isObject()) {
            throw jsi::JSError(runtime, "createAudioRecorder argument must be an object");
          }

          auto options = args[0].getObject(runtime);

          // Validate sampleRate exists and is a number
          auto sampleRateProp = options.getProperty(runtime, "sampleRate");
          if (sampleRateProp.isUndefined() || !sampleRateProp.isNumber()) {
            throw jsi::JSError(runtime, "options.sampleRate must be a number");
          }
          auto sampleRate = static_cast<float>(sampleRateProp.getNumber());

          // Validate bufferLengthInSamples exists and is a number
          auto bufferLengthProp = options.getProperty(runtime, "bufferLengthInSamples");
          if (bufferLengthProp.isUndefined() || !bufferLengthProp.isNumber()) {
            throw jsi::JSError(runtime, "options.bufferLengthInSamples must be a number");
          }
          auto bufferLength = static_cast<int>(bufferLengthProp.getNumber());

          try {
            auto audioRecorderHostObject = std::make_shared<AudioRecorderHostObject>(
              audioEventHandlerRegistry, sampleRate, bufferLength);

            return jsi::Object::createFromHostObject(runtime, audioRecorderHostObject);
          } catch (const std::exception& e) {
            throw jsi::JSError(runtime, std::string("Failed to create AudioRecorder: ") + e.what());
          }
        });
  }
};

} // namespace audioapi