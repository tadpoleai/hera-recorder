
#!/usr/bin/bash

pushd $(dirname ${BASH_SOURCE[0]}) > /dev/null

if [[ ! $ROS_PACKAGE_PATH =~ $(pwd) ]]; then
    ROS_PACKAGE_PATH=$(pwd):$ROS_PACKAGE_PATH
fi

chmod -x `find -name package.xml`

if [[ ! $PYTHONPATH =~ $(pwd) ]]; then
    PYTHONPATH=$(pwd):$PYTHONPATH
fi

popd > /dev/null
