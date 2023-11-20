/* Copyright 2023 teamprof.net@gmail.com
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this
 * software and associated documentation files (the "Software"), to deal in the Software
 * without restriction, including without limitation the rights to use, copy, modify,
 * merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#pragma once
#include <stdint.h>

// Definition of regions in lane
// When swimmer is in "region left", a "laneLeft" sound is output from earphone
// When swimmer is in "region middle", a "laneMiddle" sound is output from earphone
// When swimmer is in "region right", a "laneRight" sound is output from earphone
//
// Lane
//   |<------------2.5m------------>|
//   |  region .  region  . region  |
//   |  left   .  middle  . right   |
//   |<-0.75m->.<-1.00m-->.<-0.75m->|
//   |         .          .         |
//   |         .          .         |
//   |         .          .         |
//   |         .          .         |
//  left                           right
//  lane                           lane
//  rope                           rope
//

class SwimmingPool
{
public:
    static constexpr float PoolLenght = 50.0;
    static constexpr float PoolWidth = 25.0;
    static constexpr float NUM_LANES = 10.0;

    static constexpr float LaneRopeRegionX = 0.75 / (PoolWidth / NUM_LANES);
};
