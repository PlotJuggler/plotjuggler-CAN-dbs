#
# KASM
#
FROM kasmweb/core-ubuntu-noble:1.17.0
USER root

ENV HOME=/home/kasm-default-profile
ENV STARTUPDIR=/dockerstartup
ENV INST_SCRIPTS=$STARTUPDIR/install
WORKDIR $HOME

######### Customize Container Here ###########
RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y \
    build-essential \
    can-utils \
    cmake \
    fd-find \
    file \
    git \
    iproute2 \
    kmod \
    libfmt-dev \
    liblz4-dev \
    libmosquittopp-dev \
    libprotoc-dev \
    libqt5opengl5-dev \
    libqt5serialbus5-dev \
    libqt5serialport5-dev \
    libqt5svg5-dev \
    libqt5websockets5-dev \
    libqt5x11extras5-dev \
    libzmq3-dev \
    libzstd-dev \
    python3-can \
    python3-pip \
    qtbase5-dev \
    qttools5-dev \
    qttools5-dev-tools \
    wget \
    && rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*

ENV PATH="${PATH}:/usr/lib/cargo/bin"

# Dirty, but it is only for dev
RUN pip3 install --break-system-packages --ignore-installed cantools

WORKDIR /opt/plotjuggler

# Install plotJuggler
# 3a2e901 PlotJuggler
# 8f8d62f plotjuggler-CAN-dbs
RUN git clone --depth 1 --recurse-submodules https://github.com/facontidavide/PlotJuggler.git . &&\
    cd plotjuggler_plugins &&\
    git clone --depth 1 --recurse-submodules https://github.com/PlotJuggler/plotjuggler-CAN-dbs.git &&\
    echo "add_subdirectory( plotjuggler-CAN-dbs )" >> CMakeLists.txt &&\
    cd ../ &&\
    mkdir build &&\
    cd build &&\
    cmake .. &&\
    make -j &&\
    make install

# Create desktop shortcut for PlotJuggler
RUN echo "[Desktop Entry]\n\
    Type=Application\n\
    Name=PlotJuggler\n\
    Exec=/usr/local/bin/plotjuggler\n\
    Icon=plotjuggler\n\
    Comment=PlotJuggler Data Visualization Tool\n\
    Categories=Development;Engineering;\n\
    Terminal=false\n\
    StartupNotify=true" > $HOME/Desktop/plotjuggler.desktop && \
    chmod +x $HOME/Desktop/plotjuggler.desktop && \
    chown 1000:1000 $HOME/Desktop/plotjuggler.desktop

# zsh in container
RUN sh -c "$(wget -O- https://github.com/deluan/zsh-in-docker/releases/download/v1.2.1/zsh-in-docker.sh)" -- \
    -p git \
    -p https://github.com/zsh-users/zsh-autosuggestions \
    -p https://github.com/zsh-users/zsh-completions

# Custom scripts
COPY scripts/custom_startup.sh $STARTUPDIR/custom_startup.sh
# Configure sudo for specific commands without password
RUN echo "kasm-user ALL=(ALL) NOPASSWD: /sbin/modprobe vcan, /usr/sbin/modprobe vcan, \
    /sbin/ip link add vcan0 type vcan, /usr/sbin/ip link add vcan0 type vcan, \
    /sbin/ip link set vcan0 type can bitrate 500000 restart-ms 100, /usr/sbin/ip link set vcan0 type can bitrate 500000 restart-ms 100, \
    /sbin/ip link set vcan0 up, /usr/sbin/ip link set vcan0 up" > /etc/sudoers.d/kasm-vcan && \
    chmod 0440 /etc/sudoers.d/kasm-vcan
COPY scripts/can_dbc_simulator.py /usr/local/bin/can_dbc_simulator

RUN chmod +x /usr/local/bin/can_dbc_simulator $STARTUPDIR/custom_startup.sh 

WORKDIR $HOME

######### End Customizations ###########
RUN chown 1000:0 $HOME
RUN $STARTUPDIR/set_user_permission.sh $HOME

ENV HOME=/home/kasm-user
WORKDIR $HOME
RUN mkdir -p $HOME && chown -R 1000:0 $HOME

USER 1000
