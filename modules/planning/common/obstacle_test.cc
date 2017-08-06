/******************************************************************************
 * Copyright 2017 The Apollo Authors. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *****************************************************************************/

/**
 * @file
 **/

#include <memory>
#include <unordered_map>
#include <vector>

#include "gtest/gtest.h"

#include "modules/perception/proto/perception_obstacle.pb.h"
#include "modules/prediction/proto/prediction_obstacle.pb.h"

#include "modules/common/util/file.h"
#include "modules/common/util/util.h"
#include "modules/planning/common/obstacle.h"

namespace apollo {
namespace planning {

using apollo::perception::PerceptionObstacle;
TEST(Obstacle, IsStaticObstacle) {
  perception::PerceptionObstacle perception_obstacle;
  EXPECT_TRUE(Obstacle::IsStaticObstacle(perception_obstacle));

  perception_obstacle.set_type(perception::PerceptionObstacle::UNKNOWN);
  EXPECT_FALSE(Obstacle::IsStaticObstacle(perception_obstacle));

  perception_obstacle.set_type(
      perception::PerceptionObstacle::UNKNOWN_UNMOVABLE);
  EXPECT_TRUE(Obstacle::IsStaticObstacle(perception_obstacle));

  perception_obstacle.set_type(perception::PerceptionObstacle::UNKNOWN_MOVABLE);
  EXPECT_FALSE(Obstacle::IsStaticObstacle(perception_obstacle));

  perception_obstacle.set_type(perception::PerceptionObstacle::PEDESTRIAN);
  EXPECT_FALSE(Obstacle::IsStaticObstacle(perception_obstacle));

  perception_obstacle.set_type(perception::PerceptionObstacle::BICYCLE);
  EXPECT_FALSE(Obstacle::IsStaticObstacle(perception_obstacle));

  perception_obstacle.set_type(perception::PerceptionObstacle::VEHICLE);
  EXPECT_FALSE(Obstacle::IsStaticObstacle(perception_obstacle));
}

class ObstacleTest : public ::testing::Test {
 public:
  virtual void SetUp() {
    prediction::PredictionObstacles prediction_obstacles;
    ASSERT_TRUE(common::util::GetProtoFromFile(
        "modules/planning/common/testdata/sample_prediction.pb.txt",
        &prediction_obstacles));
    std::list<std::unique_ptr<Obstacle>> obstacles;
    Obstacle::CreateObstacles(prediction_obstacles, &obstacles);
    ASSERT_EQ(5, obstacles.size());
    for (auto& obstacle : obstacles) {
      const auto id = obstacle->Id();
      indexed_obstacles_.Add(id, std::move(obstacle));
    }
  }

 protected:
  IndexedObstacles indexed_obstacles_;
};

TEST_F(ObstacleTest, CreateObstacles) {
  ASSERT_EQ(5, indexed_obstacles_.Items().size());
  ASSERT_TRUE(indexed_obstacles_.Find("2156_0"));
  ASSERT_TRUE(indexed_obstacles_.Find("2156_1"));
  ASSERT_TRUE(indexed_obstacles_.Find("2157_0"));
  ASSERT_TRUE(indexed_obstacles_.Find("2157_1"));
  ASSERT_TRUE(indexed_obstacles_.Find("2161"));
}

TEST_F(ObstacleTest, GetPointAtTime) {
  ASSERT_TRUE(indexed_obstacles_.Find("2156_0"));
  const auto* obstacle = indexed_obstacles_.Find("2156_0");

  // first
  const auto first_point = obstacle->GetPointAtTime(0.0);
  EXPECT_FLOAT_EQ(0.0, first_point.relative_time());
  EXPECT_FLOAT_EQ(76.684071405, first_point.path_point().x());
  EXPECT_FLOAT_EQ(350.481852505, first_point.path_point().y());

  // last
  const auto last_point = obstacle->GetPointAtTime(10044.15320);
  EXPECT_FLOAT_EQ(10044.1531943, last_point.relative_time());
  EXPECT_FLOAT_EQ(186.259371951, last_point.path_point().x());
  EXPECT_FLOAT_EQ(341.853799387, last_point.path_point().y());

  // middle
  const auto middle_point = obstacle->GetPointAtTime(3730.0);
  EXPECT_LE(3689.68892853, middle_point.relative_time());
  EXPECT_GE(3894.67164678, middle_point.relative_time());
  EXPECT_GE(139.091700103, middle_point.path_point().x());
  EXPECT_LE(135.817210975, middle_point.path_point().x());
  EXPECT_GE(349.875902219, middle_point.path_point().y());
  EXPECT_LE(349.549888973, middle_point.path_point().y());
}

TEST_F(ObstacleTest, PerceptionBoundingBox) {
  ASSERT_TRUE(indexed_obstacles_.Find("2156_0"));
  const auto* obstacle = indexed_obstacles_.Find("2156_0");
  const auto& box = obstacle->PerceptionBoundingBox();

  std::vector<common::math::Vec2d> corners;
  box.GetAllCorners(&corners);
  EXPECT_EQ(4, corners.size());
  EXPECT_FLOAT_EQ(3.832477, box.length());
  EXPECT_FLOAT_EQ(1.73200099013, box.width());
  EXPECT_FLOAT_EQ(76.684071405, box.center_x());
  EXPECT_FLOAT_EQ(350.481852505, box.center_y());
  EXPECT_FLOAT_EQ(0.00531211859358, box.heading());
}

TEST_F(ObstacleTest, GetBoundingBox) {
  ASSERT_TRUE(indexed_obstacles_.Find("2156_0"));
  const auto* obstacle = indexed_obstacles_.Find("2156_0");
  const auto& point = obstacle->Trajectory().trajectory_point(2);
  const auto& box = obstacle->GetBoundingBox(point);
  std::vector<common::math::Vec2d> corners;
  box.GetAllCorners(&corners);
  EXPECT_EQ(4, corners.size());
  EXPECT_FLOAT_EQ(3.832477, box.length());
  EXPECT_FLOAT_EQ(1.73200099013, box.width());
  EXPECT_FLOAT_EQ(83.2581699369, box.center_x());
  EXPECT_FLOAT_EQ(350.779556678, box.center_y());
  EXPECT_FLOAT_EQ(0.040689919, box.heading());
}

TEST_F(ObstacleTest, Trajectory) {
  ASSERT_TRUE(indexed_obstacles_.Find("2156_0"));
  const auto* obstacle = indexed_obstacles_.Find("2156_0");
  const auto& points = obstacle->Trajectory().trajectory_point();
  EXPECT_EQ(50, points.size());
}

TEST_F(ObstacleTest, Perception) {
  ASSERT_TRUE(indexed_obstacles_.Find("2156_0"));
  const auto* obstacle = indexed_obstacles_.Find("2156_0");
  const auto& perception_obstacle = obstacle->Perception();
  EXPECT_EQ(2156, perception_obstacle.id());
}
}  // namespace planning
}  // namespace apollo
