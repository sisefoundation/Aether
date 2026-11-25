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

#include "timing_tests.hpp"

#include "aether/core/timing/sample_boundary.hpp"

#include <QTest>

#include <cstdint>

namespace {

bool boundaryIs(std::int64_t n, std::int64_t numerator, std::int64_t denominator, std::int64_t expected) {
  const aether::TimingResult result =
      aether::sampleBoundary(n, 48000, aether::FrameRate{numerator, denominator});
  return result.ok() && result.value == expected;
}

bool countIs(std::int64_t start,
             std::int64_t end,
             std::int64_t numerator,
             std::int64_t denominator,
             std::int64_t expected) {
  const aether::TimingResult result =
      aether::sampleCount(start, end, 48000, aether::FrameRate{numerator, denominator});
  return result.ok() && result.value == expected;
}

}  // namespace

void TimingTests::publishedBoundaries() {
  QVERIFY(boundaryIs(0, 24, 1, 0));
  QVERIFY(boundaryIs(1, 24, 1, 2000));
  QVERIFY(boundaryIs(8, 24, 1, 16000));
  QVERIFY(boundaryIs(40, 24, 1, 80000));
  QVERIFY(boundaryIs(107892, 24, 1, 215784000));
  QVERIFY(boundaryIs(200000, 24, 1, 400000000));

  QVERIFY(boundaryIs(0, 25, 1, 0));
  QVERIFY(boundaryIs(1, 25, 1, 1920));
  QVERIFY(boundaryIs(8, 25, 1, 15360));
  QVERIFY(boundaryIs(40, 25, 1, 76800));
  QVERIFY(boundaryIs(107892, 25, 1, 207152640));
  QVERIFY(boundaryIs(200000, 25, 1, 384000000));

  QVERIFY(boundaryIs(0, 24000, 1001, 0));
  QVERIFY(boundaryIs(1, 24000, 1001, 2002));
  QVERIFY(boundaryIs(8, 24000, 1001, 16016));
  QVERIFY(boundaryIs(40, 24000, 1001, 80080));
  QVERIFY(boundaryIs(107892, 24000, 1001, 215999784));
  QVERIFY(boundaryIs(200000, 24000, 1001, 400400000));

  QVERIFY(boundaryIs(0, 30000, 1001, 0));
  QVERIFY(boundaryIs(1, 30000, 1001, 1601));
  QVERIFY(boundaryIs(2, 30000, 1001, 3203));
  QVERIFY(boundaryIs(5, 30000, 1001, 8008));
  QVERIFY(boundaryIs(8, 30000, 1001, 12812));
  QVERIFY(boundaryIs(16, 30000, 1001, 25625));
  QVERIFY(boundaryIs(40, 30000, 1001, 64064));
  QVERIFY(boundaryIs(107892, 30000, 1001, 172799827));
  QVERIFY(boundaryIs(200000, 30000, 1001, 320320000));

  QVERIFY(boundaryIs(0, 30, 1, 0));
  QVERIFY(boundaryIs(1, 30, 1, 1600));
  QVERIFY(boundaryIs(8, 30, 1, 12800));
  QVERIFY(boundaryIs(40, 30, 1, 64000));
  QVERIFY(boundaryIs(107892, 30, 1, 172627200));
  QVERIFY(boundaryIs(200000, 30, 1, 320000000));
}

void TimingTests::eightFrameSpansAreNotAllEqual() {
  const std::int64_t sizes[8] = {1601, 1602, 1601, 1602, 1602, 1601, 1602, 1601};
  std::int64_t sum = 0;
  for (int frame = 0; frame < 8; ++frame) {
    QVERIFY(countIs(frame, frame + 1, 30000, 1001, sizes[frame]));
    sum += sizes[frame];
  }
  QCOMPARE(sum, static_cast<std::int64_t>(12812));
  QVERIFY(countIs(0, 8, 30000, 1001, 12812));
  QVERIFY(countIs(8, 16, 30000, 1001, 12813));
  QVERIFY(countIs(0, 8, 30, 1, 12800));
  QVERIFY(countIs(8, 16, 30, 1, 12800));
  QVERIFY(countIs(0, 8, 24, 1, 16000));
  QVERIFY(countIs(0, 8, 25, 1, 15360));
  QVERIFY(countIs(0, 8, 24000, 1001, 16016));

  const aether::TimingResult origin = aether::sampleCount(0, 8, 48000, aether::FrameRate{30000, 1001});
  const aether::TimingResult later = aether::sampleCount(8, 16, 48000, aether::FrameRate{30000, 1001});
  QVERIFY(origin.ok());
  QVERIFY(later.ok());
  QVERIFY(origin.value != later.value);
}

void TimingTests::adjacentRepeatedAndNonZeroStarts() {
  const std::int64_t numerators[] = {24, 25, 24000, 30000, 30};
  const std::int64_t denominators[] = {1, 1, 1001, 1001, 1};
  for (int i = 0; i < 5; ++i) {
    const aether::FrameRate rate{numerators[i], denominators[i]};
    const aether::TimingResult first = aether::sampleCount(0, 8, 48000, rate);
    const aether::TimingResult second = aether::sampleCount(8, 16, 48000, rate);
    const aether::TimingResult both = aether::sampleCount(0, 16, 48000, rate);
    const aether::TimingResult again = aether::sampleCount(0, 8, 48000, rate);
    const aether::TimingResult at100 = aether::sampleCount(100, 108, 48000, rate);
    const aether::TimingResult at1000 = aether::sampleCount(1000, 1008, 48000, rate);
    QVERIFY(first.ok() && second.ok() && both.ok() && again.ok() && at100.ok() && at1000.ok());
    QCOMPARE(first.value + second.value, both.value);
    QCOMPARE(first.value, again.value);
    QVERIFY(at100.value > 0);
    QVERIFY(at1000.value > 0);
  }
  QVERIFY(countIs(100, 108, 24, 1, 16000));
  QVERIFY(countIs(1000, 1008, 30000, 1001, 12812));
  QVERIFY(countIs(8, 16, 30000, 1001, 12813));
  const aether::TimingResult zero = aether::sampleCount(0, 0, 48000, aether::FrameRate{24, 1});
  QVERIFY(zero.ok());
  QCOMPARE(zero.value, static_cast<std::int64_t>(0));
}

void TimingTests::longWalkMatchesBoundary() {
  const std::int64_t numerators[] = {24, 25, 24000, 30000, 30};
  const std::int64_t denominators[] = {1, 1, 1001, 1001, 1};
  const std::int64_t expected[] = {400000000, 384000000, 400400000, 320320000, 320000000};
  for (int rateIndex = 0; rateIndex < 5; ++rateIndex) {
    const aether::FrameRate rate{numerators[rateIndex], denominators[rateIndex]};
    std::int64_t sum = 0;
    for (std::int64_t frame = 0; frame < 200000; ++frame) {
      const aether::TimingResult span = aether::sampleCount(frame, frame + 1, 48000, rate);
      QVERIFY(span.ok());
      sum += span.value;
    }
    const aether::TimingResult end = aether::sampleBoundary(200000, 48000, rate);
    QVERIFY(end.ok());
    QCOMPARE(sum, end.value);
    QCOMPARE(end.value, expected[rateIndex]);
  }
}

void TimingTests::negativeAndOverflowFailClosed() {
  const aether::FrameRate rate{30000, 1001};
  const aether::TimingResult negative = aether::sampleBoundary(-1, 48000, rate);
  QCOMPARE(static_cast<int>(negative.error), static_cast<int>(aether::TimingError::NegativePosition));
  QCOMPARE(negative.value, static_cast<std::int64_t>(0));

  const aether::TimingResult negativeSpan = aether::sampleCount(-1, 8, 48000, rate);
  QCOMPARE(static_cast<int>(negativeSpan.error), static_cast<int>(aether::TimingError::NegativePosition));

  const aether::TimingResult inverted = aether::sampleCount(8, 7, 48000, rate);
  QCOMPARE(static_cast<int>(inverted.error), static_cast<int>(aether::TimingError::InvalidSpan));

  const aether::TimingResult badRate = aether::sampleBoundary(1, 48000, aether::FrameRate{0, 1});
  QCOMPARE(static_cast<int>(badRate.error), static_cast<int>(aether::TimingError::InvalidRate));
  const aether::TimingResult badSample = aether::sampleBoundary(1, 0, rate);
  QCOMPARE(static_cast<int>(badSample.error), static_cast<int>(aether::TimingError::InvalidRate));

  const aether::TimingResult overflow =
      aether::sampleBoundary(INT64_MAX, INT64_MAX, aether::FrameRate{1, INT64_MAX});
  QCOMPARE(static_cast<int>(overflow.error), static_cast<int>(aether::TimingError::Overflow));
  QCOMPARE(overflow.value, static_cast<std::int64_t>(0));
}
