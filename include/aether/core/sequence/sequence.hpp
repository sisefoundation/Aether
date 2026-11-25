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

#pragma once

// Editorial sequence rules. This header does not include SQLite and does not
// open a database. Storage is int64 frames. A path is not an identity field.

#include "aether/core/project/result.hpp"
#include "aether/core/timing/sample_boundary.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace aether {

struct SequenceRecord {
  std::string sequenceId;
  std::string projectId;
  std::string name;
  FrameRate rate;
  std::string createdUtc;
  std::string modifiedUtc;
};

struct TrackRecord {
  std::string trackId;
  std::string sequenceId;
  int ordinal = 0;
  std::string createdUtc;
  std::string modifiedUtc;
};

struct TimelineInstance {
  std::string instanceId;
  std::string sequenceId;
  std::string trackId;
  std::string masterObjectId;
  std::string mediaId;
  FrameRate sourceRate;
  std::int64_t sourceStart = 0;
  std::int64_t sourceEnd = 0;
  std::int64_t recordStart = 0;
  std::int64_t recordEnd = 0;
  std::string createdUtc;
  std::string modifiedUtc;
};

// The five rates named by the accepted ADR-002. The pair must already be reduced.
bool isAcceptedFrameRate(FrameRate rate);

// Half-open [start, end). Rejects a negative, an empty span, and end < start.
// There is no floating-point argument, so a subframe cannot be rounded in.
bool isHalfOpenFrameSpan(std::int64_t start, std::int64_t end);

// True when the half-open spans share a frame. Abutting spans do not overlap.
bool recordRangesOverlap(std::int64_t startA, std::int64_t endA, std::int64_t startB, std::int64_t endB);

// Editorial placement check. Does not write. Overlap on one track is refused.
// A full transition or compositing model is not implemented here.
Result<void> validateInstancePlacement(const TimelineInstance& instance,
                                       FrameRate sequenceRate,
                                       const std::vector<TimelineInstance>& sameTrack,
                                       const std::string& exceptInstanceId);

// Record end for a placement whose source rate is the sequence rate.
// The record length is the source length. A different rate is refused.
// This does not write and does not call pictureSourceOffset.
Result<std::int64_t> identityRecordEnd(FrameRate sequenceRate,
                                       FrameRate sourceRate,
                                       std::int64_t sourceStart,
                                       std::int64_t sourceEnd,
                                       std::int64_t recordStart);

// Exclusive end of a displayed inclusive Mark Out. The stored marks stay points.
// The span a command derives is [markIn, markOut + 1).
Result<std::int64_t> exclusiveEndFromInclusiveMark(std::int64_t markIn, std::int64_t markOut);

// One destination track. Leftover pieces keep the source frames that sat on the
// surviving sequence frames. The replacement is one inserted row.
struct SameRateOverwritePlan {
  std::vector<TimelineInstance> removed;
  std::vector<TimelineInstance> updated;
  std::vector<TimelineInstance> inserted;
};

Result<SameRateOverwritePlan> planSameRateOverwrite(const TimelineInstance& replacement,
                                                   const std::vector<TimelineInstance>& existing);

// One instance cut at an interior sequence frame. The left piece keeps the id.
// The right piece has an empty id. The source cut is s0 + (f - r0) only when
// the source duration equals the record duration and the rates match after
// reduction. This does not call pictureSourceOffset and does not write.
struct SplitPlan {
  TimelineInstance left;
  TimelineInstance right;
};

Result<SplitPlan> planSplit(const TimelineInstance& instance, FrameRate sequenceRate, std::int64_t frame);

// One named track and the rows already stored on it. Extract does not write.
struct ExtractTrackInput {
  std::string trackId;
  std::vector<TimelineInstance> instances;
};

// before is the stored rows. after is the closed track. A new fragment has an
// empty instance id. A fragment that still starts at its original record start
// keeps that id. An uncut shift also keeps its id.
struct ExtractTrackResult {
  std::string trackId;
  std::vector<TimelineInstance> before;
  std::vector<TimelineInstance> after;
};

// Closes [recordStart, recordEnd) on each named track. A crossing row is cut
// only when planSplit's gate passes. Anything else writes nothing: the caller
// must not apply a failed plan. This does not call pictureSourceOffset.
Result<std::vector<ExtractTrackResult>> planExtract(FrameRate sequenceRate,
                                                    std::int64_t recordStart,
                                                    std::int64_t recordEnd,
                                                    const std::vector<ExtractTrackInput>& tracks);

}  // namespace aether
