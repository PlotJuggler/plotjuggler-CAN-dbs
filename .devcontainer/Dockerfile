#
# KASM
#
FROM kasmweb/core-ubuntu-noble:1.16.1
USER root

ENV HOME=/home/kasm-default-profile
ENV STARTUPDIR=/dockerstartup
ENV INST_SCRIPTS=$STARTUPDIR/install
WORKDIR $HOME

######### Customize Container Here ###########

# I
RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y \
    build-essential \
    can-utils \
    cmake \
    file \
    git \
    iproute2 \
    kmod \
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
    qtbase5-dev \
    wget \
    && rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*

WORKDIR /opt/plotjuggler

# 3a2e901 PlotJuggler
# 8f8d62f plotjuggler-CAN-dbs
RUN git clone --depth 1 --recurse-submodules https://github.com/miltkall/PlotJuggler.git . &&\
    cd plotjuggler_plugins &&\
    git clone --depth 1 --recurse-submodules https://github.com/miltkall/plotjuggler-CAN-dbs.git &&\
    echo "add_subdirectory( plotjuggler-CAN-dbs )" >> CMakeLists.txt &&\
    cd ../ &&\
    mkdir build &&\
    cd build &&\
    cmake .. &&\
    make -j &&\
    make install

WORKDIR $HOME

######### End Customizations ###########

RUN chown 1000:0 $HOME
RUN $STARTUPDIR/set_user_permission.sh $HOME

ENV HOME=/home/kasm-user
WORKDIR $HOME
RUN mkdir -p $HOME && chown -R 1000:0 $HOME

USER 1000

