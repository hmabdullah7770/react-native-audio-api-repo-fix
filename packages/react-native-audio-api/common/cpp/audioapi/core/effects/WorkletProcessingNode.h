#pragma once

#include <jsi/jsi.h>
#include <audioapi/core/utils/worklets/WorkletsRunner.h>
#include <audioapi/core/AudioNode.h>
#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/utils/AudioBus.h>
#include <audioapi/utils/AudioArray.h>
#include <audioapi/jsi/AudioArrayBuffer.h>

#include <memory>
#include <vector>

namespace audioapi {

#if RN_AUDIO_API_TEST
class WorkletProcessingNode : public AudioNode {
 public:
  explicit WorkletProcessingNode(
      BaseAudioContext *context,
      std::shared_ptr<worklets::SerializableWorklet> &worklet,
      std::weak_ptr<worklets::WorkletRuntime> runtime
  ) : AudioNode(context) {}

 protected:
  std::shared_ptr<AudioBus> processNode(const std::shared_ptr<AudioBus>& processingBus, int framesToProcess) override { return processingBus; }
};
#else

using namespace facebook;

class WorkletProcessingNode : public AudioNode {
 public:
  explicit WorkletProcessingNode(
      BaseAudioContext *context,
      std::shared_ptr<worklets::SerializableWorklet> &worklet,
      std::weak_ptr<worklets::WorkletRuntime> runtime
  );

 protected:
  std::shared_ptr<AudioBus> processNode(const std::shared_ptr<AudioBus>& processingBus, int framesToProcess) override;

 private:
  WorkletsRunner workletRunner_;
  std::shared_ptr<worklets::SerializableWorklet> shareableWorklet_;
  std::vector<std::shared_ptr<AudioArrayBuffer>> inputBuffsHandles_;
  std::vector<std::shared_ptr<AudioArrayBuffer>> outputBuffsHandles_;
};

#endif // RN_AUDIO_API_TEST

} // namespace audioapi
