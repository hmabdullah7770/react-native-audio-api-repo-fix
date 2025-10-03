#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/sources/ConstantSourceNode.h>
#include <audioapi/dsp/AudioUtils.h>
#include <audioapi/utils/AudioArray.h>
#include <audioapi/utils/AudioBus.h>

namespace audioapi {
ConstantSourceNode::ConstantSourceNode(BaseAudioContext *context)
    : AudioScheduledSourceNode(context) {
  offsetParam_ = std::make_shared<AudioParam>(
      1.0, MOST_NEGATIVE_SINGLE_FLOAT, MOST_POSITIVE_SINGLE_FLOAT, context);
  isInitialized_ = true;
}

std::shared_ptr<AudioParam> ConstantSourceNode::getOffsetParam() const {
  return offsetParam_;
}

void ConstantSourceNode::processNode(
    const std::shared_ptr<AudioBus> &processingBus,
    int framesToProcess) {
  size_t startOffset = 0;
  size_t offsetLength = 0;

  updatePlaybackInfo(processingBus, framesToProcess, startOffset, offsetLength);

  if (!isPlaying() && !isStopScheduled()) {
    processingBus->zero();
    return;
  }

  auto offsetBus = offsetParam_->processARateParam(
      framesToProcess, context_->getCurrentTime());

  auto offsetChannelData = offsetBus->getChannel(0)->getData();

  for (int channel = 0; channel < processingBus->getNumberOfChannels();
       ++channel) {
    auto outputChannelData = processingBus->getChannel(channel)->getData();

    std::copy(
        offsetChannelData + startOffset,
        offsetChannelData + startOffset + offsetLength,
        outputChannelData + startOffset);
  }

  if (isStopScheduled()) {
    handleStopScheduled();
  }
}
} // namespace audioapi
