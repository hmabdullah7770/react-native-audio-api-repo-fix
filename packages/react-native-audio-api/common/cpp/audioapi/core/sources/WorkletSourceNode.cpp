#include <audioapi/core/sources/WorkletSourceNode.h>
#include <audioapi/core/utils/Constants.h>

namespace audioapi {

WorkletSourceNode::WorkletSourceNode(
    BaseAudioContext *context,
    std::shared_ptr<worklets::SerializableWorklet> &worklet,
    std::weak_ptr<worklets::WorkletRuntime> runtime)
    : AudioScheduledSourceNode(context),
      workletRunner_(runtime),
      shareableWorklet_(worklet) {
  isInitialized_ = true;

  // Prepare buffers for audio processing
  size_t outputChannelCount = this->getChannelCount();
  outputBuffsHandles_.resize(outputChannelCount);
  for (size_t i = 0; i < outputChannelCount; ++i) {
    auto buff = new uint8_t[RENDER_QUANTUM_SIZE * sizeof(float)];
    outputBuffsHandles_[i] = std::make_shared<AudioArrayBuffer>(
        buff, RENDER_QUANTUM_SIZE * sizeof(float));
  }
}

void WorkletSourceNode::processNode(
    const std::shared_ptr<AudioBus> &processingBus,
    int framesToProcess) {
  if (isUnscheduled() || isFinished() || !isEnabled()) {
    processingBus->zero();
    return;
  }

  size_t startOffset = 0;
  size_t nonSilentFramesToProcess = framesToProcess;

  updatePlaybackInfo(
      processingBus, framesToProcess, startOffset, nonSilentFramesToProcess);

  if (nonSilentFramesToProcess == 0) {
    processingBus->zero();
    return;
  }

  size_t outputChannelCount = processingBus->getNumberOfChannels();

  auto result = workletRunner_.executeOnRuntimeGuardedSync(
      [this, nonSilentFramesToProcess, startOffset](jsi::Runtime &rt) {
        auto jsiArray = jsi::Array(rt, this->outputBuffsHandles_.size());
        for (size_t i = 0; i < this->outputBuffsHandles_.size(); ++i) {
          auto arrayBuffer = jsi::ArrayBuffer(rt, this->outputBuffsHandles_[i]);
          jsiArray.setValueAtIndex(rt, i, arrayBuffer);
        }
        return workletRunner_
            .executeWorklet(
                shareableWorklet_,
                jsiArray,
                jsi::Value(rt, static_cast<int>(nonSilentFramesToProcess)),
                jsi::Value(rt, this->context_->getCurrentTime()),
                jsi::Value(rt, static_cast<int>(startOffset)))
            .value_or(jsi::Value::undefined());
      });

  // If the worklet execution failed, zero the output
  // It might happen if the runtime is not available
  if (!result.has_value()) {
    processingBus->zero();
    return;
  }

  // Copy the processed data back to the AudioBus
  for (size_t i = 0; i < outputChannelCount; ++i) {
    float *channelData = processingBus->getChannel(i)->getData();
    memcpy(
        channelData + startOffset,
        outputBuffsHandles_[i]->data(),
        nonSilentFramesToProcess * sizeof(float));
  }

  handleStopScheduled();
}

} // namespace audioapi
