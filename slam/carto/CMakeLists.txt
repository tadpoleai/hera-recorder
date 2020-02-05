cmake_minimum_required(VERSION 3.5.1)
project(hera-slam-carto)

install(DIRECTORY "share" DESTINATION ${SLAM_CARTO_ROOT})

install(
  FILES bin/carto_node
  PERMISSIONS OWNER_EXECUTE OWNER_READ GROUP_EXECUTE GROUP_READ WORLD_EXECUTE
              WORLD_READ
  DESTINATION /usr/local/bin)

install(
  FILES bin/carto_occupancy_grid_node
  PERMISSIONS OWNER_EXECUTE OWNER_READ GROUP_EXECUTE GROUP_READ WORLD_EXECUTE
              WORLD_READ
  DESTINATION /usr/local/bin)
