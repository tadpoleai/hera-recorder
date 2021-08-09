set -e
set -v

apt-get install -f -y \
  build-essential \
  g++ \
  sudo \
  wget \
  curl \
  dialog \
  unzip \
  cmake \
  git \
  libraw1394-11 \
  libusb-1.0-0 \
  libgtkmm-2.4-1v5 \
  libglademm-2.4-1v5 \
  libgtkmm-2.4-dev \
  libgtkglextmm-x11-1.2-dev \
  libglademm-2.4-dev \
  nodejs \
  npm \
  node-gyp \
  nodejs-dev \
  libconfig++-dev \
  libpcl-common1.8 \
  libboost-dev \
  libboost-test-dev \
  libboost-program-options-dev \
  libboost-filesystem-dev \
  libboost-thread-dev \
  libevent-dev \
  libsmbclient-dev \
  automake \
  libtool \
  flex \
  bison \
  pkg-config \
  g++ \
  libssl1.0-dev

apt-get install -y -f --no-install-recommends \
  ros-melodic-ros-base \
  ros-melodic-roscpp \
  ros-melodic-rosbag \
  ros-melodic-sensor-msgs \
  ros-melodic-geometry-msgs \
  ros-melodic-tf2 \
  ros-melodic-tf2-geometry-msgs \
  ros-melodic-tf \
  ros-melodic-pcl-conversions \
  ros-melodic-pcl-ros \
  libpcl-dev

apt-get autoremove -y -f