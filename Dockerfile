FROM tiryoh/ros-desktop-vnc:noetic

# ROSリポジトリを追加（既に含まれている可能性がありますが、念のため）
RUN sh -c 'echo "deb http://packages.ros.org/ros/ubuntu $(lsb_release -sc) main" > /etc/apt/sources.list.d/ros-latest.list'
RUN apt-key adv --keyserver 'hkp://keyserver.ubuntu.com:80' --recv-key C1CF6E31E6BADE8868B172B4F42ED6FBAB17C654

RUN apt-get update && apt-get install -y \
    git \
    python3-rosdep \
    python3-catkin-tools \
    ros-noetic-pcl-ros \
    libpcl-dev \
    ros-noetic-map-server \
    ros-noetic-tf2-sensor-msgs \
    ros-noetic-move-base-msgs \
    && rm -rf /var/lib/apt/lists/*


WORKDIR /home/ubuntu

RUN mkdir -p /home/ubuntu/catkin_ws/src

WORKDIR /home/ubuntu/catkin_ws/src
RUN git clone https://github.com/at-wat/neonavigation_rviz_plugins.git &&\
    git clone https://github.com/suke-toudara/neonavigation.git && \
    git clone https://github.com/at-wat/neonavigation_msgs.git 
    

WORKDIR /home/ubuntu/catkin_ws

# rosdepの初期化と更新
RUN rosdep init || true
RUN rosdep update

# 依存関係のインストール
RUN rosdep install --from-paths src --ignore-src -y -r -v

# catkin_makeでビルド
RUN /bin/bash -c 'source /opt/ros/noetic/setup.bash && catkin_make -DCMAKE_BUILD_TYPE=Release'

# ROS環境を設定
RUN echo "source /opt/ros/noetic/setup.bash" >> /home/ubuntu/.bashrc && \
    echo "source /home/ubuntu/catkin_ws/devel/setup.bash" >> /home/ubuntu/.bashrc

# 作業ディレクトリを設定
WORKDIR /home/ubuntu

# エントリーポイントを設定（オプション）
CMD ["/bin/bash"]