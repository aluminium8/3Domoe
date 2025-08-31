#pragma once

#include <Eigen/Dense>

namespace MITSU_Domoe
{
    struct Polygon_mesh
    {
        Eigen::MatrixXd V;
        Eigen::MatrixXi F;
        Eigen::MatrixXd N;
    };
    struct Polygon_clusters
    {
    };
    struct Point_cloud
    {
    };
} // namespace MITSU_Domoe
