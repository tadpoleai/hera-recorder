#!/bin/bash

ps -aux | grep hera-slam[-] | awk '{print $2}' | xargs kill -9

ps -aux | grep carto[_] | awk '{print $2}' | xargs kill -9

ps -aux | grep robot_state[_] | awk '{print $2}' | xargs kill -9

ps -aux | grep ros | awk '{print $2}' | xargs kill -9
