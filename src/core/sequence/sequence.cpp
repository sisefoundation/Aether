// Copyright 2026 Society of Indian Screen Editors Foundation
//
// AETHER
// Architect: Subhajit Singha, SISE
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "aether/core/sequence/sequence.hpp"

#include <limits>

namespace aether {
namespace {

std::int64_t gcdPositive(std::int64_t left, std::int64_t right) {
  while (right != 0) {
    const std::int64_t next = left % right;
    left = right;
    right = next;
  }
  return left < 0 ? -left : left;
}

}  // namespace

bool isAcceptedFrameRate(FrameRate rate) {
  if (rate.numerator <= 0 || rate.denominator <= 0) {
    return false;
  }
  if (gcdPositive(rate.numerator, rate.denominator) != 1) {
    return false;
  }
  const FrameRate named[] = {{24, 1}, {25, 1}, {24000, 1001}, {30000, 1001}, {60000, 1001}};
  for (const FrameRate candidate : named) {
    if (candidate.numerator == rate.numerator && candidate.denominator == rate.denominator) {
      return true;
    }
  }
  return false;
}

bool isHalfOpenFrameSpan(std::int64_t start, std::int64_t end) {
  return start >= 0 && end > start;
}

bool recordRangesOverlap(std::int64_t startA, std::int64_t endA, std::int64_t startB, std::int64_t endB) {
  return startA < endB && startB < endA;
}

Result<void> validateInstancePlacement(const TimelineInstance& instance,
                                       FrameRate sequenceRate,
                                       const std::vector<TimelineInstance>& sameTrack,
                                       const std::string& exceptInstanceId) {
  if (instance.sequenceId.empty() || instance.trackId.empty() || instance.masterObjectId.empty() ||
      instance.mediaId.empty()) {
    return Result<void>::failure(ErrorCode::InvalidArgument, "instance identity is incomplete");
  }
  if (!isAcceptedFrameRate(sequenceRate) || !isAcceptedFrameRate(instance.sourceRate)) {
    return Result<void>::failure(ErrorCode::InvalidArgument, "frame rate is not an accepted reduced rate");
  }
  if (!isHalfOpenFrameSpan(instance.sourceStart, instance.sourceEnd) ||
      !isHalfOpenFrameSpan(instance.recordStart, instance.recordEnd)) {
    return Result<void>::failure(ErrorCode::InvalidArgument,
                                 "a frame span must be a half-open range of non-negative int64 frames");
  }
  for (const TimelineInstance& existing : sameTrack) {
    if (!exceptInstanceId.empty() && existing.instanceId == exceptInstanceId) {
      continue;
    }
    if (existing.trackId != instance.trackId) {
      continue;
    }
    if (recordRangesOverlap(instance.recordStart, instance.recordEnd, existing.recordStart, existing.recordEnd)) {
      return Result<void>::failure(ErrorCode::InvalidArgument,
                                   "record ranges on one track must not overlap; transitions are not implemented");
    }
  }
  return Result<void>::success();
}

Result<std::int64_t> identityRecordEnd(FrameRate sequenceRate,
                                      FrameRate sourceRate,
                                      std::int64_t sourceStart,
                                      std::int64_t sourceEnd,
                                      std::int64_t recordStart) {
  if (!isAcceptedFrameRate(sequenceRate) || !isAcceptedFrameRate(sourceRate)) {
    return Result<std::int64_t>::failure(ErrorCode::InvalidArgument, "frame rate is not an accepted reduced rate");
  }
  if (sequenceRate.numerator != sourceRate.numerator || sequenceRate.denominator != sourceRate.denominator) {
    return Result<std::int64_t>::failure(ErrorCode::InvalidArgument,
                                        "source rate and sequence rate differ; no duration mapping is authorised");
  }
  if (!isHalfOpenFrameSpan(sourceStart, sourceEnd)) {
    return Result<std::int64_t>::failure(ErrorCode::InvalidArgument,
                                        "a frame span must be a half-open range of non-negative int64 frames");
  }
  if (recordStart < 0) {
    return Result<std::int64_t>::failure(ErrorCode::InvalidArgument, "record start is negative");
  }
  const std::int64_t span = sourceEnd - sourceStart;
  if (recordStart > std::numeric_limits<std::int64_t>::max() - span) {
    return Result<std::int64_t>::failure(ErrorCode::InvalidArgument, "record range does not fit");
  }
  return Result<std::int64_t>::success(recordStart + span);
}

Result<std::int64_t> exclusiveEndFromInclusiveMark(std::int64_t markIn, std::int64_t markOut) {
  if (markIn < 0 || markOut < 0) {
    return Result<std::int64_t>::failure(ErrorCode::InvalidArgument, "a frame must be a non-negative integer");
  }
  if (markOut < markIn) {
    return Result<std::int64_t>::failure(ErrorCode::InvalidArgument, "Mark Out is before Mark In");
  }
  if (markOut == std::numeric_limits<std::int64_t>::max()) {
    return Result<std::int64_t>::failure(ErrorCode::InvalidArgument, "inclusive Mark Out does not fit");
  }
  return Result<std::int64_t>::success(markOut + 1);
}

namespace {

Result<TimelineInstance> retreatTail(const TimelineInstance& instance, std::int64_t recordCut) {
  if (recordCut <= instance.recordStart || recordCut >= instance.recordEnd) {
    return Result<TimelineInstance>::failure(ErrorCode::InvalidArgument, "leftover source span is not a half-open range");
  }
  const std::int64_t delta = instance.recordEnd - recordCut;
  if (instance.sourceEnd < delta) {
    return Result<TimelineInstance>::failure(ErrorCode::InvalidArgument, "leftover source span is not a half-open range");
  }
  TimelineInstance next = instance;
  next.recordEnd = recordCut;
  next.sourceEnd = instance.sourceEnd - delta;
  if (!isHalfOpenFrameSpan(next.sourceStart, next.sourceEnd) ||
      !isHalfOpenFrameSpan(next.recordStart, next.recordEnd)) {
    return Result<TimelineInstance>::failure(ErrorCode::InvalidArgument, "leftover source span is not a half-open range");
  }
  return Result<TimelineInstance>::success(next);
}

Result<TimelineInstance> advanceHead(const TimelineInstance& instance, std::int64_t recordCut) {
  if (recordCut <= instance.recordStart || recordCut >= instance.recordEnd) {
    return Result<TimelineInstance>::failure(ErrorCode::InvalidArgument, "leftover source span is not a half-open range");
  }
  const std::int64_t delta = recordCut - instance.recordStart;
  if (instance.sourceStart > std::numeric_limits<std::int64_t>::max() - delta) {
    return Result<TimelineInstance>::failure(ErrorCode::InvalidArgument, "leftover source span does not fit");
  }
  TimelineInstance next = instance;
  next.recordStart = recordCut;
  next.sourceStart = instance.sourceStart + delta;
  if (!isHalfOpenFrameSpan(next.sourceStart, next.sourceEnd) ||
      !isHalfOpenFrameSpan(next.recordStart, next.recordEnd)) {
    return Result<TimelineInstance>::failure(ErrorCode::InvalidArgument, "leftover source span is not a half-open range");
  }
  return Result<TimelineInstance>::success(next);
}

}  // namespace

Result<SameRateOverwritePlan> planSameRateOverwrite(const TimelineInstance& replacement,
                                                  const std::vector<TimelineInstance>& existing) {
  if (!isHalfOpenFrameSpan(replacement.sourceStart, replacement.sourceEnd) ||
      !isHalfOpenFrameSpan(replacement.recordStart, replacement.recordEnd)) {
    return Result<SameRateOverwritePlan>::failure(ErrorCode::InvalidArgument,
                                                 "a frame span must be a half-open range of non-negative int64 frames");
  }
  if ((replacement.sourceEnd - replacement.sourceStart) != (replacement.recordEnd - replacement.recordStart)) {
    return Result<SameRateOverwritePlan>::failure(ErrorCode::InvalidArgument,
                                                 "source duration and record duration differ");
  }
  SameRateOverwritePlan plan;
  TimelineInstance incoming = replacement;
  incoming.instanceId.clear();
  plan.inserted.push_back(incoming);
  for (const TimelineInstance& instance : existing) {
    if (instance.trackId != replacement.trackId) {
      continue;
    }
    if (!recordRangesOverlap(instance.recordStart, instance.recordEnd, replacement.recordStart,
                            replacement.recordEnd)) {
      continue;
    }
    const bool startsBefore = instance.recordStart < replacement.recordStart;
    const bool endsAfter = instance.recordEnd > replacement.recordEnd;
    const bool startsInside = instance.recordStart >= replacement.recordStart;
    const bool endsInside = instance.recordEnd <= replacement.recordEnd;
    if (startsInside && endsInside) {
      plan.removed.push_back(instance);
      continue;
    }
    if (startsBefore && endsAfter) {
      auto left = retreatTail(instance, replacement.recordStart);
      if (!left.ok()) {
        return Result<SameRateOverwritePlan>::failure(left.error.code, left.error.message);
      }
      auto right = advanceHead(instance, replacement.recordEnd);
      if (!right.ok()) {
        return Result<SameRateOverwritePlan>::failure(right.error.code, right.error.message);
      }
      right.value.instanceId.clear();
      plan.updated.push_back(left.value);
      plan.inserted.push_back(right.value);
      continue;
    }
    if (startsBefore && endsInside) {
      auto left = retreatTail(instance, replacement.recordStart);
      if (!left.ok()) {
        return Result<SameRateOverwritePlan>::failure(left.error.code, left.error.message);
      }
      plan.updated.push_back(left.value);
      continue;
    }
    if (startsInside && endsAfter) {
      auto right = advanceHead(instance, replacement.recordEnd);
      if (!right.ok()) {
        return Result<SameRateOverwritePlan>::failure(right.error.code, right.error.message);
      }
      plan.updated.push_back(right.value);
      continue;
    }
    return Result<SameRateOverwritePlan>::failure(ErrorCode::InvalidArgument,
                                                 "overwrite intersection was not classified");
  }
  return Result<SameRateOverwritePlan>::success(std::move(plan));
}

namespace {

FrameRate reducedRate(FrameRate rate) {
  if (rate.numerator <= 0 || rate.denominator <= 0) {
    return {};
  }
  const std::int64_t divisor = gcdPositive(rate.numerator, rate.denominator);
  if (divisor <= 0) {
    return {};
  }
  return {rate.numerator / divisor, rate.denominator / divisor};
}

bool sameReducedRate(FrameRate left, FrameRate right) {
  const FrameRate reducedLeft = reducedRate(left);
  const FrameRate reducedRight = reducedRate(right);
  return reducedLeft.numerator > 0 && reducedLeft.denominator > 0 &&
         reducedLeft.numerator == reducedRight.numerator && reducedLeft.denominator == reducedRight.denominator;
}

}  // namespace

Result<SplitPlan> planSplit(const TimelineInstance& instance, FrameRate sequenceRate, std::int64_t frame) {
  if (frame == instance.recordStart || frame == instance.recordEnd) {
    return Result<SplitPlan>::failure(ErrorCode::InvalidArgument, "a split at the start or end is rejected");
  }
  if (frame < instance.recordStart || frame > instance.recordEnd) {
    return Result<SplitPlan>::failure(ErrorCode::InvalidArgument, "split frame is not inside the record span");
  }
  if (!isHalfOpenFrameSpan(instance.sourceStart, instance.sourceEnd) ||
      !isHalfOpenFrameSpan(instance.recordStart, instance.recordEnd)) {
    return Result<SplitPlan>::failure(ErrorCode::InvalidArgument,
                                      "a frame span must be a half-open range of non-negative int64 frames");
  }
  const std::int64_t sourceDuration = instance.sourceEnd - instance.sourceStart;
  const std::int64_t recordDuration = instance.recordEnd - instance.recordStart;
  if (!sameReducedRate(instance.sourceRate, sequenceRate) || sourceDuration != recordDuration) {
    return Result<SplitPlan>::failure(ErrorCode::InvalidArgument, "source span is not one frame per record frame");
  }
  const std::int64_t sourceSplit = instance.sourceStart + (frame - instance.recordStart);
  SplitPlan plan;
  plan.left = instance;
  plan.left.recordEnd = frame;
  plan.left.sourceEnd = sourceSplit;
  plan.right = instance;
  plan.right.instanceId.clear();
  plan.right.recordStart = frame;
  plan.right.sourceStart = sourceSplit;
  return Result<SplitPlan>::success(std::move(plan));
}

namespace {

bool addFrames(std::int64_t left, std::int64_t right, std::int64_t* out) {
  if (right > 0 && left > std::numeric_limits<std::int64_t>::max() - right) {
    return false;
  }
  if (right < 0 && left < std::numeric_limits<std::int64_t>::min() - right) {
    return false;
  }
  *out = left + right;
  return true;
}

bool sourceFrameAt(const TimelineInstance& instance, std::int64_t frame, std::int64_t* out) {
  std::int64_t delta = 0;
  if (!addFrames(frame, -instance.recordStart, &delta)) {
    return false;
  }
  return addFrames(instance.sourceStart, delta, out);
}

std::string crossingRefusal(const TimelineInstance& instance) {
  return "Track " + instance.trackId + " instance " + instance.instanceId +
         " crosses the extract interval and its source span is not one frame per record frame.";
}

}  // namespace

Result<std::vector<ExtractTrackResult>> planExtract(FrameRate sequenceRate,
                                                   std::int64_t recordStart,
                                                   std::int64_t recordEnd,
                                                   const std::vector<ExtractTrackInput>& tracks) {
  if (!isAcceptedFrameRate(sequenceRate)) {
    return Result<std::vector<ExtractTrackResult>>::failure(ErrorCode::InvalidArgument,
                                                            "frame rate is not an accepted reduced rate");
  }
  if (!isHalfOpenFrameSpan(recordStart, recordEnd)) {
    return Result<std::vector<ExtractTrackResult>>::failure(
        ErrorCode::InvalidArgument, "the record range must be a half-open interval of integer sequence frames");
  }
  const std::int64_t distance = recordEnd - recordStart;
  std::vector<ExtractTrackResult> results;
  for (const ExtractTrackInput& input : tracks) {
    if (input.trackId.empty()) {
      return Result<std::vector<ExtractTrackResult>>::failure(ErrorCode::InvalidArgument, "choose a track");
    }
    for (std::size_t left = 0; left < input.instances.size(); ++left) {
      for (std::size_t right = left + 1; right < input.instances.size(); ++right) {
        const TimelineInstance& first = input.instances[left];
        const TimelineInstance& second = input.instances[right];
        if (recordRangesOverlap(first.recordStart, first.recordEnd, second.recordStart, second.recordEnd)) {
          return Result<std::vector<ExtractTrackResult>>::failure(
              ErrorCode::InvalidArgument,
              "Track " + input.trackId + " already has overlapping record ranges. Extract wrote nothing.");
        }
      }
    }
    ExtractTrackResult result;
    result.trackId = input.trackId;
    result.before = input.instances;
    for (const TimelineInstance& instance : input.instances) {
      if (instance.trackId != input.trackId) {
        return Result<std::vector<ExtractTrackResult>>::failure(ErrorCode::InvalidArgument,
                                                                "Track " + input.trackId + " instance " +
                                                                    instance.instanceId + " is not on that track.");
      }
      if (!isHalfOpenFrameSpan(instance.sourceStart, instance.sourceEnd) ||
          !isHalfOpenFrameSpan(instance.recordStart, instance.recordEnd)) {
        return Result<std::vector<ExtractTrackResult>>::failure(
            ErrorCode::InvalidArgument, "Track " + input.trackId + " instance " + instance.instanceId +
                                           " is not a half-open frame span.");
      }
      const std::int64_t start = instance.recordStart;
      const std::int64_t end = instance.recordEnd;
      if (end <= recordStart) {
        result.after.push_back(instance);
        continue;
      }
      if (start >= recordEnd) {
        TimelineInstance shifted = instance;
        shifted.recordStart = start - distance;
        shifted.recordEnd = end - distance;
        result.after.push_back(std::move(shifted));
        continue;
      }
      if (start >= recordStart && end <= recordEnd) {
        continue;
      }
      const std::int64_t sourceDuration = instance.sourceEnd - instance.sourceStart;
      const std::int64_t recordDuration = end - start;
      if (!sameReducedRate(instance.sourceRate, sequenceRate) || sourceDuration != recordDuration) {
        return Result<std::vector<ExtractTrackResult>>::failure(ErrorCode::InvalidArgument, crossingRefusal(instance));
      }
      auto refuseLength = [&](const char* reason) {
        return Result<std::vector<ExtractTrackResult>>::failure(
            ErrorCode::InvalidArgument, "Track " + input.trackId + " instance " + instance.instanceId + " " + reason);
      };
      if (start < recordStart && end > recordEnd) {
        std::int64_t leftSourceEnd = 0;
        std::int64_t rightSourceStart = 0;
        if (!sourceFrameAt(instance, recordStart, &leftSourceEnd) ||
            !sourceFrameAt(instance, recordEnd, &rightSourceStart)) {
          return refuseLength("source mapping does not fit.");
        }
        TimelineInstance left = instance;
        left.recordEnd = recordStart;
        left.sourceEnd = leftSourceEnd;
        TimelineInstance right = instance;
        right.instanceId.clear();
        right.recordStart = recordStart;
        std::int64_t rightRecordEnd = 0;
        if (!addFrames(recordStart, end - recordEnd, &rightRecordEnd)) {
          return refuseLength("record mapping does not fit.");
        }
        right.recordEnd = rightRecordEnd;
        right.sourceStart = rightSourceStart;
        if (!isHalfOpenFrameSpan(left.sourceStart, left.sourceEnd) ||
            !isHalfOpenFrameSpan(left.recordStart, left.recordEnd) ||
            !isHalfOpenFrameSpan(right.sourceStart, right.sourceEnd) ||
            !isHalfOpenFrameSpan(right.recordStart, right.recordEnd)) {
          return refuseLength("would leave a zero-length fragment.");
        }
        result.after.push_back(std::move(left));
        result.after.push_back(std::move(right));
        continue;
      }
      if (start < recordStart && end <= recordEnd) {
        std::int64_t leftSourceEnd = 0;
        if (!sourceFrameAt(instance, recordStart, &leftSourceEnd)) {
          return refuseLength("source mapping does not fit.");
        }
        TimelineInstance left = instance;
        left.recordEnd = recordStart;
        left.sourceEnd = leftSourceEnd;
        if (!isHalfOpenFrameSpan(left.sourceStart, left.sourceEnd) ||
            !isHalfOpenFrameSpan(left.recordStart, left.recordEnd)) {
          return refuseLength("would leave a zero-length fragment.");
        }
        result.after.push_back(std::move(left));
        continue;
      }
      if (start >= recordStart && end > recordEnd) {
        std::int64_t rightSourceStart = 0;
        std::int64_t rightRecordEnd = 0;
        if (!sourceFrameAt(instance, recordEnd, &rightSourceStart) ||
            !addFrames(recordStart, end - recordEnd, &rightRecordEnd)) {
          return refuseLength("source mapping does not fit.");
        }
        TimelineInstance piece = instance;
        piece.recordStart = recordStart;
        piece.recordEnd = rightRecordEnd;
        piece.sourceStart = rightSourceStart;
        if (start != recordStart) {
          piece.instanceId.clear();
        }
        if (!isHalfOpenFrameSpan(piece.sourceStart, piece.sourceEnd) ||
            !isHalfOpenFrameSpan(piece.recordStart, piece.recordEnd)) {
          return refuseLength("would leave a zero-length fragment.");
        }
        result.after.push_back(std::move(piece));
        continue;
      }
      return Result<std::vector<ExtractTrackResult>>::failure(ErrorCode::InvalidArgument,
                                                              "extract intersection was not classified");
    }
    for (std::size_t left = 0; left < result.after.size(); ++left) {
      for (std::size_t right = left + 1; right < result.after.size(); ++right) {
        const TimelineInstance& first = result.after[left];
        const TimelineInstance& second = result.after[right];
        if (recordRangesOverlap(first.recordStart, first.recordEnd, second.recordStart, second.recordEnd)) {
          return Result<std::vector<ExtractTrackResult>>::failure(
              ErrorCode::InvalidArgument,
              "record ranges on one track must not overlap; transitions are not implemented");
        }
      }
    }
    results.push_back(std::move(result));
  }
  return Result<std::vector<ExtractTrackResult>>::success(std::move(results));
}

}  // namespace aether
